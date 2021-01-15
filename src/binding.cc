#include "node.h"
#include <mutex>
#include <unordered_map>
#include <set>

using namespace node;
using namespace v8;

namespace interruptor_bindings {

template <typename T> inline void USE(T&&) {}

std::set<uint64_t> interrupted;

class Interruptor {
 public:
  explicit Interruptor(Isolate* isolate);
  ~Interruptor();
  static std::pair<Interruptor*, std::unique_lock<std::mutex>> ForIndex(uint64_t index);

  void Interrupt();
  uint64_t index() const { return index_; }

 private:
  Isolate* isolate_;
  std::mutex mutex_;
  bool has_interrupted_ = false;
  uint64_t index_;

  static std::unordered_map<uint64_t, Interruptor*> maps_;
  static std::mutex maps_mutex_;
  static uint64_t current_index_;
};
std::unordered_map<uint64_t, Interruptor*> Interruptor::maps_;
std::mutex Interruptor::maps_mutex_;
uint64_t Interruptor::current_index_ = 0;

Interruptor::Interruptor(Isolate* isolate) : isolate_(isolate) {
  std::unique_lock<std::mutex> lock(maps_mutex_);
  index_ = current_index_++;
  maps_[index_] = this;
}

Interruptor::~Interruptor() {
  {
    std::unique_lock<std::mutex> lock(maps_mutex_);
    maps_.erase(index_);
  }
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (has_interrupted_) {
      isolate_->CancelTerminateExecution();
    }
  }
}

std::pair<Interruptor*, std::unique_lock<std::mutex>> Interruptor::ForIndex(uint64_t index) {
  std::unique_lock<std::mutex> lock(maps_mutex_);
  auto it = maps_.find(index);
  return {
    it == maps_.end() ? nullptr : it->second,
    std::move(lock)
  };
}

void Interruptor::Interrupt() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!has_interrupted_) {
    isolate_->TerminateExecution();
    has_interrupted_ = true;
    interrupted.insert(index_);
  }
}

void Interrupt(const FunctionCallbackInfo<Value>& args) {
  assert(args[0]->IsBigInt());
  uint64_t index = args[0].As<BigInt>()->Uint64Value();
  std::pair<Interruptor*, std::unique_lock<std::mutex>> result =
      Interruptor::ForIndex(index);
  if (result.first != nullptr) {
    result.first->Interrupt();
  }
}

void HasInterrupted(const FunctionCallbackInfo<Value>& args) {
  assert(args[0]->IsBigInt());
  uint64_t index = args[0].As<BigInt>()->Uint64Value();
  args.GetReturnValue().Set(interrupted.count(index) == 1);
}

void RunInterruptible(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();
  Interruptor interruptor(isolate);

  Local<Value> index = BigInt::NewFromUnsigned(isolate, interruptor.index());
  assert(args[0]->IsFunction());
  MaybeLocal<Value> ret = args[0].As<Function>()->Call(
      context,
      Null(isolate),
      1,
      &index);
  if (!ret.IsEmpty())
    args.GetReturnValue().Set(ret.ToLocalChecked());
}

NODE_MODULE_INIT() {
  Isolate* isolate = context->GetIsolate();
  exports->Set(context,
               String::NewFromUtf8(
                  isolate, "runInterruptible", NewStringType::kInternalized)
                  .ToLocalChecked(),
               FunctionTemplate::New(isolate, RunInterruptible)
                  ->GetFunction(context).ToLocalChecked()).Check();
  exports->Set(context,
               String::NewFromUtf8(
                  isolate, "interrupt", NewStringType::kInternalized)
                  .ToLocalChecked(),
               FunctionTemplate::New(isolate, Interrupt)
                  ->GetFunction(context).ToLocalChecked()).Check();
  exports->Set(context,
               String::NewFromUtf8(
                  isolate, "hasInterrupted", NewStringType::kInternalized)
                  .ToLocalChecked(),
               FunctionTemplate::New(isolate, HasInterrupted)
                  ->GetFunction(context).ToLocalChecked()).Check();
}

}

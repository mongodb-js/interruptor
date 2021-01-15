import bindings from 'bindings';
const native = bindings('interruptor');
export type InterruptHandle = { __id: number };

export function runInterruptible<Ret>(
  fn: (handle: InterruptHandle) => Ret
): Ret | void {
  return native.runInterruptible((index: number) => {
    return fn({ __id: index });
  });
}

export function interrupt(handle: InterruptHandle): void {
  native.interrupt(handle.__id);
  // Make a dummy call to make sure that the termination exception is propagated.
  process.memoryUsage();
}

export function hasInterrupted(handle: InterruptHandle): boolean {
  return native.hasInterrupted(handle.__id);
}

function load() {
  try {
    return require('../build/Release/interruptor.node');
  } catch {
    // Webpack will fail when just returning the require, so we need to wrap in a try/catch and rethrow.
    /* eslint no-useless-catch: 0 */
    try {
      return require('../build/Debug/interruptor.node');
    } catch (error) {
      throw error;
    }
  }
}

const native = load();

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

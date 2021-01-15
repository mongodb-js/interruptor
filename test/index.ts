import assert from 'assert';
import { Worker } from 'worker_threads';
import { runInterruptible, interrupt, InterruptHandle } from '../';

describe('runInterruptible', () => {
  it('can interrupt itself', () => {
    let i = 0;
    runInterruptible((handle: InterruptHandle) => {
      i++;
      interrupt(handle);
      i++;
    });
    assert.strictEqual(i, 1);
  });

  it('is a no-op after the call', () => {
    let savedHandle;
    runInterruptible((handle: InterruptHandle) => {
      savedHandle = handle;
    });
    interrupt(savedHandle);
  });

  it('can be interrupted from a worker thread', () => {
    runInterruptible((handle: InterruptHandle) => {
      // eslint-disable-next-line no-new
      new Worker(`
        require(${JSON.stringify(require.resolve('../'))}).interrupt(
          require('worker_threads').workerData.handle);`, {
        eval: true,
        workerData: { handle }
      });
      while (true);
    });
  });
});

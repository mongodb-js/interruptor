# interruptor – Run a function with the possibility to interrupt it from another thread

## Usage Example

```js
import { runInterruptible, interrupt } from 'interruptor';
runInterruptible(handle => {
  // pass handle to another thread using .postMessage();

  while(true);
});

// In another thread:
interrupt(handle);
```

## Caveats

This is a native addon, and currently no pre-built binaries are available.

This only interrupts synchronous execution inside the callback, not async
functions (although the `microtaskMode` option for the `vm` module in
Node.js 14 and above can help with Promises here).

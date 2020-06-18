# Monitor

This is an implementation of handler monitor using WooFs. There can only be one single handler running inside a monitor at the same time.

## Usage

To make a monitor and use it to manage handlers, do the followings:

1. Use `monitor_create(monitor_name)` to create a monitor. Don't call it more than once.
2. Replace `WooFPut(...)` with `monitor_put(monitor_name, ...)`.
3. Inside the handler, use `monitor_cast(ptr)` instead of `ptr` to access the element pointer. Remember to free it before the handler returns.
4. Inside the handler, instead of using `seq_no` to access the element sequence number, use `monitor_seqno(ptr)`.
5. Before exiting from handler (or as soon as exiting critical section), call `monitor_exit(ptr)`. Don't call it more than once.

See [test_handler_monitored.c](test_handler_monitored.c) and [test_monitor.c](test_monitor.c) for example.

## Queuing handler

Calling `monitor_put(monitor_name, ...)` will put the element to the specified WooF, and queue a handler invocation to the monitor. The monitor will invoke the handler in their queued order one by one when `monitor_exit(ptr)` is called in the previous handler. Instead of using `monitor_put(monitor_name, ...)`, you can also call `WooFPut(woof_name, NULL, &element)` then use `monitor_queue(monitor_name, woof_name, handler, seq_no)` to queue the handler.

## Remote handler

To use a monitor to coordinate handlers on a remote server, use `monitor_remote_put(monitor_uri, ...)`. The monitor URI and WooF name must be complete URI including IP address and namespace. Notice even though handlers in a monitor can be invoked by a remote client, a monitor can only coordinate handlers on the same server.
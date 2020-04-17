# Monitor

This is an implementation of handler monitor using WooFs. There can only be one single handler running inside a monitor at the same time.

## Usage

To make a monitor and use it to manage handlers, simply 

1. Use `monitor_create(monitor_name)` to create a monitor.
2. Replace `WooFPut(...)` with `monitor_put(monitor_name, ...)`.
3. Inside the handler, use `monitor_cast(ptr)` instead of `ptr` to access the element.
4. Before exiting from handler, call `monitor_exit(ptr)`. 

See [test_handler_monitored.c](test_handler_monitored.c) and [test_monitor.c](test_monitor.c) for example.

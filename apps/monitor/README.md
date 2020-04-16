# Monitor

This is an implementation of handler monitor using WooFs. There can only be one handler running inside the WooF monitor at the same time.

## Usage

To make a monitored WooF, simply 

1. replace `WooFCreate()` and  with `monitor_create()` and `monitor_put()`, and 
2. add a `monitor_exit(wf, seq_no)` at the end of monitored handler, 
3. use `monitor_get()` instead of `WooFGet()` to get element from monitored WooFs.

See [monitor_test_handler.c](monitor_test_handler.c) and [monitor_test.c](monitor_test.c) for example.

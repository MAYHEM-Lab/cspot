# cspot
c spot run

log-test-thread
  -- if the local logs are too small, then they wrap and lose the
     dependency chain causing infinitely pending events.
  -- if intermediate syncing is done, the logs will differ in terms
     of ordering of independent events

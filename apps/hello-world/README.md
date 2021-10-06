# Hello World CSPOT Example
This directory contains a simple example utilizing the CSPOT framework.

After running 'make', the following binaries will be in the 'cspot' directory:
* woofc-namespace-platform
  * This is the general framework binary built in the top-level CSPOT repository
  * This binary creates a log and launches containers in Docker
* woofc-container
  * This binary is launched in a Docker context by 'woofc-namespace-platform'
  * It first starts a message server (found in woofc-access.c), which listens for and responds to messages based on their first frame's tag (e.g. performing a put, get, etc. on the WooF)
  * It then creates a pool of WooFForker threads that fork into some handler binary (in this example, 'hw') when the log is changed
* hw-start/hw-client
  * These binaries are representative of client programs that interact with a WooF in some namespace
  * They create a WooF and append an element to it via 'WooFPut', specifying that a binary named 'hw' should be triggered after the operation
* hw
  * This binary handler is triggered on 'WooFPut's
  * It simply prints the fact that it's been triggered by printing the originating WooF and message given

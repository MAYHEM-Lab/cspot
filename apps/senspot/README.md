# Senspot Example Application

To run:
1) (on *server* machine): $ ./senspot-init -W la-sensor -s 10000
2) (on *server* machine): $ ./woofc-namespace-platform
3) (on *client* machine, any of the following):
  * $ ./core-temp-sensor.sh woof://<ip-address-of-server>:<port>/path/to/la-sensor 
  * $ ./senspot-get -W woof://<ip-address-of-server>/path/to/la-sensor -S <seq-id>
  * $ ./read-temp.sh woof://<ip-address-of-server>/path/to/la-sensor 5

Note that the *server* and *client* machine can be the same, in which case you'll need to supply the '-L' flag with the use of `senspot-get`, i.e.
  * $ echo 45 | ./senspot-put -L -W la-sensor -T d
  * $ ./senspot-get -L -W la-sensor

For example, I ran this application distributed across my local machine while running a virtual machine in VirtualBox.
As a quick guide to setting that up:

1. Download and install a CentOS image onto VirtualBox
2. Ensure networking in CentOS, then:
   a. Get the IP address of CentOS machine, via its command line: $ hostname -I
   b. Remember this IP address (let's call it <ADDR>)
3. Install `cspot` on the CentOS machines (probably already done if you're reading this), then
   a. Determine the port that the WooF namespace will be listening on via: $ ./woofc-namespace-platform
   b. It will say "starting platform listening to port <PORT>"; remember this port (let's call it <PORT>)
4. In VirtualBox:
   a. Navigate to 'Machine > Settings > Network'
   b. Ensure that "Enable Network Adapter" is checked and "Attached To" is set to "NAT"
   c. Select 'Advanced > Port Forwarding' and in the new window, select the button "Add new port forwarding rule"
   d. Add the following rule:
      * Name: Rule 1
      * Protocol: TCP
      * Host IP: 127.0.0.1
      * Host Port: 2222
      * Guest IP: <ADDR>   # from step 2b.
      * Guest Port: 22
   e. Add the following rule:
      * Name: Rule 2
      * Protocol: TCP
      * Host IP: 127.0.0.1
      * Host Port: 2223
      * Guest IP: <ADDR>   # from step 2b.
      * Guest Port: <PORT> # from step 3b.
4. On the machine hosting VirtualBox:
   a. Navigate to your `senspot` example directory
   b. Run $ ./core-temp-sensor.sh woof://127.0.0.1:<PORT>/path/to/la-sensor 

Thus the VirtualBox VM will be running the CSPOT instance containing the WooF, and your local machine will be acting as a client whose temperature readings are sent to the VM.

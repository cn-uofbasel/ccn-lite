# Start CCN-Lite

## Installation CCN-Lite

1. `git clone https://github.com/cn-uofbasel/ccn-lite`
2. Set the ccn-lite env: `export CCNL_HOME=".../.../ccn-lite"` (don't forget to add it to your  bash profile if you want it to persist)
3. Ubuntu: `sudo apt-get install libssl-dev`

   OSX: `brew install openssl` (assuming the [homebrew](http://brew.sh) packet manager is installed)

3. `make clean all` in `$CCNL_HOME`

## Running Simple Demo Script `demo-relay.sh`

This script starts, connects and fills the content store for two relays A and B (`A[] ---> B[/ndn/abc]`). With the help of the `ccn-lite-peek` tool an interest for a content object (`f1`) in the cache of node `B` is send to node `A`. The returned data is piped to the `ccn-lite-pktdump` tool which detects the packet format and converts the packet to a readable format. To test it out, first run `sh $CCNL_HOME/test/script/demo-relay.sh` to see the usage. A valid command is `sh $CCNL_HOME/test/script/demo-relay.sh ndn2013 udp false`. 

## Explanation of the Demo Script

Since the script uses several variables for the setup and the content object already exist, we are going to explain every step in the following. This should help you to get a simple CCN-Lite environment setup.

### Producing content

`ccn-lite-mkC` creates a content object in a specified wireformat. CCN-Lite currently supports three wireformats. We use `ndn2013` in the following. `ccnb` and `ccnx2014` are also available. 

```bash
$CCNL_HOME/util/ccn-lite-mkC -s ndn2013 "/ndn/test/mycontent" > $CCNL_HOME/test/ndntlv/mycontent.ndntlv
```
type something, your text will be used as data

### Starting ccn-lite-relay `A` 

`ccn-lite-relay` is a ccn network router (or forwarder).

`-v` indicates the loglevel (will change in the future). 
`-u` sets the relay to listen on UDP port `9998`.
`-x` sets up a unix socket, we will use that port later to send management commands. 

```bash
$CCNL_HOME/ccn-lite-relay -v 99 -s ndn2013 -u 9998 -x /tmp/ccn-lite-relay-a.sock
```

### Starting ccn-lite-relay `B`
Similar to starting relay `A`, but on a different port. Additional, with `-d` we add all content objects from a directory to the cache of the relay. Currently the relay expects all files to have the file extension `.ndntlv` (and `.ccntlv`/`.ccnb` respectively).
Open a new terminal window for relay `B`.

```bash
$CCNL_HOME/ccn-lite-relay -v 99 -s ndn2013 -u 9999 -x /tmp/ccn-lite-relay-b.sock -d $CCNL_HOME/test/ndntlv
```

### Add a forwarding rule
The two relays are not yet connected to each other. We need a forwarding rule from node `A` to node `B`.
On node `A`, we first add a new face. In this case, the face should point to a UDP socket (address of node `B`).
We need to remember the ID of this face for the next step. Again, open a new terminal window.
```bash
FACEID=`$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/ccn-lite-relay-a.sock newUDPface any 127.0.0.1 9999 | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
```
Next, we need to define the namespace of the face. We choose `/ndn` because our test content as well as all objects in `./test/ndntlv` have a name starting with `/ndn`. Later, all interest which match with the longest prefix on this name will be forwarded to this face. Relay `A` is now connected to relay `B` but relay `B` does not have a forwarding rule to node `B` (`A ---> B`).
```bash
$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/ccn-lite-relay-a.sock prefixreg /ndn $FACEID ndn2013 | $CCNL_HOME/util/ccn-lite-ccnb2xml 
```

### Send interest for name `/ndn/test/mycontent/` to node `A`
The `ccn-lite-peek` utility encodes the specified name in a interest with the according suite and sends it to a socket. In this case we want it to send the interst to the the relay `A`. Relay `A` will receive the interest, forward it to node `B` which will in turn respond with our initally created content object to relay `A`. Relay `A` sends the content objects back to peek, which prints it to stdout. Here, we also pipe the output to `ccn-lite-pktdump` which detects the encoded format (here ndntlv) and prints the wireformat-encoded packet in a readable format.
```bash
$CCNL_HOME/util/ccn-lite-peek -s ndn2013 -u 127.0.0.1/9998 "/ndn/test/mycontent/" | $CCNL_HOME/util/ccn-lite-pktdump
```

















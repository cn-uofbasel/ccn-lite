A
# CCN-Lite and NFN Tutorial

## Introduction

This Tutorial explains and demonstrates two different scenarios. The first scenario is static content lookup in a CCN network. The second scenario is a function call and dynamic content creation in a NFN network.

![alt text](scenario-content-lookup.png)

This scenario involves a client sending an interest to a network, consisting of heterogeneous CCN implementations. The interest will be fulfilled by either a content store within the network or a producer of content.

For CCN, there exist mainly three different implementations and packet formats. The original CCNb encoding and the currently evolving NDN and CCNx implementations. CCN-Lite is a lightweight, cross-compatible implementation of CCN. It is written in C and (optionally) runs in the kernel space.

The second scenario is dynamic content creation with NFN by calling and chaining functions.

![scenario function call](scenario-function-call.png)

This scenario shows both the extended CCN-Lite router with NFN capabilities to manipulate computation names as well as distribute computation and it shows the external compute environment which is responsible to carry out the actual computations.

## Scenario 1: Simple Content-lookup
![content-lookup-simple](demo-content-lookup-simple.png)
This scenario consists of a topology of two nodes `A` and `B` each running an instance of the CCN-Lite relay. The cache of relay `B` is populated with some content and a forwarding rule is setup from node `A` to node `B`. Interests are send to node `A`.

### 0. Installing CCN-Lite

1. `git clone https://github.com/cn-uofbasel/ccn-lite`
2. Set the CCN-Lite env: `export CCNL_HOME=".../.../ccn-lite"` (don't forget to add it to your  bash profile if you want it to persist)
3. Dependencies:
	* Ubuntu: `sudo apt-get install libssl-dev`
	* OSX: `brew install openssl` (assuming the [homebrew](http://brew.sh) packet manager is installed)

4. `make clean all` in `$CCNL_HOME`

### 1. Producing Content

`ccn-lite-mkC` creates a content object in a specified wireformat. CCN-Lite currently supports three wireformats. We use `ndn2013` in the following, `ccnb` and `ccnx2014` are also available. 

```bash
$CCNL_HOME/util/ccn-lite-mkC -s ndn2013 "/ndn/test/mycontent" > $CCNL_HOME/test/ndntlv/mycontent.ndntlv
```
Type something, your text will be used as the data for the content object.

### 2. Starting ccn-lite-relay `A` 

`ccn-lite-relay` is a ccn network router (or forwarder).

`-v` indicates the loglevel. 

`-u` sets the relay to listen on UDP port `9998`.

`-x` sets up a Unix socket, we will use that port later to send management commands. 

```bash
$CCNL_HOME/ccn-lite-relay -v 99 -s ndn2013 -u 9998 -x /tmp/mgmt-relay-a.sock
```

### 3. Starting ccn-lite-relay `B`
Similar to starting relay `A`, but on a different port. Additional, with `-d` we add all content objects from a directory to the cache of the relay. Currently the relay expects all files to have the file extension `.ndntlv` (and `.ccntlv`/`.ccnb` respectively).
Open a new terminal window for relay `B`.

```bash
$CCNL_HOME/ccn-lite-relay -v 99 -s ndn2013 -u 9999 -x /tmp/mgmt-relay-b.sock -d $CCNL_HOME/test/ndntlv
```

### 4. Add a Forwarding Rule
The two relays are not yet connected to each other. We need a forwarding rule from node `A` to node `B`.
`ccn-lite-ctrl` provides a set of management commands to configure and maintain a relay.
On node `A`, we first add a new face. In this case, the face should point to a UDP socket (address of node `B`).
We need to remember the ID of this face for the next step. Again, open a new terminal window.
```bash
FACEID=`$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock newUDPface any 127.0.0.1 9999 | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
```
Next, we need to define the namespace of the face. We choose `/ndn` because our test content as well as all objects in `./test/ndntlv` have a name starting with `/ndn`. Later, all interest which match with the longest prefix on this name will be forwarded to this face. Relay `A` is now connected to relay `B` but relay `B` does not have a forwarding rule to node `B` (`A ---> B`).
```bash
$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock prefixreg /ndn $FACEID ndn2013 | $CCNL_HOME/util/ccn-lite-ccnb2xml 
```

### 5. Send Interest for Name `/ndn/test/mycontent/` to `A`
The `ccn-lite-peek` utility encodes the specified name in a interest with the according suite and sends it to a socket. In this case we want it to send the interest to the the relay `A`. Relay `A` will receive the interest, forward it to node `B` which will in turn respond with our initially created content object to relay `A`. Relay `A` sends the content objects back to peek, which prints it to stdout. Here, we also pipe the output to `ccn-lite-pktdump` which detects the encoded format (here ndntlv) and prints the wireformat-encoded packet in a readable format.
```bash
$CCNL_HOME/util/ccn-lite-peek -s ndn2013 -u 127.0.0.1/9998 "/ndn/test/mycontent/" | $CCNL_HOME/util/ccn-lite-pktdump
```
## Demo 2: Content Lookup from NDN Testbed
![content-lookup-NDNTestbed](demo-content-lookup-NDNTestbed.png)
Similar to Scenario 1, but this time the network consists of the NDN Testbed instead of a set of CCN-Lite relays. 


Peek sends the command directly to a node in the NDN Testbed. `-w` sets the timeout of peek to 10 seconds.
```bash
$CCNL_HOME/util/ccn-lite-peek -s ndn2013 -u 192.43.193.111/6363 -w 10 "/ndn/edu/ucla" | $CCNL_HOME/util/ccn-lite-pktdump
```

## Scenario 3: Connecting CCNL with NDNTestbed
![content-lookup-CCNL-NDNTestbed](demo-content-lookup-CCNL-NDNTestbed.png)
Scenario 3 combines Scenario 1 and 2 by connecting a (local) CCN-Lite relay to the NDNTestbed and sending interests to it. The relay will forward the interests to the testbed.


### 1. Shutdown relay `B`
To shutdown a relay we can use the ctrl tool.
```bash
$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt-relay-b.sock debug halt 
```

### 2. Remove Face to `B`
To see the current configuration of the face, use:
```bash
$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml
```
Now we can destroy the face:
```bash
$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock destoryface $FACEID | $CCNL_HOME/util/ccn-lite-ccnb2xml 
```
And check again if the face was actually removed.

### 2. Connecting `A` to Testbed
```bash
$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock newUDPface any 192.43.193.111 6363| $CCNL_HOME/util/ccn-lite-ccnb2xml
```

### 3. Send interest to `A`
```bash
$CCNL_HOME/util/ccn-lite-peek -s ndn2013 -u 127.0.0.1/9998 -w 10 "/ndn/edu/ucla" | $CCNL_HOME/util/ccn-lite-pktdump
```

## Scenario 4: Local NFN Relay

![](demo-function-call-simple)
Scenario 4 represents a simple NFN example with only one node. The node runs CCN-Lite with NFN enabled capabilities. A simple computation which is executed directly in the abstract machine in the network is send to this node.

### Scenario 5: Sending a Computation request for an Abstract Machine
![](demo-function-call-ext)
This Scenario is similar to Scenario 4, but additionally there is an external compute environment connected to the NFN enabled CCN-Lite relay. The compute environment hosts a single Named Function called `WordCount` which counts the number of words in a document.

## Scenario 5: NFN request with external Named Function / Service 



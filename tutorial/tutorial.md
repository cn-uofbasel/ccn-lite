# CCN-lite 

## Table Of Contents:
- [Introduction](#introduction)
- [Scenario 1: Simple Content-lookup](#scenario1)
- [Scenario 2: Content Lookup from NDN Testbed](#scenario2)
- [Scenario 3: Connecting CCNL with NDN Testbed](#scenario3)

<a name="introduction"/>
## Introduction

This tutorial explains and demonstrates three scenarios for 
Content-Centric-Networking (CCN) use cases.

As of 2014, there exist mainly three different CCN implementations and packet
formats. a) The original CCNx software and its CCNb encoding, as well as the
currently evolving b) Named-Data Networking (NDN) software and c) the new
CCNx1.0 implementation.

CCN-lite is a forth implementation: It is a lightweight and cross-compatible
implementation of Content Centric Networking which "speaks" all three dialects.
It is written in pure C and (optionally) runs in the Linux kernel space.
Moreover, it contains an extension to CCN called Named-Function-Networking,
which is also covered in this tutorial.

![alt text](scenario-content-lookup.png)

The first three scenarios of this tutorial demonstrate static content
lookup in a CCN network. We let a client send interests into a network
that consists of heterogeneous CCN implementations (our CCN-lite as
well as the NDN testbed). The interest will be fulfilled by either a
content store within the network or a producer of content. Note that although
CCN-lite handles all packet formats, the client has to pick one format and can
only access content that is encoded (and made routable) in that chosen format.

![scenario function call](scenario-function-call.png)



<a name="scenario1"/>
## Scenario 1: Simple content-lookup

![content-lookup-simple](demo-content-lookup-simple.png)

This scenario consists of a topology of two nodes `A` and `B` each running an
instance of the CCN-lite relay. The cache of relay `B` is populated with some
content and a forwarding rule is setup from node `A` to node `B`. Interests are
send to node `A`.


### 0. Installing CCN-lite

Install CCN-lite by following the [Unix readme](../doc/README-unix.md).


### 1. Producing content

`ccn-lite-mkC` creates an (unsigned) content object in a specified wire format,
subject to the maximum packet size of 4 KiB. `ccn-lite-mkC` currently supports
five wire formats. We use `ndn2013` in the following, `ccnb`, and `ccnx2015`
are also available. `ccn-lite-mkC` converts input from
stdin, so type something and press `Enter` after executing the following line:

```bash
$CCNL_HOME/build/bin/ccn-lite-mkC -s ndn2013 "/ndn/test/mycontent" > $CCNL_HOME/test/ndntlv/mycontent.ndntlv
```


### 2. Starting `ccn-lite-relay` for node `A`

`ccn-lite-relay` is a ccn network router (or forwarder). Type
`ccn-lite-relay -h` to see all available options. We will use the following:
* `-v` indicates the loglevel.
* `-u` sets the relay to listen on UDP port `9998`.
* `-x` sets up a Unix socket, we will use this socket to send management
  commands to the relay.

```bash
$CCNL_HOME/build/bin/ccn-lite-relay -v trace -s ndn2013 -u 9998 -x /tmp/mgmt-relay-a.sock
```


### 3. Starting `ccn-lite-relay` for node `B`

We start the relay for `B` similarly to relay `A` but on a different port.
Additional, with `-d` we add all content objects from a directory to the cache
of the relay. Currently the relay expects all files to have the file extension
`.ndntlv`, `.ccnb`, or `.ccntlv` respectively.
Open a new terminal window for relay `B`:

```bash
$CCNL_HOME/build/bin/ccn-lite-relay -v trace -s ndn2013 -u 9999 -x /tmp/mgmt-relay-b.sock \
  -d $CCNL_HOME/test/ndntlv
```


### 4. Add a forwarding rule

The two relays are not yet connected to each other. We want to add a forwarding
rule from node `A` to node `B` which is a mapping of a prefix to an outgoing
face. Thus, we first need to create the face on relay `A` followed by defining
the forwarding rule for `/ndn`.

`ccn-lite-ctrl` provides a set of management commands to configure and maintain
a relay. These management commands are based on a request-reply protocol using
interest and content objects. Again, type `ccn-lite-ctrl -h` to see all
available options.

Currently the ctrl tool is hardwired for the `ccnb` format (but the relay still
handles packets in all other formats, too). To decode the reply of the ctrl tool
we use `ccn-lite-ccnb2xml`.

Finally, because faces are identified by dynamically assigned numbers, we need
to extract the `face id` from the reply of the `create face` command. When defining
the forwarding rule we can then refer to this `face id`.

For creating the face at node `A`, open a new terminal window:
```bash
FACEID=`$CCNL_HOME/build/bin/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock newUDPface any 127.0.0.1 9999 \
  | $CCNL_HOME/build/bin/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/^[^0-9]*\([0-9]\+\).*/\1/'`
```
For defining the namespace that should become reachable through this face, we
have to configure a forwarding rule. We choose `/ndn` as namespace (prefix)
pattern because our test content as well as all objects in `./test/ndntlv` have
a name starting with `/ndn`. Later, all interest which match with the longest
prefix on this name will be forwarded to this face.

In other words: Relay `A` is technically connected to relay `B` through the UDP
face, but logically, relay `A` does not yet have the necessary forwarding state
to reach `B`. To create a forwarding rule (`/ndn ---> B`), we execute:
```bash
$CCNL_HOME/build/bin/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/build/bin/ccn-lite-ccnb2xml
```

You might want to verify a relay's configuration through the built-in HTTP
server. Just point your browser to `http://127.0.0.1:6363/`.

This ends the configuration part and we are ready to use the two-node setup for
experiments.


### 5. Send Interest for Name `/ndn/test/mycontent/` to `A`

The `ccn-lite-peek` utility encodes the specified name in a interest with the
according suite and sends it to a socket. In this case we want `ccn-lite-peek`
to send an interest to relay `A`. Relay `A` will receive the interest, forward
it to node `B` which will in turn respond with our initially created content
object to relay `A`. Relay `A` sends the content objects back to peek, which
prints it to stdout. Here, we pipe the output to `ccn-lite-pktdump` which
detects the encoded format (here `ndn2013`) and prints the wire format-encoded
packet in a somehow readable format.

```bash
$CCNL_HOME/build/bin/ccn-lite-peek -s ndn2013 -u 127.0.0.1/9998 "/ndn/test/mycontent" \
  | $CCNL_HOME/build/bin/ccn-lite-pktdump
```

If you want to see only the content use the `-f 2` output format option:
```bash
$CCNL_HOME/build/bin/ccn-lite-peek -s ndn2013 -u 127.0.0.1/9998 "/ndn/test/mycontent" \
  | $CCNL_HOME/build/bin/ccn-lite-pktdump -f 2
```



<a name="scenario2"/>
## Scenario 2: Content lookup from NDN Testbed

![content-lookup-NDNTestbed](demo-content-lookup-NDNTestbed.png)

Similar to Scenario 1, but this time the network consists of the NDN Testbed
instead of a set of CCN-lite relays.

Peek sends the interest directly to a node in the NDN Testbed. `-w` sets the
timeout of peek to 10 seconds.

```bash
$CCNL_HOME/build/bin/ccn-lite-peek -s ndn2013 -u 192.43.193.111/6363 -w 10 "/ndn/edu/ucla/ping" \
  | $CCNL_HOME/build/bin/ccn-lite-pktdump
```

Note: `/ndn/edu/ucla/ping` dynamically creates a new content packet with a
limited lifetime and random name extension. Due to the network level caching,
repeating the above command might return a copy instead of triggering a new
response. Try it out!



<a name="scenario3"/>
## Scenario 3: Connecting a CCN-lite relay to the NDN Testbed

![content-lookup-CCNL-NDNTestbed](demo-content-lookup-CCNL-NDNTestbed.png)

Scenario 3 combines Scenario 1 and 2 by connecting a (local) CCN-lite relay to
the NDN Testbed and sending interests to it. The relay will forward the
interests to the testbed.


### 1. Shutdown relay `B`

To properly shutdown a relay we can use `ccn-lite-ctrl`:
```bash
$CCNL_HOME/build/bin/ccn-lite-ctrl -x /tmp/mgmt-relay-b.sock debug halt | $CCNL_HOME/build/bin/ccn-lite-ccnb2xml
```


### 2. Remove face to `B`

To see the current configuration of the face, use:
```bash
$CCNL_HOME/build/bin/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock debug dump | $CCNL_HOME/build/bin/ccn-lite-ccnb2xml
```
Now we can destroy the face:
```bash
$CCNL_HOME/build/bin/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock destroyface $FACEID \
  | $CCNL_HOME/build/bin/ccn-lite-ccnb2xml
```

Check again if the face was actually removed.


### 3. Connecting node `A` directly to the NDN Testbed

Connect to the NDN testbed server of the University of Basel by creating a new
UDP face to the NFD of Basel and then registering the prefix `/ndn`:
```bash
FACEID=`$CCNL_HOME/build/bin/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock newUDPface any 192.43.193.111 6363 \
  | $CCNL_HOME/build/bin/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/^[^0-9]*\([0-9]\+\).*/\1/'`
```

```bash
$CCNL_HOME/build/bin/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/build/bin/ccn-lite-ccnb2xml
```


### 4. Send interest to `A`

Request data from the Testbed system of the UCLA. The interest will be
transmitted over the Testbed server of the University of Basel to the Testbed
system of the UCLA:
```bash
$CCNL_HOME/build/bin/ccn-lite-peek -s ndn2013 -u 127.0.0.1/9998 -w 10 "/ndn/edu/ucla" \
  | $CCNL_HOME/build/bin/ccn-lite-pktdump
```

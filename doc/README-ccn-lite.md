# README-ccn-lite.md (2014-12-23)

The main code base is on GitHub at [https://github.com/cn-uofbasel/ccn-lite)](https://github.com/cn-uofbasel/ccn-lite). If you want to file a bug report, please create an issue on GitHub (and: thanks a lot!)

If you are interested in running CCN-lite, see the fine [tutorial](../tutorial/tutorial.md)!

If you are interested in the latest (bleeding edge) code, see the [dev-master](https://github.com/cn-uofbasel/ccn-lite/tree/dev-master) branch on GitHub.

### Abstract

CCN-lite is a reduced and lightweight --yet functionally
interoperable-- implementation of the CCNx protocol. It covers:

-  the classic CCNx ccnb version (Nov 2013)
-  the Named-Data-Networking (NDN) protocol (as of Nov 2013)
-  the CCNx1.0 encoding (unofficial, as of Dec 2014)
-  an experimental and compact encoding for IoT environments (Nov 2014)
-  the novel Named-Function-networking approach (University of Basel)

CCN-lite runs in UNIX user space, as well (most of it) as a Linux
kernel module, making it suitable for resource constraint devices like
the Raspberry Pi, sensor nodes and the Internet of Things (IoT). The
same, unchanged, code runs in the OMNeT simulator (shipped independently
of this CCN-lite course release).

CCN-lite is meant as a code base for class room work, experimental
extensions and simulation experiments. The ISC license makes it an
excellent starting point for commercial products.


### Table of Contents
1. [Rationale for CCN-lite, what you get (and what is missing)](#rationale)
2. [Extensions to CCNx](#extensions)
3. [CCN-lite supported platforms and how to compile](#platforms)
4. [Command Line Tools](#lof)
5. [How to start](#start)
6. [Useful links](#links)

- - -

<a name="rationale"></a>

### 1. Rationale for CCN-lite, what you get (and what is missing)

The original motivation in 2011 for creating CCN-lite was that PARC's CCNx
router software had grown huge. CCN-lite provides a lean alternative
for educational purposes and as a stepping stone. It's for those who
want a simple piece of software for experimentation or own
development, but who do not need all features of the full thing.

This brings the interesting question of interoperability: What minimum
functionality is necessary in order to participate in a CCNx network?
With CCN-lite, we have made a first attempt at identifying that set,
but we are aware that this set might be incomplete, or may change
depending on how CCNx evolves. We consider this question of
"sufficient CCNx functionality" to be of engineering and academic
interest of its own and welcome contributions from everybody.

Hence, with CCN-lite, we did a race-to-the-bottom and strived for a kind
of "Level-0" interoperability. Level-0, as we understood it,
covers:

- ccnb encoding of messages (or any other encoding flavor, including the TLV variants)
- PIT and FIB (CCN basic data structures)
- longest and exact prefix matching (for basic CCN operations),
  including minsuffixcomp und maxsuffixcomp handling
- matching of publisher's public key (to fight cache poisoning)
- nonce and/or hop limit tracking (to avoid loops as a minimal safeguard)

As an interoperability goal we set for ourselves the threshold that
the CCN-lite software must be able to route at the CCN level between
fully fledged CCNx forwarders. Of course, this is a moving target, but
we regularly use our tools to interface with the NDN testbed, for
example.

What we (deliberately) do NOT cover in our CCN-lite code is:

- sophisticated data structures for performance optimisations (it's up to
  you to exchange the linked lists with whatever fits your needs)
- exclusion filtering in interests
- all TCP connectivity and the old CCNx cmd line utilities that have TCP
  hardwired into them (we are datagram purists)
- crypto functionality, which here is not our prime concern
- repository functionality, SYNC server etc

A second reason to create CCN-lite was the software license (the
original CCNx project picked GNU, but NDN also has chosen this path)
where we prefer a Berkeley Software Distribution style of "do whatever
you want" as we believe that this will help adopting CCN
technology. Therefore, CCN-lite is released under the ISC license. We
have learned that several companies have picked up CCN-lite for their
internal experiments, so we think our license choice was right.

What you get with CCN-lite is:

- a tiny CCNx core (1000-2000 lines of C suffice)
- three supported platforms: UNIX user space, Linux kernel, OMNeT
  all using the same core logic (OMNeT to be re-released independently),
  Docker files are also provided
- partially interoperable management protocol implementation
- a simple HTTP server to display the relay's internal config
- plus some interesting extensions of our own, see the next section.

We are running CCN-lite on PCs, but also on the Raspberry Pi, both the
user space version as well as the kernel module.

- - -

<a name="extensions"></a>

## 2. Extensions to CCNx

  In several selected areas we have started our own "experiments",
  resulting in contributions that are now part of CCN-lite:

  a) *Named functions* for letting clients express "cooked" results
     instead of accessing only raw data. See also the use of
     [SCALA](https://github.com/cn-uofbasel/nfn-scala)
     to host function execution and to interface to a NFN network.

  b) Experimental *RPC functionality* for letting neighbors mutually invoke
     functions, which could be the starting point both for network management
     functionality and for data marshalling (of interests and data objects)
     using the TLV encoding.

  c) Clean *packet scheduler support* at chunk level as well as packet or
     fragment level (symbol USE_SCHEDULER)

  d) *Packet fragmentation* and lost packet detection support for running
     the CCNx protocol natively over Ethernet (symbol USE_FRAG).
     This is somehow outdated and waits for protocol specs to
     emerge.

  Other features that you can switch on and off at compile time are:
```
    USE_CCNxDIGEST        // enable digest component (requires crypto lib)
    USE_CHEMFLOW          // experimental scheduler, src not included
    USE_DEBUG             // basic data structure dumping
    USE_DEBUG_MALLOC      // compile with memory armoring
    USE_ETHERNET          // talk to eth/wlan devices, raw frames
    USE_FRAG              // enable fragments (to run CCNx over Ethernet)
    USE_HTTP_STATUS       // provide status info for web browsers
    USE_LOGGING           // enable log messages
    USE_KITE              // forthcoming (routing along the return path)
    USE_MGMT              // react to CCNx mgmt protocol messages
    USE_NACK              // NACK support (for NFN)
    USE_NFN               // named function networking
    USE_SCHEDULER         // rate control at CCNx msg and fragment level
    USE_SIGNATURES        // authenticate mgmt messages
    USE_SUITE_*           // multiprotocol (CCNB, NDN2013, CCNx2014, IOT2014, LOCALRPC)
    USE_UNIXSOCKET        // add UNIX IPC to the set of interfaces
```

The approach for these extensions is that one can tailor a CCN
forwarder (or NDN etc) to including only those features really
necessary.  We have strived to make these choices as orthogonal as
possible and invite you to attempt the same for your additions.

- - -

<a name="platforms"></a>

## 3. CCN-lite supported platforms and how to compile

  The CCN-lite code currently supports three platforms, defined in
  corresponding main source files (they shouild compile under Linux and
  Mac OSX). In these main files, a symbol is set which selects the platform:

```
  CCNL_UNIX         UNIX user space relay    ccn-lite-relay.c

  CCNL_LINUXKERNEL  Linux kernel module      ccn-lite-lnxkernel.c
  CCNL_OMNET        OmNet++ integration      ccn-lite-omnet.c
```
  The main forwarder is called ccn-lite-relay. Two additional main
  source files are provided for demo purposes:

  The *ccn-lite-simu.c* is a standalone simulator that internally
  runs a simple 4 node topology - it is used just for basic functional
  testing, e.g. hunting for memory leaks.

  The *ccn-lite-minimalrelay.c* is an exercise in writing the least
  C code possible in order to get a working NDN forwarder. It has all
  extra features disabled and only provides UDP connectivity. But
  hey, it works! And it really IS lean, looking at the lines of
  C code:
```
   417 ccn-lite-minimalrelay.c
   940 ccnl-core.c
   696 ccnl-core-fwd.c    // only partially needed
   737 ccnl-core-util.c
   498 ccnl-pkt-ndntlv.c  // only partially needed

   346 ccnl-core.h
   163 ccnl-defs.h
    95 ccnl-pkt-ndntlv.h
```

  The provided Makefile compiles the code for the CCNL_UNIX
  platform - just type make.

  If you want named function support (NFN), define an environment variable at the
  shell level before invoking make:
  export USE_NFN=1; make clean all

  If you want NFN *and* NACK support, define an additional environment variable
  before invoking make:
  export USE_NACK=1; make clean all

  If you want to build the Linux kernel module, define an environment
  variable at the shell level before invoking make:
  export USE_KRNL=1; make clean all

  The OMNeT platform is currently out-dated and has moved to an
  independent project, but will be available soon (target date is in
  early 2015).

  Ubuntu needs the following packages:
    libssl-dev
  (load them with "sudo apt-get install ...")

- - -

<a name="lof"></a>

## 4. Command Line Tools

The main files (and corresponding executables) in the src directory are:
```
   ccn-lite-relay.c              CCN-lite forwarder: user space

                                 This code is compiled to three executables,
                                 depending on compile time environment flags:
                                   ccn-lite-relay
                                   ccn-nfn-relay
                                   ccn-nfn-relay-nack

   ccn-lite-lnxkernel.c          CCN-lite forwarder: Linux kernel module


   util/ccn-lite-ctrl.c          cmd line program running the CCNx mgmt
                                 protocol (over UNIX sockets). Used for
                                 configuring a running relay either running
                                 in user space or as a kernel module

   util/ccn-lite-ccnb2xml.c      simple ccnb parser

   util/ccn-lite-cryptoserver.c  used by the kernel module to carry out
                                 compute intensive crypto operations (in
                                 user space)

   util/ccn-lite-ctrl.c          mgmt tool to talk to a relay

   util/ccn-lite-fetch.c         fetches both a single chunk content, or
                                 a series of chunk for larger named data.
                                 The real content is returned (without
                                 any protocol bytes)

   util/ccn-lite-mkC.c           simple content composer (to stdout, no crypto)

   util/ccn-lite-mkI.c           simple interest composer (to stdout)

   util/ccn-lite-peek.c          simple interest injector waiting for
                                 a content chunk, can also be used to
                                 request named-function results

   util/ccn-lite-pktdump.c       powerful packet dumper for all known packet
                                 formats. Output is in hexdump style, XML
                                 or just the pure content.

   util/ccn-lite-produce.c       creates a series of chunks for data that
                                 does not fit into a single PDU

   util/ccn-lite-rpc.c           send an RPC request and return the reply

   util/ccn-lite-simplenfn.c     a simplified interface to request named-
                                 function results
```

- - -

<a name="start"></a>

## 5. How to start

The best way to start is to work through the
[tutorial](../tutorial/tutorial.md)!

### 5') Starting (and controlling) the Linux kernel module

```
  #-- insert the module, enabeling eth0, debug msgs
  % sudo insmod ./ccn-lite-lnxkernel.ko e=eth0 v=99

  #-- talk to the module via the default UNIX socket, trigger a dump
  % ./util/ccn-lite-ctrl debug dump | ./util/ccn-lite-ccnb2xml

  #-- have a look at the kernel output
  % tail -f /var/log/syslog

  #-- remove the module
  % sudo rmmod ccn-lite-lnxkernel
```

- - -

<a name="links"></a>

## 6. Links:

- Source code repository:
  https://github.com/cn-uofbasel/ccn-lite

- CCN-lite web site:
  http://www.ccn-lite.net/

- CCNx site:
  http://www.ccnx.org/

- NDN site:
  http://named-data.net/

- NFN site:
  http://named-function.net/

- - -

History:

* 2012-10-12 created (christian.tschudin@unibas.ch)
* 2013-07-27 part of ccn-lite release 0.1.0 on GitHub
* 2014-12-23 this version (release 0.2.0)

- - -

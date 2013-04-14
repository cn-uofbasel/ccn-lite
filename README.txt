# ccn-lite/README.txt

# history:
# 2012-10-12 created (christian.tschudin@unibas.ch)
# 2013-04-08 this version


Abstract:

    CCN-lite is a reduced and lightweight, but yet
    functionally interoperable implementation of
    the CCNx protocol. It is as a code base for
    class room work, experimental extensions, CCN
    relays running on resource constraint devices,
    and for commercial products.


Table of Contents:

    1) Rationale for CCN-lite, what you get (and what is missing)
    2) Extensions to CCNx
    3) CCN-lite supported platforms and how to compile
    4) List of files
    5) Useful links

---

1) Rationale for CCN-lite, what you get (and what is missing)

The original motivation for creating CCN-lite was that PARC's CCNx
router software has grown huge. CCN-lite provides a lean alternative
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

Hence, with CCN-lite, we do a race-to-the-bottom and strive for a kind
of "Level-0" interoperability. Level-0, as we currently understand it,
should cover:

- ccnb encoding of messages (PDU level compatibility)
- PIT and FIB (CCN basic data structures)
- longest prefix matching (for basic CCN operations)
- matching of publisher's public key (to fight cache poisoning)
- nonce tracking (to avoid loops as a minimal safeguard)

As an interoperability goal we set for ourselves the threshold that
the CCN-lite software must be able to route at the CCN level between
fully fledged CCNx daemons so that one can successfully run
applications such as PARC's chat and the SYNC protocol over a
heterogeneous CCNx network:

 ccnchat or sync                                    ccnchat or sync
         |                                                  |
        ccnd <--> ccn-lite-relay <--> ccn-lite-relay <--> ccnd

What we do NOT cover in our CCN-lite code is:

- sophisticated data structures for performance optimisations (it's up to
  you to exchange the linked lists with whatever fits your needs)
- negative content selection (the cumbersome to use Bloom filters etc),
  and scoping
- all TCP connectivity and CCNx cmd line utilities that have TCP
  hardwired into them (we are datagram purists)
- crypto functionality, which here is not our prime concern
- repository functionality, SYNC server etc (but these can be
  used in a CCN-lite context as described above, or implemented
  externally)

A second reason to create CCN-lite was the software license (CCNx
picked GNU) where we prefer a Berkeley Software Distribution style of
"do whatever you want" as we believe that this will help adopting CCN
technology. Therefore, CCN-lite is released under the ISC license.

So what you get with CCN-lite is:
- a tiny CCNx core (1000 lines of C)
- three supported platforms: UNIX user space, Linux kernel, OMNeT
  all using the same core logic
- partially interoperable management protocol implementation
- plus some interesting extensions of our own, see the next section.

---

2) Extensions to CCNx

  In two selected areas we have started our own "experiments",
  resulting in contributions that are now part of CCN-lite:

  a) fragmentation and lost packet detection support for running
     the CCNx protocol natively over Ethernet (symbol USE_ENCAPS)

  b) clean scheduler support at chunk level as well as packet
     level (symbol USE_SCHEDULER)

  Other features that you can switch on and off at compile time are:

    USE_DEBUG             // enable log messages
    USE_DEBUG_MALLOC      // compile with memory armoring
    USE_ETHERNET          // talk to eth/wlan devices, raw frames
    USE_MGMT              // react to CCNx mgmt protocol messages
    USE_UNIXSOCKET        // add UNIX IPC to the set of interfaces
    USE_CHEMFLOW          // experimental scheduler, src not included

---

3) CCN-lite supported platforms and how to compile

  The CCN-lite code currently supports three platforms, defined in
  corresponding main source files. In these main files, a symbol is
  set which selects the platform:

  CCNL_UNIX         UNIX user space relay    ccn-lite-relay.c
  CCNL_LINUXKERNEL  Linux kernel module      ccn-lite-lnxkernel.c
  CCNL_OMNET        OmNet++ integration      ccn-lite-omnet.c

  Two additional main source files are provided for demo purposes:

  CCNL_SIMULATION  UNIX user space simulator (ccn-lite-simu.c)
  CCNL_UNIX        (ccn-lite-minimalrelay.c)

  The ccn-lite-simu is a standalone simulator that internally runs
  a simple 4 node topology - it is just for basic functional
  testing (and we used it to hunt for memory leaks).

  The ccn-lite-minimalrelay is an exercise in writing the least
  C code possible in order to get a working CCN relay. It has all
  features disabled and only provides UDP connectivity. But hey,
  it works! And it really IS lean, looking at the lines of C code:

     66 ccnx.h
     73 ccnl.h
    998 ccnl-core.c
    385 ccn-lite-minimalrelay.c


  The provided Makefile compiles the code for the CCNL_UNIX and
  the CCNL-LINUXKERNEL platforms, as well as the two demo binaries
  CCNL_SIMULATION and ccn-lite-minimalrelay - just type make.

  The code for the OMNeT platform is a wrapper that needs to be
  compiled from within the OMNeT environment. More details and
  installation instructions can be found in README-OMNeT.txt

---

4) List of Files

CCN-lite main logic and extensions:
   ccnl-core.c
   ccnl-core.h
   ccnl-ext-debug.c
   ccnl-ext-encaps.c
   ccnl-ext-http.c
   ccnl-ext-mgmt.c
   ccnl-ext-sched.c
   ccnl-ext.h
   ccnl.h
   ccnx.h

CCN-lite portability:
   ccnl-includes.h
   ccnl-platform.c

CCN-lite main files for binaries: (= "platforms")
   ccn-lite-relay.c              CCN-lite user space
   ccn-lite-lnxkernel.c          CCN-lite Linux kernel module

   ccn-lite-minimalrelay.c       minimal user space prog for CCNx over UDP
   ccn-lite-simu.c               CCN-lite standalone simulation (as a unit test)
   ccnl-simu-client.c            used in the unit test above

Utilities:
   util/Makefile
   util/ccn-lite-ctrl.c          cmd line program running the CCNx mgmt
                                 protocol (over UNIX sockets). Used for
                                 configuring a running relay either running
                                 in user space or as a kernel module
   util/ccn-lite-parse.c         simple ccnb parser
   util/ccn-lite-peek.c          simple interest injector waiting for
                                 a content chunk

OMNeT:
   ccn-lite-omnet/               directory with OMNeT specific code
   ccn-lite-omnet.c              wrapper for CCN-lite inclusion into OMNet++

Documentation:
   ccnl-datastruct.pdf           diagram of CCN-lite's internal data structures
   ccnl-datastruct.dot           source file of above
   ccn-lite-logo-712x184.jpg     logo
   rts-cts-20120802.txt          MSC of the scheduler interactions
   Nice2012-poster.pdf           poster from the CCNxCon meeting in Nice

Misc:
   Makefile                      
   README.txt                    this file
   README-OMNeT.txt              OMNeT-specific instructions

---

5) Links:

   Repository of our CCN-lite software:
   https://github.com/cn-uofbasel/ccn-lite

   CCN-lite web site:
   http://ccn-lite.net/

   CCNx site: (including access to PARC's reference implementation)
   http://www.ccnx.org/

# eof

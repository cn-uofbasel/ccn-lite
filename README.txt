# ccn-lite/README.txt

# history:
# 2012-10-12 created (christian.tschudin@unibas.ch)
# 2013-07-27 this version


Abstract:

    CCN-lite is a reduced and lightweight, yet
    functionally interoperable, implementation
    of the CCNx protocol. It runs in UNIX user
    space as well as a Linux kernel module,
    making it suitable for resource constraint
    devices like the Raspberry Pi. The same,
    unchanged, code runs in the OMNet simulator.

    CCN-lite is meant as a code base for class
    room work, experimental extensions and
    simulation experiments. The ISC license makes
    it an excellent starting point for commercial
    products.

    Web page: http://www.ccn-lite.net
    Github:   https://github.com/cn-uofbasel/ccn-lite


Table of Contents:

    1) Rationale for CCN-lite, what you get (and what is missing)
    2) Extensions to CCNx
    3) CCN-lite supported platforms and how to compile
    4) List of files
    5) How to start
    6) Useful links

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
- longest prefix matching (for basic CCN operations),
  including minsuffixcomp und maxsuffixcomp handling
- matching of publisher's public key (to fight cache poisoning)
- nonce tracking (to avoid loops as a minimal safeguard)

As an interoperability goal we set for ourselves the threshold that
the CCN-lite software must be able to route at the CCN level between
fully fledged CCNx daemons so that one can successfully run
applications such as PARC's SYNC protocol over a heterogeneous
CCNx network:

  sync (ccnr)                                             sync (ccnr)
     |                                                       |
   ccnd <--> ccn-lite-relay <-- eth --> ccn-lite-relay <--> ccnd

See the script test/script/interop-SYNCoEth-a.sh how this is done.

What we do NOT cover in our CCN-lite code is:

- sophisticated data structures for performance optimisations (it's up to
  you to exchange the linked lists with whatever fits your needs)
- exclusion filtering in interests i.e., negative content selection
  like the cumbersome to use Bloom filters
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

We are running CCN-lite on PCs, but also on the Raspberry Pi, both the
user space version as well as the kernel module.

---

2) Extensions to CCNx

  In two selected areas we have started our own "experiments",
  resulting in contributions that are now part of CCN-lite:

  a) fragmentation and lost packet detection support for running
     the CCNx protocol natively over Ethernet (symbol USE_FRAG)

  b) clean scheduler support at chunk level as well as packet or
     fragment level (symbol USE_SCHEDULER)

  Other features that you can switch on and off at compile time are:

    USE_CCNxDIGEST        // enable digest component (requires crypto lib)
    USE_DEBUG             // enable log messages
    USE_DEBUG_MALLOC      // compile with memory armoring
    USE_FRAG              // enable fragments (to run CCNx over Ethernet)
    USE_ETHERNET          // talk to eth/wlan devices, raw frames
    USE_HTTP_STATUS       // provide status info for web browsers
    USE_MGMT              // react to CCNx mgmt protocol messages
    USE_SCHEDULER         // rate control at CCNx msg and fragment level
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

  The ccn-lite-simu.c (CCNL-SIMULATION) is a standalone simulator
  that internally runs a simple 4 node topology - it is used just for
  basic functional testing, e.g. hunting for memory leaks.

  The ccn-lite-minimalrelay.c is an exercise in writing the least
  C code possible in order to get a working CCNx relay. It has all
  extra features disabled and only provides UDP connectivity. But
  hey, it works! And it really IS lean, looking at the lines of
  C code:

     66 ccnx.h
     73 ccnl.h
    194 ccnl-core.h
    998 ccnl-core.c
    385 ccn-lite-minimalrelay.c


  The provided Makefile compiles the code for the CCNL_UNIX and
  the CCNL-LINUXKERNEL platforms, as well as the two demo binaries
  CCNL_SIMULATION and ccn-lite-minimalrelay - just type make.
  An additional BSDmakefile enables easy compilation on FreeBSD
  platforms, including Apple's OSX.

  The code for the OMNeT platform is a wrapper that needs to be
  compiled from within the OMNeT environment. More details and
  installation instructions can be found in README-OMNeT.txt

  Ubuntu needs the following packages:
    libssl-dev
  (load them with "sudo apt-get install ...")

---

4) List of Files

CCN-lite main logic and extensions:
   ccnl-core.c
   ccnl-core.h
   ccnl-ext-debug.c
   ccnl-ext-frag.c
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
   util/ccn-lite-ctrl.c          cmd line program running the CCNx mgmt
                                 protocol (over UNIX sockets). Used for
                                 configuring a running relay either running
                                 in user space or as a kernel module
   util/ccn-lite-ccnb2hex.c      simple ccnb parser
   util/ccn-lite-ccnb2xml.c      simple ccnb parser, currently only works
                                 for mgmt replies
   util/ccn-lite-peek.c          simple interest injector waiting for
                                 a content chunk
   util/ccn-lite-mkI.c           simple interest composer (to stdout)
   util/ccn-lite-mkC.c           simple content composer (to stdout, no crypto)
   util/ccn-lite-mkF.c           turns a ccnb object info fragments (files)
   util/ccn-lite-deF.c           reassemble fragment files

OMNeT:
   ccn-lite-omnet/               directory with OMNeT specific code
   ccn-lite-omnet.c              wrapper for CCN-lite inclusion into OMNet++

Documentation:
   ccnl-datastruct.pdf           diagram of CCN-lite's internal data structures
   ccnl-datastruct.dot           source file of above
   ccn-lite-logo-712x184.jpg     logo
   rts-cts-20120802.txt          MSC of the scheduler interactions
   Nice2012-poster.pdf           poster from the CCNxCon meeting in Nice, 2012

Test Scripts
   test/scripts/*.sh             consult these files for learning how to use
                                 CCN-lite, the mgmt protocol, and how to test
                                 interopability with PARC's ccnd

Misc:
   BSDmakefile
   Makefile                      
   README.txt                    this file
   README-OMNeT.txt              OMNeT-specific instructions

---

5) How to start

5.a) A simple hello-world config (user space relay)

  % cd the_ccn-lite_directory

  #-- start a CCNx relay in the background, preload the content store:
  % ./ccn-lite-relay -d test/ccnb &

  #-- have a look at the internal state
  % firefox 127.0.0.1:9695 &

  #-- retrieve content, in raw CCNx format:
  % ./util/ccn-lite-peek /ccnx/0.7.1/doc/technical/URI.txt

  #-- retrieve content, parse the fields
  % ./util/ccn-lite-peek /ccnx/0.7.1/doc/technical/URI.txt \
    | ./util/ccn-lite-ccnb2hex


5.b) Advanced configurations via the mgmt protocol

  #-- start the relay with sudo (if you want to access eth devices)
  % sudo ./ccn-lite-relay

  #-- bring up a wifi interface, listen for ethtype value 0x3456
  # sudo ./util/ccn-lite-ctrl newETHdev wlan0 0x3456 \
    | ./util/ccn-lite-ccnb2xml

  #-- define a peer reachable via wifi at 11:22:33:44:55:66, ethtype 0x9876
  % sudo ./util/ccn-lite-ctrl newETHface any 11:22:33:44:55:66 0x9876 \
    | ./util/ccn-lite-ccnb2xml

  #-- define a peer reachable at 10.0.0.1 and UDP port 9000, picking
  #   any of the machines IP interfaces
  % sudo ./util/ccn-lite-ctrl newUDPface any 10.0.0.1 9000 \
    | ./util/ccn-lite-ccnb2xml

  #-- add a routing entry (pick face 3 in this example)
  % sudo ./util/ccn-lite-ctrl prefixreg /acme.com/frontpage 3 \
    | ./util/ccn-lite-ccnb2xml

  #-- have a look at the internal state, repeatedly during the previous steps
  % firefox 127.0.0.1:9695


5.c) Starting (and controlling) the Linux kernel module

  #-- insert the module, enabeling eth0, debug msgs
  % sudo insmod ./ccn-lite-lnxkernel.ko e=eth0 v=99

  #-- talk to the module via the default UNIX socket, trigger a dump
  % ./util/ccn-lite-ctrl debug dump | ./util/ccn-lite-ccnb2xml

  #-- have a look at the kernel output
  % tail -f /var/log/syslog

  #-- remove the module
  % sudo rmmod ccn-lite-lnxkernel

---

6) Links:

   Source code repository:
   https://github.com/cn-uofbasel/ccn-lite

   CCN-lite web site:
   http://www.ccn-lite.net/

   CCNx site: (including access to PARC's reference implementation)
   http://www.ccnx.org/

# eof

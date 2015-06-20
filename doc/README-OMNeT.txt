# ccn-lite/README-OMNeT.txt

# history:
# 2013-04-17 created ({sifalakis.manos,christian.tschudin}@unibas.ch)
# 2015-06-19 updated ({sifalakis.manos,christian.tschudin}@unibas.ch)


Abstract:

    CCN-lite is a reduced and lightweight, but yet functionally interoperable
    implementation of the original CCNx protocol of PARC and its thereafter 
    derived ecosystem of protocols, extensions, packet encodings, etc. It 
    currently covers 

    -  the original CCNx ccnb version (Nov 2013)
    -  the Named-Data-Networking (NDN) protocol (as of Nov 2013)
    -  the CCNx1.0 encoding (unofficial, as of Dec 2014)
    -  an experimental and compact encoding for IoT environments (Nov 2014)
    -  the novel Named-Function-networking approach (University of Basel)

    CCN-lite runs in UNIX user space, as well (most of it) as a Linux kernel
    module, making it suitable for resource constraint devices like the 
    Raspberry Pi, sensor nodes and the Internet of Things (IoT). The same, 
    unchanged, code runs in the OMNeT simulator (shipped independently of this 
    CCN-lite course release).

    CCN-lite is meant as a code base for class room work, experimental
    extensions and simulation experiments. The ISC license makes it an
    excellent starting point for commercial products.

    This file describes the build process for preparing an omnet archive 
    bundle and how to import in OMNeT++ v4.5 and possibly later.

Table of Contents:

    1) Requirements
    2) How to generate an OMNeT++ archive
    3) How to import/install the code in OMNeT++ ver 4.5
    4) Further documentation and code examples

---

1) Tested Requirements

- OMNeT++ 4.5
- INET framework 2.4.0

Obviously you will need a working version of the OMNeT++
simulator. The latest version of OMNeT++ at the time of this
writing is ver 4.6. However, the herein described procedure 
has been tested to work with ver 4.5 (and there is no obvious
reason why it shouldn't work in ver 4.6 as well -- lest some
IDE GUI buttons and options may have been moved around menus).

To use CCN-lite with OMNeT++ in any meaningful experiments
you will need at least a version of the INET framework to
provide MAC level and socket level support. This version of
CCN-lite has been tested to work with INET 2.4.0. Again 
unless the major version number jump has been (3.x.x), it
is very likely that the code still works with later versions
of INET too (actually INET is likely more critical than the 
omnet version).

---

2) How to generate an OMNeT++ archive

From within the src/ directory of the CCN-lite release simply 
type and run

 $ make ccn-lite-omnet

This should produce in the same directory an OMNeT++ archive
bundle named

 ccn-lite-omnet.tgz

That's it!

---

3) How to import/install the code in OMNeT++ ver 4.5

After installing in OMNeT++ the INET framework, follow
exactly the same process to import ccn-lite-omnet. Namely, 

 (Menu) File
   ->Import

     ->General
     ->Existing Projects in Workspace
       NEXT
 
       ->(radio button select) Select Archive File
         Browse for the archive file you created in section 2

       ->(tick box activate) ccn-lite-omnet
         FINISH

Next proceed to re-index by right-clicking on the newly
imported project at the Project Explorer and selecting 

 Index -> Re-build

Finally, build the ccn-lite-omnet project

 (Menu) Project -> Build Project


__Important Note__: 

INET needs to be installed and already built before
you attempt to build ccn-lite-omnet, and in the same workspace

---

4) Further documentation and code

4.a) Contents of the bundle you generated in (2)

At this point you're ready to build your own topologies and
experiment with CCN in OMNeT++. To get you started if you re
not too familiar with OMNeT++ and INET, in the ccn-lite-omnet
project you will find the following sub-directories

  src/		Source code base for integrating ccn-lite
                in omnet. Therein the ccn-lite/ sub-directory 
		contains the parts of the ccn-lite source tree
		used in omnet
  topologies/   Contains a couple of simple example topologies
  compounds/    Contains a vanilla NED definition for a CCN 
		node over Ethernet (as used in the unit-test
		simulation scenario). This NED glues together
		INET functionality with the Ccn module in src.
  simulations/  An example simulation scenario for unit-tests 
		and for kickstarting your games with ccn-lite-omnet.
                Role files for each node in the scenario are 
		provided in the .cfg file included.

Some additional developer's documentation providing an
overview of the software architecture of the OMNeT++ wrapper
of CCN-lite, the current scenario configuration options, the
OMNeT++ message definitions and the function of the various
modules, can be found in the doc/omnet/ directory of the
CCN-lite source tree.


4.b) Key parts of the omnet code base

In brief the omnet part consists of:

- A class that abstracts away the integration with INET (CcnInet).

- A class that implements all the Ccn related functionality (Ccn),
  which inherits directly from CcnInet.

- A class that provides a c++ interface to the ccn-lite code (CcnCore), 
  and takes care of making it reentrant so that multiple ccn-lite 
  node incarnations can be instantiated in the same omnet process. 
  The Ccn class encapsulates CcnCore.

- A class that is the "deus ex-machina" of a ccn simulation scenario
  (CcnAdmin). It initialises the nodes and topology and makes things 
  you would like to simulate happen "by fate" during the simulation 
  (e.g. changes in the state of nodes, failures, initiation of requests,
  updates of CSs, etc). This class can be used to compensate for the 
  lack of established routing protocol implementations, cache 
  replacement policies, etc ; in a centralised and organised way.

- A number of other suport classes such as CcnContext, that wraps 
  ccn packets exchanged between ccn-lite nodes; TimerList, that is
  used for event services to ccn-lite nodes, etc.


4.c) Last, and most IMPORTANT: what to expect from it!

The code base of ccn-lite-omnet is far from complete, and far from
satisfying everyone's wish list! The intend (so far at least) has not 
to been to provide "a service" to the ICN community, but rather to 
provide a "starting point". Having taken care of the boring part of
the basic functionality, you should be able to build on top of this 
code-base the functionality you want to have for your experiments/
research.

(Or if you feel up to it why not even re-develop the basic functionality 
better, to get over its constraints and caveats).

The main starting point for building up your wish list would typically 
be the Ccn class. Either by extending it e.g. to integrate transport 
intelligence, CS management, etc .. or by writing an application layer 
that talks to it (and hosts your application logic).

# eof

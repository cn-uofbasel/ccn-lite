# ccn-lite/README-OMNeT.txt

# history:
# 2013-04-17 created ({sifalakis.manos,christian.tschudin}@unibas.ch)


Abstract:

    CCN-lite is a reduced and lightweight, but yet
    functionally interoperable implementation of
    the CCNx protocol. It is as a code base for
    class room work, experimental extensions, CCN
    relays running on resource constraint devices,
    and for commercial products.

    This file describes the build process for 
    preparing an omnet archive bundle and how to 
    import it in OMNeT++ v4.2.2 and later.

Table of Contents:

    1) Requirements
    2) How to generate an OMNeT++ archive
    3) How to import/install the code in OMNeT++ ver 4.2.2
    4) Further documentation and code examples

---

1) Requirements

- OMNeT++ v.4.2.2
- INET framework
- MiXiM framework

Obviously you will need a working version of the OMNeT++
simulator. The latest version of OMNeT++ at the time of this
writing is ver 4.3. However, as you will also need the INET
framework, which currently has not been ported for ver 4.3,
we recommend to use ver 4.2.2. The following instructions
are therefore tailored to OMNeT++ ver 4.2.2.

To use CCN-lite with OMNeT++ in any meaningful experiments
you will need at least a version of the INET framework to
provide MAC level and socket level support. This version of
CCN-lite has been tested to work with inet-2.1.0 (latest 
stable ver by April 22, 2013), which can be acquired at

http://inet.omnetpp.org/index.php?n=Main.Download

Last, if you wish to experiment with wireless topologies,
although you can stick to the basic support of INET, we
would strongly recommend to additionally install the MiXiM
framework (and following the respective instructions to
disable the existing wireless code base in INET). The simple
reason for this is the richer more realistic propagation
models provided by MiXiM. This version of CCN-lite has been
tested to work with MiXiM ver 2.2.1, although more recent
versions should also work without problems (so long as the
integration with INET works).

---

2) How to generate an OMNeT++ archive

From the top source CCN-lite directory (where this file is
located) simply run

 $ make ccn-lite-omnet

This should produce in the same directory an OMNeT++ archive
bundle named

 ccn-lite-omnet.tgz

That's it!

---

3) How to import/install the code in OMNeT++ ver 4.2.2

After installing in OMNeT++ the INET framework, follow
exactly the same process to import ccn-lite-omnet. Namely, 

 (Menu) File
   ->Import

    ->General
     ->Existing Projects in Workspace
     
       ->(radio) Select Archive File
        ->(tick) ccn-lite-omnet
          ->Finish

Next proceed to re-index by right-clicking on the newly
imported project at the Project Explorer and selecting 

 Index -> Re-build

Finally, build the ccn-lite-omnet project

 (Menu) Project -> Build Project


__Important__: 

INET needs to be installed and already built before
ccn-lite-omnet can successfully finish the compilation

__Caveats (with OMNeT++ ver 4.2.2)__: 

Those of you already using OMNeT++ extensivelly, will
already know that this version had several issues with
latest features that were added in the Eclipse CDT
environment (such as the debugger nuissances, the bogus
repots of the code analyser, and the crashes when building
before indexing). Those nuissances appear also with the
'ccn-lite-omnet' project of course. However, if you are
patient with pressing NEXT a dozen times at the start of the
Debug mode, if you index before compiling, and if you ignore
the false "semantic errors" that are flagged in the
ccn-lite-omnet.c file, you ll see that everything works like
a charm :). All these nuissances have hopefully dissapeared
again in version 4.3!

---

4) Further documentation and code examples

At this point you re ready to build your own topologies and
experiment with CCN in OMNeT++. To get you started if you re
not too familiar with OMNeT++ and INET, in the in the
ccn-lite project you will find the following

  topologies/   folder containing two simple toy-tologies 
  simulations/  folder with one example scenario that uses
                one of the topologies
  compounds/    folder containing vanilla NED definition for
		a CCN node over Ethernet (used in the
		example scenario), which you can
		extend/modify to work with Wireless MAC, or
                over IP 

More samples to come soon..

Some additional developer's documentation providing an
overview of the software architecture of the OMNeT++ wrapper
of CCN-lite, the current scenario configuration options, the
OMNeT++ message definitions and the function of the various
modules, can be found in the doc/omnet/ directory of the
CCN-lite source tree.

# eof

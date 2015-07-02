# CCN-lite for OMNeT++

## Prerequisites

Follow the [UNIX installation instructions](README-unix.md) to set up the
CCN-lite sources and relevant environment variables.

Additionally, the OMNeT++ bundle of CCN-lite requires both a working version of
the [OMNeT++ simulator](https://omnetpp.org/) as well as an installed and built
version of the [INET Framework](https://inet.omnetpp.org/). Notice that the
bundle has been tested with:
* OMNeT++ 4.5 and
* INET Framework 2.4.0

Although not guaranteed, the bundle should work with newer versions *unless there
has been a major version jump* (INET Framework 3.0.0).

## Installation

1.  Build the CCN-lite OMNeT++ archive bundle by invoking the make target
    `ccn-lite-omnet`:

    ```bash
    cd $CCNL_HOME/src
    make ccn-lite-omnet
    ```

    This target generates an OMNeT++ archive bundle named `ccn-lite-omnet.tgz`
    in the `src` directory.

2.  Import the archive bundle as an existing project into OMNeT++:

    * Open the import dialog with: `File > Import`
    * Import an existing project: `General > Existing Projects into Workspace`
    * Select the previously generated archive file:
      `Select archive file > .../ccn-lite/src/ccn-lite-omnet.tgz`
    * Activate the check box: `☑ ccn-lite-omnet (ccn-lite-omnet)`
    * Import by pressing: `Finish`

3.  Re-index the project by right-clicking on the newly imported project and
    selecting:

    `Index > Rebuild`

4.  Build the project:

    `Project > Build Project`

5.  Run the example simulation `example1`:

    `Project > Run`


## Documentation

### Bundle contents

In the ccn-lite-omnet project you will find the following sub-directories:

| Directory     | Description                                                  |
| :------------ | :------------------------------------------------------------|
| `src`         | Source code base for integrating ccn-lite in omnet. Therein the ccn-lite/ sub-directory contains the parts of the ccn-lite source tree used in omnet. |
| `topologies`  | Contains a couple of simple example topologies.              |
| `compounds`   | Contains a vanilla NED definition for a CCN node over Ethernet (as used in the unit-test simulation scenario). This NED glues together INET functionality with the Ccn module in src. |
| `simulations` | An example simulation scenario for unit-tests and for kickstarting your games with ccn-lite-omnet. Role files for each node in the scenario are provided in the .cfg file included. |


Some additional developer's documentation providing an overview of the software
architecture of the OMNeT++ wrapper of CCN-lite, the current scenario
configuration options, the OMNeT++ message definitions and the function of the
various modules is described in the getting started document found at
`$CCNL_HOME/doc/internal/omnetpp-getting-started.pdf`.


### OMNeT++ code base

In brief the OMNeT++ part consists of:

| Class        | Description                                                   |
| :----------- | :-------------------------------------------------------------|
| `CcnInet`    | Abstracts away the integration with INET.                     |
| `Ccn`        | Implements all the Ccn related functionality and inherits directly from `CcnInet`. |
| `CcnCore`    | Provides a C++ interface to the ccn-lite code and takes care of making it reentrant so that multiple CCN-lite node incarnations can be instantiated in the same OMNeT++ process. The `Ccn` class encapsulates `CcnCore`. |
| `CcnAdmin`   | The "deus ex-machina" of a ccn simulation scenario. It initialises the nodes and topology and makes things you would like to simulate happen "by fate" during the simulation (e.g. changes in the state of nodes, failures, initiation of requests, updates of CSs, etc). This class can be used to compensate for the lack of established routing protocol implementations, cache replacement policies, etc. in a centralised and organised way. |
| `CcnContext` | Wraps CCN packets exchanged between CCN-lite nodes.           |
| `TimerList`  | Used for event services to CCN-lite nodes.                    |


### Additional remarks

The code base of ccn-lite-omnet is far from complete and far from satisfying
everyone's wish list! It may even have bugs that went unotice (if you find one,
don't hesitate to [open up an issue on GitHub](https://github.com/cn-uofbasel/ccn-lite/issues)).
The intend has not to been to provide a service to the ICN community, but rather
to provide a starting point. Having taken care of the boring part of the basic
functionality, you should be able to build on top of this code-base the
functionality you want to have for your experiments or research.

If you feel up to it, why not even re-develop the basic functionality better to
get over its constraints and caveats?

The main starting point for extending the provided functionality would typically
be the `Ccn` class. Either by extending it to integrate transport intelligence,
content store management and so on. Or by writing an application layer that
talks to it and hosts your application logic.

ccn-lite
========

CCN-lite is a lightweight implementation of Name Based Networking,
in particular:

- PARC's Content Centric Networking Protocol
  http://www.ccnx.org/

- the Named-Data Networking project (to come soon)
  http://named-data.net/

- the Named-Function Networking project (to come soon)
  http://named-function.net/

CCN-lite has been included in the RIOT operating system
for the Internet of Things (IoT):
http://www.riot-os.org/

See also the ICN Research Group of the IRTF
https://irtf.org/icnrg

nfn-scala
=========

This project is the Scala environment for NFN enabled CCN-lite. It enables data computation in NFN. An implementation of the compute environment in Scala, an API to interface with NFN, a DSL to program the lambda calculus and other things are part of this project.

#Installation

The main dependencies are Java JDK 7, libssl-dev, and sbt 0.13. The Java classpath JAVA_HOME needs to be set to the corresponding JDK root directory.

##Linux

Setup process for ubuntu.

1. `sudo apt-get install openjdk-7-jdk`
2. `sudo apt-get install libssl-dev`
3. `export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64`
4. Follow [instructions](http://www.scala-sbt.org/0.13.2/docs/Getting-Started/Setup.html) to install sbt
5. `make clean && make all` in the `ccn-lite` root directory

##OSX

JDK 7 should be available, otherwise it can be downloaded directly from oracle. We assume that homebrew is installed.

1. `brew install openssl`
2. `brew install sbt`
3. `export JAVA_HOME="/Library/Java/JavaVirtualMachines/jdk1.7.0_07.jdk/Contents/Home/"`
4. `make clean && make -f BSDmakefile` in the `ccn-lite` root directory

#Running NFN

## Running sbt and compiling the native library
Checkout the branch nfn-scala. Set the `NFN_SCALA` variable with `export NFN_SCALA="<path>/ccn-lite/nfn"` and change to this directory.
Run the `start_sbt.sh` script, this will launch sbt with the linked dynamic library (not yet compiled). To compile this library, run the sbt command `compileJniNativelib`.

## Running the project
There are some runnable projects from our evaluation and for testing. Type `run` in sbt and choose from a list of runnable applications, e.g. `evaluation.PaperExperiment` runs the currently selected experiment (change the experiment to run in source code) from the paper experiments.

#Visualization
To replay and visualize the most recently run NFN program, change to the directory `./omnetreplay`. An installation of [OMNeT++](http://www.omnetpp.org) is required (we used Version 4.4.1, but other versions should work as well). Now you should be able to run the `make.sh` script which compiles and runs everything. From then on the simulation can be directly started with `./omentreplay`.

# Issues
- ccn-lite, ccn-nfn, and nfn-scala are not yet grouped into a single distribution. They are found in different branches and two repositories.
- NACKs branch has some bugs where messages are sent unnecessarily.
- Remove hack in nfn-scala project: it does not handle large content objects according to ccn-lite. Instead of splitting large content objects, the data is dumped into a file on the local filesystem and the path is stored in the content object.
- The question is open if using JNI is a maintainable approach or if the ccn-lite library should be directly used with the cli-commands.
- The project is not yet setup to be easily used in an actual network (`Node` wrapper assumes that everything is local). However, all the functionality (except the hack for large data files) is prepared because communication happens over sockets.

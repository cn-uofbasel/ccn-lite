# CCN-Lite and NFN Tutorial

Table Of Contents:
- [Introduction](#introduction)
- [Scenario 1: Simple Content-lookup](#scenario1)
- [Scenario 2: Content Lookup from NDN Testbed](#scenario2)
- [Scenario 3: Connecting CCNL with NDN Testbed](#scenario3)
- [Scenario 4: Simple NFN request](#scenario4)
- [Scenario 5: NFN request with Compute Server Interaction](#scenario5)
- [Scenario 6: Full Named Function Networking (NFN) demo](#scenario6)
- [Scenario 7: Creating and Publishing your own Named Function](#scenario7)


<a name="introduction"/>
## Introduction

This tutorial explains and demonstrates five scenarios for both Content-Centric-Networking (CCN) as well as Named-Function-Networking (NFN) use cases.

As of 2014, there exist mainly three different CCN implementations and packet formats. a) The original CCNx software and its CCNb encoding, as well as the currently evolving b) Named-Data Networking (NDN) software and c) the new CCNx1.0 implementation.

CCN-Lite is a forth implementation: It is a lightweight and cross-compatible implementation of Content Centric Networking which "speaks" all three dialects. It is written in pure C and (optionally) runs in the Linux kernel space. Moreover, it contains an extension to CCN called Named-Function-Networking, which is also covered in this tutorial.

![alt text](scenario-content-lookup.png)

The first three scenarios of this tutorial demonstrate static content
lookup in a CCN network. We let a client send interests into a network
that consists of heterogeneous CCN implementations (our CCN-lite as
well as the NDN testbed). The interest will be fulfilled by either a
content store within the network or a producer of content. Note that although CCN-lite handles all packet formats, the client has to pick one format and can only access content that is encoded (and made routable) in that chosen format.

Scenario 4 and 5 issue function calls to the network for dynamic
content creation in a NFN network. The chosen setup is shown below
where the NFN-enhanced CCN-Lite router manipulates computation names
and distributes computations, for example to the external compute
environment which is responsible to carry out the actual computations.

![scenario function call](scenario-function-call.png)


<a name="scenario1"/>
## Scenario 1: Simple Content-lookup 

![content-lookup-simple](demo-content-lookup-simple.png)

This scenario consists of a topology of two nodes `A` and `B` each running an instance of the CCN-Lite relay. The cache of relay `B` is populated with some content and a forwarding rule is setup from node `A` to node `B`. Interests are send to node `A`.

### 0. Installing CCN-Lite 

1. `git clone https://github.com/cn-uofbasel/ccn-lite`
2. Set the CCN-Lite env: `export CCNL_HOME="<..>/ccn-lite"` (don't forget to add it to your  bash profile if you want it to persist). 
3. Add the binary directory of ccn-lite to your path `export PATH=$PATH:"<..>/ccn-lite/bin"`
4. Dependencies:
	* Ubuntu: `sudo apt-get install libssl-dev`
	* OSX: `brew install openssl` (assuming the [homebrew](http://brew.sh) packet manager is installed)

5. `make clean all` in `$CCNL_HOME/src`

### 1. Producing Content 

`ccn-lite-mkC` creates an (unsigned) content object in a specified wire format, subject to the maximum packet size of 4kB. CCN-Lite currently supports three wire formats. We use `ndn2013` in the following, `ccnb` and `ccnx2014` are also available. 

```bash
$CCNL_HOME/src/util/ccn-lite-mkC -s ndn2013 "/ndn/test/mycontent" > $CCNL_HOME/test/ndntlv/mycontent.ndntlv
```
Type something, your text will be used as the data for the content object.

### 2. Starting `ccn-lite-relay` for node `A`  

`ccn-lite-relay` is a ccn network router (or forwarder).

`-v` indicates the loglevel. 

`-u` sets the relay to listen on UDP port `9998`.

`-x` sets up a Unix socket, we will use that port later to send management commands. 

```bash
$CCNL_HOME/src/ccn-lite-relay -v 99 -s ndn2013 -u 9998 -x /tmp/mgmt-relay-a.sock
```

### 3. Starting `ccn-lite-relay` for node `B` 

Similar to starting relay `A`, but on a different port. Additional, with `-d` we add all content objects from a directory to the cache of the relay. Currently the relay expects all files to have the file extension `.ndntlv` (and `.ccntlv`/`.ccnb` respectively).
Open a new terminal window for relay `B`.

```bash
$CCNL_HOME/src/ccn-lite-relay -v 99 -s ndn2013 -u 9999 -x /tmp/mgmt-relay-b.sock -d $CCNL_HOME/test/ndntlv
```

### 4. Add a Forwarding Rule
The two relays are not yet connected to each other. We want to add a forwarding rule from node `A` to node `B` which is a mapping of a prefix to an outgoing face. Thus, we first need to create the face on relay `A` followed by defining the forwarding rule for `/ndn`.

`ccn-lite-ctrl` provides a set of management commands to configure and maintain a relay. These management commands are based on a request-reply protocol using interest and content objects. Currently the ctrl tool is hardwired for the `ccnb` format (but the relay still handles packets in all other formats, too). 
To decode the reply of the ctrl tool we use the `ccn-lite-ccnb2xml` tool.

Finally, because faces are identified by dynamically assigned numbers,  we need to extract the FACEID from the reply of the create face command. When defining the forwarding rule we can then refer to this FACEID. 

For creating the face at node `A`, open a new terminal window:
```bash
FACEID=`$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock newUDPface any 127.0.0.1 9999 | $CCNL_HOME/src/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
```
For defining the namespace that should become reachable through this face, we have to configure a forwarding rule. We choose `/ndn` as namespace (prefix) pattern because our test content as well as all objects in `./test/ndntlv` have a name starting with `/ndn`. Later, all interest which match with the longest prefix on this name will be forwarded to this face.

In other words: Relay `A` is technically connected to relay `B` through the UDP face, but logically, relay `A` does not yet have the necessary forwarding state to reach `B`. To create a forwarding rule (`/ndn ---> B`), we execute:
```bash
$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock prefixreg /ndn $FACEID ndn2013 | $CCNL_HOME/src/util/ccn-lite-ccnb2xml 
```

You might want to verify a relay's configuration through the built-in HTTP server. Just point your browser to `http://127.0.0.1:6363/`

This ends the configuration part and we are ready to use the two-node setup for experiments.

### 5. Send Interest for Name `/ndn/test/mycontent/` to `A` 
The `ccn-lite-peek` utility encodes the specified name in a interest with the according suite and sends it to a socket. In this case we want `ccn-lite-peek` to send an interest to relay `A`. Relay `A` will receive the interest, forward it to node `B` which will in turn respond with our initially created content object to relay `A`. Relay `A` sends the content objects back to peek, which prints it to stdout. Here, we pipe the output to `ccn-lite-pktdump` which detects the encoded format (here ndntlv) and prints the wire format-encoded packet in a somehow readable format.
```bash
$CCNL_HOME/src/util/ccn-lite-peek -s ndn2013 -u 127.0.0.1/9998 "/ndn/test/mycontent" | $CCNL_HOME/src/util/ccn-lite-pktdump
```

If you want to see the content proper, use the `-f 2` output format option:
```bash
$CCNL_HOME/src/util/ccn-lite-peek -s ndn2013 -u 127.0.0.1/9998 "/ndn/test/mycontent" | $CCNL_HOME/src/util/ccn-lite-pktdump -f 2
```

<a name="scenario2"/>
## Scenario 2: Content Lookup from NDN Testbed 

![content-lookup-NDNTestbed](demo-content-lookup-NDNTestbed.png)

Similar to Scenario 1, but this time the network consists of the NDN Testbed instead of a set of CCN-Lite relays. 


Peek sends the interest directly to a node in the NDN Testbed. `-w` sets the timeout of peek to 10 seconds.

```bash
$CCNL_HOME/src/util/ccn-lite-peek -s ndn2013 -u 192.43.193.111/6363 -w 10 "/ndn/edu/ucla/ping" | $CCNL_HOME/src/util/ccn-lite-pktdump
```

Note: `/ndn/edu/ucla/ping` dynamically creates a new content packet with a limited lifetime and random name extension. Due to the network level caching, repeating the above command might return a copy instead of triggering a new response. Try it out!

<a name="scenario3"/>
## Scenario 3: Connecting a CCNL Relay to the NDN Testbed 

![content-lookup-CCNL-NDNTestbed](demo-content-lookup-CCNL-NDNTestbed.png)

Scenario 3 combines Scenario 1 and 2 by connecting a (local) CCN-Lite relay to the NDN Testbed and sending interests to it. The relay will forward the interests to the testbed.


### 1. Shutdown relay `B` 
To shutdown a relay we can use the ctrl tool.
```bash
$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-relay-b.sock debug halt 
```

### 2. Remove Face to `B` 
To see the current configuration of the face, use:
```bash
$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock debug dump | $CCNL_HOME/src/util/ccn-lite-ccnb2xml
```
Now we can destroy the face:
```bash
$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock destroyface $FACEID | $CCNL_HOME/src/util/ccn-lite-ccnb2xml 
```
And check again if the face was actually removed.

### 3. Connecting node `A` directly to the NDN Testbed 
Connect to the NDN testbed server of the University of Basel by creating a new UDP face to the NFD of Basel and then registering the prefix `/ndn`
```bash
$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock newUDPface any 192.43.193.111 6363| $CCNL_HOME/src/util/ccn-lite-ccnb2xml
```

```bash
$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock prefixreg /ndn FACEID ndn2013 | $CCNL_HOME/src/util/ccn-lite-ccnb2xml 
```

### 4. Send interest to `A` 
Request data from the Testbed system of the UCLA. The Interest will be transmitted over the Testbed server of the University of Basel to the Testbed system of the UCLA:
```bash
$CCNL_HOME/src/util/ccn-lite-peek -s ndn2013 -u 127.0.0.1/9998 -w 10 "/ndn/edu/ucla" | $CCNL_HOME/src/util/ccn-lite-pktdump
```


<a name="scenario4"/>
## Scenario 4: Simple Named Function Networking (NFN) demo

![](demo-function-call-simple.png)

This scenario consists of a single NFN node `A`. In this demo, we will request the network to execute a simple built-in operation: `add 1 2`. A slightly more complex numeric expression is also shown.

### 1. Start a NFN-relay

To build a CCN-lite relay with NFN functionality, export the variable:
```bash
export USE_NFN=1
```
and rebuild the project in $CCNL_HOME/src with `make`

Or build directly:
```bash
make ccn-nfn-relay
```

The ccn-nfn-relay can be started similar to the ccn-lite-relay:
```bash
$CCNL_HOME/src/ccn-nfn-relay -v 99 -u 9001 -x /tmp/mgmt-nfn-relay-a.sock
```

### 2. Send a NFN request

To send a NFN request, we can use the `ccn-lite-simplenfn` tool instead of `ccn-lite-peek`. This tool is very similar, but instead of fetching the content for a static name it returns the result of a dynamic NFN computation.
```bash
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 "add 1 2" | $CCNL_HOME/src/util/ccn-lite-pktdump -f 3
```
Try out more complex expression evaluations, for example `mult 23 (add 4 456)`.

<a name="scenario5"/>
## Scenario 5: NFN request with Compute Server Interaction 

![](demo-function-call-ext.png)

This scenario explains how to setup an NFN-node which can interact with a Compute Server. 
A compute server is an external application which can execute functions written in a high level programming language.
Instead of running a complex Compute Server, a `dummyserver` is used in this scenario with the sole goal to let a mock function named `/test/data` be made available.

### 1. Start a NFN-relay
A NFN-relay is started on the same way as shown in the previous scenario:
```bash
$CCNL_HOME/src/ccn-nfn-relay -v 99 -u 9001 -x /tmp/mgmt-nfn-relay-a.sock
```

### 2. Start the computation dummy server
The dummy server is written in Python and can only handle the function `/test/data`.
Start it with:
```bash
python $CCNL_HOME/test/scripts/nfn/dummyanswer_ndn.py
```
For more complex functions you have to setup the `nfn-scala` computation environment.



### 3. Add a compute face
In order to interact with the Compute Server which runs on Port 9002 we need to setup a new interface.
```bash
FACEID=`$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock newUDPface any 127.0.0.1 9002 | $CCNL_HOME/src/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
```
And to register the name "COMPUTE" to this interface. This name is reserved in NFN networks for the interaction with a Compute Server:
```bash
$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock prefixreg /COMPUTE $FACEID ndn2013 | $CCNL_HOME/src/util/ccn-lite-ccnb2xml 
```

### 4. Add (the result of) a dummy function to the cache of the relay
The `/test/data` function will mimic computations by returning a name where the result can be found (i.e., it is the constant function always returning the value 10). This result is added to the local cache of the relay where the NFN abstract machines sits:
```bash
$CCNL_HOME/src/util/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock addContentToCache $CCNL_HOME/test/ndntlv/nfn/computation_content.ndntlv | $CCNL_HOME/src/util/ccn-lite-ccnb2xml
```

### 5. Send a request for a function call:
To invoke the function call the user can issue the request:
```bash
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 "call 1 /test/data" | $CCNL_HOME/src/util/ccn-lite-pktdump -f 3
```
The result of the computation is `10`.

One can also combine build in operators and function calls:
```bash
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 "add 1 (call 1 /test/data)" | $CCNL_HOME/src/util/ccn-lite-pktdump -f 3
```
Now the result will be `11`.

<a name="scenario6"/>
## Scenario 6: Full Named Function Networking (NFN) demo
In this scenario we install a full implementation of the compute server instead of just a dummy implementation. The compute server implementation can be found in the project [nfn-scala](https://github.com/cn-uofbasel/nfn-scala). It is written in Scala and requires some installation steps. 

### 0. Installation nfn-scala 
Follow the installation instructions of nfn-scala. You mainly have to make sure that a JDK and sbt is installed.

### 1. Again, start a NFN-relay
Start a nfn-relay. We again add the content you produced in the first scenario.
```bash
$CCNL_HOME/src/ccn-nfn-relay -v 99 -u 9001 -x /tmp/mgmt-nfn-relay-a.sock -d $CCNL_HOME/test/ndntlv
```

### 2. Start the Scala compute server
Go to the nfn-scala directory. Start the compute server with:
```bash
sbt 'runMain runnables.production.ComputeServerStarter --mgmtsocket /tmp/mgmt-nfn-relay-a.sock --ccnl-port 9001 --cs-port 9002 --debug --ccnl-already-running /node/nodeA'
```


The first time this command is run will take a while (it downloads all dependencies as well as scala itself) and compiling the compute server also takes some time.
It runs a compute server on port 9002. There is quite a lot going on when starting the compute server like this. Since the application has the name of the management socket, it is able to setup the required face (a UDP face from the relay on 9001 named `/COMPUTE` to the compute server on 9002). It then publishes some data by injecting it directly into the cache of CCN-Lite. There are two documents named `/node/nodeA/docs/tiny_md` (single content object) and `/node/nodeA/docs/tutorial_md` (several chunks). There are also two named functions (or services) published: `/node/nodeA/nfn_service_WordCount` and `/node/nodaA/nfn_service_Pandoc`. We explain later how they can be used.

### 3. Send a NFN expression with a word count function call
We are going to invoke the word count service. This function takes a variable number of arguments of any type (string, integer, name, another call expression, ...) and returns an integer with the number of words (e.g. `call 3 /ndn/ch/unibas/nfn/nfn_service_WordCount /name/of/doc 'foo bar'`). To invoke this service over NFN we send the following NFN expression to the relay `A`. 
```bash
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 -w 10 "call 2 /node/nodeA/nfn_service_WordCount 'foo bar'" | $CCNL_HOME/src/util/ccn-lite-pktdump
```
The result of this request should be 2. In the following some more examples:
You can also count the number of words of the document you produced in the first scenario, which should have the name `/ndn/test/mycontent`.
```bash
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 -w 10 "call 2 /node/nodeA/nfn_service_WordCount /ndn/test/mycontent" | $CCNL_HOME/src/util/ccn-lite-pktdump
```

Some more examples you can try:

```bash
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 -w 10 "call 2 /node/nodeA/nfn_service_WordCount /node/nodeA/docs/tiny_md" | $CCNL_HOME/src/util/ccn-lite-pktdump
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 -w 10 "call 3 /node/nodeA/nfn_service_WordCount 'foo bar' /node/nodeA/docs/tiny_md" | $CCNL_HOME/src/util/ccn-lite-pktdump
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 -w 10 "add (call 2 /node/nodeA/nfn_service_WordCount 'foo bar') 40" | $CCNL_HOME/src/util/ccn-lite-pktdump
```

### 4. Invoke the pandoc service
First install the [pandoc](http://johnmacfarlane.net/pandoc) command-line utility (Ubuntu: `apt-get install pandoc` OSX: `brew install pandoc`) because the implementation of the pandoc services uses the command-line application. 
This function reformats a document from one format (e.g. markdown github) to another format (e.g. html) using pandoc.
It takes 3 parameters, the first one is the name of a document to transform, and the second is a string indicating the initial document format and the third is a string for the format the document should be converted to. A list of all supported formats can be found on the webpage (e.g. call 4 /ndn/ch/unibas/nfn/nfn_service_Pandoc /name/of/doc 'markdown' 'latex').

To test this we can send the following request:
```bash
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 -w 10 "call 4 /node/nodeA/nfn_service_Pandoc /node/nodeA/docs/tiny_md 'markdown_github' 'html'" | $CCNL_HOME/src/util/ccn-lite-pktdump -f 2
```

Since `tiny_md` is only a small document, the generated html document will also fit into a single content object.

### 5. Invoke the pandoc service with a large document
So far, all results of NFN computations were small and fit into single content objects. Next we test what happens if the result is larger, by transforming this tutorial instead of `tiny_md`.

```bash
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 -w 10 "call 4 /node/nodeA/nfn_service_Pandoc /node/nodeA/docs/tutorial_md 'markdown_github' 'html'" | $CCNL_HOME/src/util/ccn-lite-pktdump -f 2 
```
The result of this computation will not be a document, but something like `redirect:/node/nodeA/call 2 %2fndn%2fch...`. When the result is too large to fit into one content object it has to be chunked. Since chunking of computation does not make too much sense, the result is a redirect address under which the chunked result was published by the compute server. To get the result, copy the redirected address (without `redirect:`). Because `ccn-lite-peek` can only retrieve a single content object, we have to use `ccn-lite-fetch`, which works almost like `ccn-lite-peek`. It will return the stream of the data of the fetched content objects chunks, instead of wire format encoded packets, therefore `ccn-lite-pktdump` is not necessary. The `%2f` is used to escape the `/` character, because otherwise an expression would incorrectly be split into components.
```bash
$CCNL_HOME/src/util/ccn-lite-fetch -s ndn2013 -u 127.0.0.1/9001 "/node/nodeA/call 4 %2fnode%2fnodeA%2fnfn_service_Pandoc %2fnode%2fnodeA%2fdocs%2ftutorial_md 'markdown_github' 'html'" > tutorial.html
```
Open the `tutorial.html` file in the browser. You should see a HTML page of this tutorial (with missing pictures).

### 6. Function chaining
One last example shows the chaining of functions. Too see that this works, use the following:
```bash
$CCNL_HOME/src/util/ccn-lite-simplenfn -s ndn2013 -u 127.0.0.1/9001 -w 10 "call 2 /node/nodeA/nfn_service_WordCount (call 4 /node/nodeA/nfn_service_Pandoc /node/nodeA/docs/tutorial_md 'markdown_github' 'html')" | $CCNL_HOME/src/util/ccn-lite-pktdump
```

<a name="scenario7"/>
## Scenario 7: Creating and Publishing your own Named Function
So far, all services as well as the published documents of the compute server were predefined. In this scenario, we are first going to look at the compute server start script as well as how an implemented service looks like. Then, we are going to implement a new service called revert.

### 1. Explaining runnables.production.StandaloneComputeServer
Currently all running targets exist within the project itself in the runnables package. In `runnables.evaluation.*` are some scripts which setup a nfn environment in Scala-only (without manually starting ccn-lite/nfn instances). In this tutorial we will only discuss `runnables.production.StandaloneComputeServer`.
You might be able to understand what is going on even if you do not know any Scala. First, there is some basic parsing of the command-line arguments. The important part are the following lines:
```scala
// Configuration of the router, so far always ccn-lite
// It requires the socket to the management interface, isCCNOnly = false indicates that it is a NFN node
// and isAlreadyRunning tells the system that it should not have to start ccn-lite
val routerConfig = RouterConfig(config.ccnLiteAddr, config.ccnlPort, config.prefix, config.mgmtSocket.getOrElse("") ,isCCNOnly = false, isAlreadyRunning = config.isCCNLiteAlreadyRunning)


// This configuration sets up the compute server
// withLocalAm = false tells the system that it should not start an abstract machine alongside the compute server
val computeNodeConfig = ComputeNodeConfig("127.0.0.1", config.computeServerPort, config.prefix, withLocalAM = false)

// Abstraction of a node which runs both the router and the compute server on localhost (over UDP sockets)
val node = LocalNode(routerConfig, Some(computeNodeConfig))


// Publish services
// This will internally get the Java bytecode for the compiled services, put them into jar files and
// put the data of the jar into a content object.
// The name of this service is inferred from the package structure of the service as well as the prefix of the local node.
// In this case the prefix is given with the command line argument 'prefixStr' (e.g. /node/nodeA/nfn_service_WordCount)
node.publishServiceLocalPrefix(new WordCount())
node.publishServiceLocalPrefix(new Pandoc())
node.publishServiceLocalPrefix(new PDFLatex())
node.publishServiceLocalPrefix(new Reverse())


// Gets the content of the ccn-lite tutorial
node += PandocTestDocuments.tutorialMd(node.localPrefix)
// Publishes a very small two-line markdown file
node += PandocTestDocuments.tinyMd(node.localPrefix)
```

### 2. Introduction to Service Implementation
First, a simple example of an implemented service (found in `/src/main/scala/nfn/service/Reverse.scala`). Again, even without knowing Scala you should be able to grasp what it does.

```scala
package nfn.service

// ActorRef is the reference to an Akka Actor where you can send messages to
// It is used to have access to the client-library style interface to CCN where you can send interests to and
// receive content from (as well as access to the management interface and more)
// This service will no make use of this
import akka.actor.ActorRef


// NFNService is a trait, which is very similar to a Java interface
// It requires the implementation of one method called 'function'
class Reverse extends NFNService{


  // This method does not have any parameters, you can imagine 'function()'.
  // The return type is a function, which has two parameters, one is a sequence (or list) of NFNValue's and the second
  // is the reference to the actor providing the CCN interface. This function returns a value of type NFNValue.
  override def function(args: Seq[NFNValue], ccnApi: ActorRef): NFNValue = {

    // Match the arguments to the expected or supported types
    // In this case the function is only implemented for a single parameter of type string ('foo bar')
    args match{
      case Seq(NFNStringValue(str)) => 
        
        // Return a result of type NFNValue, in this case a string value
        // NFNValue is a trait with a 'toDataRepresentation', which will be called on the result of the
        // function invocation to get the result to put into the final content object
        NFNStringValue(str.reverse)
        
      // ??? is a Scala construct, it throws a NotImplementedExeption (and is of type Nothing which is a subtype of any other type)
      case _ => ???
    }
  }
}
```

Or a more complete implementation demonstrating how several arguments and different argument types can be handled:

```scala
package nfn.service

import akka.actor.ActorRef

class WordCount() extends NFNService {
  override def function(args: Seq[NFNValue], ccnApi: ActorRef): NFNValue = {
    def splitString(s: String) = s.split(" ").size
    
    NFNIntValue(
      args.map({
        // corresponds to a name or another expression in the call expression, compute server will fetches the data from the network before invoking this function
        case doc: NFNContentObjectValue => splitString(new String(doc.data)) 
        // corresponds to string 'foo bar'
        case NFNStringValue(s) => splitString(s)
        // corresponds to an integer
        case NFNIntValue(i) => 1
        case _ =>
          throw new NFNServiceArgumentException(s"$ccnName can only be applied to values of type NFNBinaryDataValue and not $args")
      }).sum
    )
  }
}
```

### 3. Implementing and publishing a custom service

If you want to use an IDE (both intellij and eclipse have a good scala plugin) you can use the `sbt gen-idea` or `sbt eclipse` to generate a project that can be directly opened with the IDE.

Create a `.scala` file in the `src/main/scala/nfn/service` package, e.g. ToUpper.scala. Implement your service accordingly (as in Java, a Scala String has the function `.toUpperCase`). It is up to you on what types the service is defined and how many arguments the service supports.

To publish this service, simply add the line `node.publishServiceLocalPrefix(new ToUpper()) to the StandaloneComputeServer script. If you used the above mention package, you do not have to import anything. If you choose a different place you need to import the class accordingly.

### 4. Test your service

After rerunning both the ccn-nfn-lite application as well as the compute server, you should be able to call your service. Run the according peek. The name of your service will be /node/nodeA/nfn_service_ToUpper. If you have chosen a different package, replace every '.' of the package name with '_'. If you want you can send us a pull request or an email with the code of your service and we will publish it to the testbed.

### 5. Uninstall sbt and downloaded libraries
The following should get rid of everything downloaded/installed:
* Delete nfn-scala
* uninstall sbt (and remove `~/.sbt` if it still exists)
* Remove `~/.ivy2` (this will of course also delete all your cached Java jars if you are using ivy)

### // end-of-tutorial

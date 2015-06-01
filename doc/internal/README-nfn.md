# Named Function Networking (NFN) by using the Lambda Calculus 

The Lambda Calculus is a formal mathematical language to represent logic. A Term T consists of variables v, abstractions @v T and applications T T, where v is a variable and T a term.

The Lambda Calculus can be evaluated by using an abstract machine. 

Already the pure Lambda Calculus is Turing complete and can represent any computation.

## The Lambda Calculus in the Network
To simplify computation we extended the pure Lambda Calculus to support integer numbers, ICN names, strings and simple operations.
A string is always between two 'string'

The operations are: 

        * add p1 p2
        * sub p1 p2 
        * mult p1 p2
        * ifelse p1 p2 p3

where pX represent the X parameter of the function.

A user can send an interest packet with a computation inside the name and the NFN network will solve the computation. 
Thereby it is possible to use these operations as well as the Lambda Calculus itself.

An interest packet that contains a computation consists of either two or more components. 
The last component is always a marker. This marker tags a packet as NFN packet and by this marker it can be distinguish from a normal ICN packet.
In the component before the NFN marker there is an arbitrary Lambda expression. 
Furthermore it is possible to put an ICN name before the lambda expression.
The entire name can be specified as:

/ \<ICN name components\> / \<Lambda expression\> / NFN marker
where the ICN name can consist of an arbitrary number of components.

## The call operation
The call operation is a special extension to the Lambda calculus.
It enables a user to perform operation outside of the abstract machine.
These operations can be written in any high level programming language.

The syntax of the call operation is defined as: 
        *call X p1 ... pX
where X is the number of the parameters which are expected by the call operation.
The first parameter p1 is always the name of the function which should be invoked.
The following parameters are either a variable, a string or a Lambda expression. 

To enable computations by using the call operation the NFN-scala compute server (https://github.com/cn-uofbasel/nfn-scal://github.com/cn-uofbasel/nfn-scala) can be used. 
A NFN-relay and the NFN compute server communicate over an ICN protocol. To enable this computation a Face with the prefix "/COMPUTE" from the relay to the compute server has to be setup.

To invoke a service, the service has to be published on the network. You will find more to this topic in the documentation of NFN-scala.

It is not possible to interact with data (content objects) from inside the network without using a Compute server.

## Game of Names
NFN is designed to optimize the location where a computation is executed.
To do this NFN handles three different cases:
        * The data are stored in the cache.
        * The computation can be executed on the location where the data are stored.
        * The computation can only be executed on the location where the function is stored (the function cannot be transferred over the network, e.g. for security reason)

Additional only computations on data are forward into the network, since for other computation it usually is faster to compute them locally. 

To forward a computation into the network, the nfn relay chooses a parameter of a call operation and prepends this name by using an abstraction.
To end up with a routable name at the beginning it is required that the relay only considers ICN names for prepending.

The relay will start trying to forward the computation to input data of the call first (input data are ICN names in the parameter list of the call operation)
It starts with the last ICN name. If no result is returned the relay will choose the next input name.
If there is no further ICN name in the parameter list, the relay will try to forward the computation to the function name (also an ICN name).
If this do not return a result, the relay tries to compute the result locally.

Example:

        *NFN call: call 2 /functions/wordcount /data/dataset1

        *Step one: try to route to an input parameter, which is an ICN name. Apply an abstraction to prepend the ICN name: /data/dataset1 (@x call /functions/wordcount x)

        *Step two: if no result if found, try another input parameter. If there is no further input parameter try to route to the function, by prepending the function name: /functions/wordcount (@x call x /data/dataset1)

        *Step three: Try to compute the result locally by sending it to the local compute server (if available): /COMPUTE/call 2 /functions/wordcount /data/dataset1 

The relay expects that the result of an interest is either returned from cache or computed on the fly. 
If a relay receives an interest for a computation it checks if the result can be returned from cache. 
If no cached result is available the relay checks whether the prepended name is local available. If it is the relay will compute the result locally.
Otherwise it will forward the interest according to the FIB entries.

A practical tutorial how to use NFN can be found in the [CCN-Lite Tutorial](../tutorial/tutorial.md)

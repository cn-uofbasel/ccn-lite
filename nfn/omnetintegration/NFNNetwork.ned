
simple Node
{
    parameters:
        @display("i=block/routing");
    gates:
        input in[];
        output out[];

}

simple NFNNode extends Node
{
    parameters:
        @display("i=,cyan");
}

simple ComputeNode extends Node
{
    parameters:
        @display("i=,gold");
}

simple DefaultNode extends Node
{
    parameters:
        @display("i=,gray");
}

network NFNNetwork
{
    submodules:
        docRepo1: NFNNode;
        docRepo3: NFNNode;
        docRepo3compute: ComputeNode;
        docRepo1compute: ComputeNode;
        docRepo4: NFNNode;
        docRepo0compute: ComputeNode;
        docRepo2compute: ComputeNode;
        docRepo2: NFNNode;
        docRepo0: NFNNode;
        docRepo4compute: ComputeNode;
    connections:
        docRepo3.out++ --> { delay = 1ms; } --> docRepo4.in++;
        docRepo3.out++ --> { delay = 1ms; } --> docRepo3compute.in++;
        docRepo3.out++ --> { delay = 1ms; } --> docRepo2.in++;
        docRepo3.out++ --> { delay = 1ms; } --> docRepo0.in++;
        docRepo3.out++ --> { delay = 1ms; } --> docRepo1.in++;
        docRepo0.out++ --> { delay = 1ms; } --> docRepo2.in++;
        docRepo0.out++ --> { delay = 1ms; } --> docRepo3.in++;
        docRepo0.out++ --> { delay = 1ms; } --> docRepo4.in++;
        docRepo0.out++ --> { delay = 1ms; } --> docRepo0compute.in++;
        docRepo0.out++ --> { delay = 1ms; } --> docRepo1.in++;
        docRepo4.out++ --> { delay = 1ms; } --> docRepo1.in++;
        docRepo4.out++ --> { delay = 1ms; } --> docRepo4compute.in++;
        docRepo4.out++ --> { delay = 1ms; } --> docRepo2.in++;
        docRepo4.out++ --> { delay = 1ms; } --> docRepo3.in++;
        docRepo4.out++ --> { delay = 1ms; } --> docRepo0.in++;
        docRepo2.out++ --> { delay = 1ms; } --> docRepo0.in++;
        docRepo2.out++ --> { delay = 1ms; } --> docRepo2compute.in++;
        docRepo2.out++ --> { delay = 1ms; } --> docRepo4.in++;
        docRepo2.out++ --> { delay = 1ms; } --> docRepo1.in++;
        docRepo2.out++ --> { delay = 1ms; } --> docRepo3.in++;
        docRepo1compute.out++ --> { delay = 1ms; } --> docRepo1.in++;
        docRepo2compute.out++ --> { delay = 1ms; } --> docRepo2.in++;
        docRepo0compute.out++ --> { delay = 1ms; } --> docRepo0.in++;
        docRepo1.out++ --> { delay = 1ms; } --> docRepo2.in++;
        docRepo1.out++ --> { delay = 1ms; } --> docRepo0.in++;
        docRepo1.out++ --> { delay = 1ms; } --> docRepo4.in++;
        docRepo1.out++ --> { delay = 1ms; } --> docRepo3.in++;
        docRepo1.out++ --> { delay = 1ms; } --> docRepo1compute.in++;
        docRepo3compute.out++ --> { delay = 1ms; } --> docRepo3.in++;
        docRepo4compute.out++ --> { delay = 1ms; } --> docRepo4.in++;
}

    

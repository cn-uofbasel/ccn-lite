
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

simple CCNNode extends Node
{
    parameters:
       @display("i=,red");
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
        nodenode4compute: ComputeNode;
        nodenode1: NFNNode;
        nodenode3compute: ComputeNode;
        nodenode4: NFNNode;
        nodenode1compute: ComputeNode;
        nodenode3: NFNNode;
        nodenode5: CCNNode;
        nodenode2: CCNNode;
    connections:
        nodenode4compute.out++ --> nodenode4.in++;
        nodenode1compute.out++ --> nodenode1.in++;
        nodenode3.out++ --> nodenode1.in++;
        nodenode3.out++ --> nodenode4.in++;
        nodenode3.out++ --> nodenode3compute.in++;
        nodenode3.out++ --> nodenode5.in++;
        nodenode3compute.out++ --> nodenode3.in++;
        nodenode5.out++ --> nodenode4.in++;
        nodenode5.out++ --> nodenode3.in++;
        nodenode4.out++ --> nodenode3.in++;
        nodenode4.out++ --> nodenode4compute.in++;
        nodenode4.out++ --> nodenode2.in++;
        nodenode4.out++ --> nodenode5.in++;
        nodenode1.out++ --> nodenode3.in++;
        nodenode1.out++ --> nodenode2.in++;
        nodenode1.out++ --> nodenode1compute.in++;
        nodenode2.out++ --> nodenode4.in++;
        nodenode2.out++ --> nodenode1.in++;
}

    

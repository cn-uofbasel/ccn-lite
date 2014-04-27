
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

network NFNNetwork
{
    submodules:
        Some(NFNNode): Some(docRepo4);
        Some(ComputeNode): Some(docRepo1compute);
        Some(ComputeNode): Some(docRepo0compute);
        Some(NFNNode): Some(docRepo1);
        Some(NFNNode): Some(docRepo0);
        Some(ComputeNode): Some(docRepo2compute);
        Some(NFNNode): Some(docRepo3);
        Some(ComputeNode): Some(docRepo3compute);
        Some(NFNNode): Some(docRepo2);
        Some(ComputeNode): Some(docRepo4compute);
    connections:
        Some(ComputeNode).out++ --> { delay = 1ms; } --> Some(NFNNode).in++;
        Some(NFNNode).out++ --> { delay = 1ms; } --> Some(NFNNode).in++;
        Some(NFNNode).out++ --> { delay = 1ms; } --> Some(ComputeNode).in++;
}

    

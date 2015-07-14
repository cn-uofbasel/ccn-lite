# CCN-Lite Testing Support

Testing support is not systematic in CCN-lite, due to the often
experimental character of additions to the code. Moreover, the
multiple extensions (USE_* which can be picked at compile time) make
it almost infeasible to cover all possible combinations (whereof some
are not valid, anyhow).

However, as developers we use the following basic test methods:

* demo scripts, both the standard relay as well as the NFN relay
* few unit tests
* the ccn-lite-simu

## Demo scripts

The most powerful demo script is test/scripts/demo-relay.sh It permits
to select

* which protocol suite should be used to ship across a node-local relay, which can be
* either user space or a Linux kernel module,
* either via UDP or UNIX sockets.

Also check out the test/scripts/demo-relay-fetch.sh if chunking is of
concern to you (same setup and choice as demo-relay-sh).

If NFN testing is sought, see the scripts in test/scripts/nfn

Note that the other subdirectories in test (i.e. ccnb, ccntlv, iottlv, ndntlv) 
hold pre-encoded packets used by the various demo scripts.


## Unit tests

See the test/unit directory and type make, then run the binary. This
is for testing core subroutines (e.g. name matching), but not overall relay
behavior.

## ccn-lite-simu

This is mainly used as a simple in-memory simulation of relay
functionality and for quick memory leaking testing. Note that this
cannot discover memory leaks of management actions and other
extensions as these are never activated during a simulation run.

// eof

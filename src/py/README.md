# CCN-lite Python client library

This directory collects a few ICN applications written in Python. The
underlying library (i.e. Python package) is in the `ccnlite` directory
and currently only supports the `ndn2013` wire format.

This set of utilities will not be expanded and serves mostly as
example.  However, the one and important application is
`ccn-nfn-proxy.py` which serves to publish and execute named functions
written in Python.

Apps in this directory:
- ccn-lite-peek.py
- ccn-lite-pktdump.py
- ccn-nfn-proxy.py

## Smoke test

As a simple test, fire up an echo server (written in C) and fetch a
smoke signal with Python code:

```bash
../ccn-lite-relay -o /the/name/of/the/echo -s ndn2013 -u 9876 &
./ccn-lite-peek.py -u 127.0.0.1/9876 -c /the/name/of/the/echo
```

## The API

Requesting content via Python is simple and done in five lines of
code. Attach to a ICN network and call `getLabeledContent`, or
`getLabeledResult`in the case of named-functions:

```python
import ccnlite.client
nw = ccnlite.client.Access()
nw.connect("127.0.0.1", 9876)
pkts = nw.getLabeledContent("/the/name/of/the/echo", raw=False)
print pkts[0]
```

## Named Function support

Named functions to be published must be placed in a module file in
the `pubfunc` directory. See `pubfunc/myNamedFunctions.py` for an example
and `ccn-lite/test/py/nfnproxy-test.py` for how to invoked these functions.

## TODOs

- fix ccn-lite-pktdump.py to also work for stdin

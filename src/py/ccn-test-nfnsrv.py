#!/usr/bin/python

# demo for invoking named functions ("labeled Results")

import cStringIO
import time

import ccnlite.client
import ccnlite.ndn2013 as ndn

def printIt(pkts):
    if pkts != None and pkts[0] != None:
        name, content = ndn.parseData(cStringIO.StringIO(pkts[0]))
        print content

theICN = ccnlite.client.Access()
theICN.connect("127.0.0.1", 9001)

# async function evaluation
theICN.getLabeledResult("", "add 1 1", callback=printIt)
theICN.getLabeledResult("", "add 2 2", callback=printIt)
theICN.getLabeledResult("", "add 3 3", callback=printIt)
time.sleep(1)

# synchronous function calls
printIt(theICN.getLabeledResult("", "add 1 221"))
printIt(theICN.getLabeledResult("", "add 2 223"))

printIt(theICN.getLabeledResult("/pynfn/hello",
                                "call 2 /myNamedFunctions/getName /test/data"))

printIt(theICN.getLabeledResult("/pynfn/hello",
                                "call 1 /myNamedFunctions/returnHelloWorld"))

printIt(theICN.getLabeledResult("/pynfn/hello",
                                "call 2 /myNamedFunctions/wordcount /test/data"))

# eof

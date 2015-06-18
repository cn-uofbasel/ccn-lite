#!/usr/bin/python

import Queue
import sys
import ccnlite.client

def onData(q, pkt):
    if pkt != None and pkt[0] != None:
        print pkt[0],
    q.put(None)

if len(sys.argv) == 1:
    sys.exit(0)

theICN = ccnlite.client.Access()
theICN.connect("127.0.0.1", 9001)

if False: # blocking call
    data = theICN.getLabeledContent(sys.argv[1], raw=True) # list of pkts
    if data != None and data[0] != None:
        print data[0],
else: # callback
    q = Queue.Queue()
    theICN.getLabeledContent(sys.argv[1], raw=True,
                             callback=lambda data:onData(q, data))
    q.get()

# eof

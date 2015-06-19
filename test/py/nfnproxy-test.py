#!/usr/bin/python

# ccn-lite/test/py/nfnproxy-test.py

# demo for invoking named functions ("labeled Results")
# the user-defined functions are in ccn-lite/src/py/pubfunc/

import cStringIO
import os
import sys
import time

sys.path.append('../../src/py')

import ccnlite.client
import ccnlite.ndn2013 as ndn

# ----------------------------------------------------------------------
''' setup the UNIX environment with two nfn relays (written in C)
    and the Python nfn proxy server
'''

print '*** initializing servers:'

os.system('killall -9 ccn-nfn-relay >/dev/null 2>&1')
os.system('killall -9 ccn-nfn-proxy.py >/dev/null 2>&1')
os.system('rm -f /tmp/mgmt1.sock /tmp/mgmt2.sock')

CCNL='../..'
CCNLPY='../../src/py'
CCNLUT='../../src/util'
CONTENT_FILE = './computation_content.ndntlv'

os.system(CCNL + '/bin/ccn-nfn-relay -s ndn2013 -u 9000 -x /tmp/mgmt1.sock -v debug >/tmp/nfn0.log 2>&1 &')
os.system(CCNL + '/bin/ccn-nfn-relay -s ndn2013 -u 9001 -x /tmp/mgmt2.sock -v debug >/tmp/nfn1.log 2>&1 &')
os.system(CCNLPY + '/ccn-nfn-proxy.py -u 127.0.0.1/9001 9002 >/tmp/nfn2.log 2>&1 &')

#Connect mgmt1 with mgmt2
os.system(CCNLUT + '/ccn-lite-ctrl -x /tmp/mgmt1.sock newUDPface any 127.0.0.1 9001 | ' + CCNLUT + '/ccn-lite-ccnb2xml | grep ACTION')
os.system(CCNLUT + '/ccn-lite-ctrl -x /tmp/mgmt1.sock prefixreg /pynfn 2 | ' + CCNLUT + '/ccn-lite-ccnb2xml | grep ACTION')

#Connect mgmt2 with proxy (=computeserver)
os.system(CCNLUT + '/ccn-lite-ctrl -x /tmp/mgmt2.sock newUDPface any 127.0.0.1 9002 | ' + CCNLUT + '/ccn-lite-ccnb2xml | grep ACTION')
os.system(CCNLUT + '/ccn-lite-ctrl -x /tmp/mgmt2.sock prefixreg /pynfn 2 | ' + CCNLUT + '/ccn-lite-ccnb2xml | grep ACTION')

#Add content to mgmt2, register it
os.system(CCNLUT + '/ccn-lite-ctrl -x /tmp/mgmt2.sock addContentToCache ' + CONTENT_FILE + ' | ' + CCNLUT + '/ccn-lite-ccnb2xml | grep ACTION')
os.system(CCNLUT + '/ccn-lite-ctrl -x /tmp/mgmt1.sock prefixreg /test 2 | ' + CCNLUT + '/ccn-lite-ccnb2xml | grep ACTION')

# ----------------------------------------------------------------------

nw = ccnlite.client.Access()
nw.connect("127.0.0.1", 9001)

# ----------------------------------------------------------------------

def printIt(pkts):
    if pkts != None and pkts[0] != None:
        name, content = ndn.parseData(cStringIO.StringIO(pkts[0]))
        print content

print
print '*** testing NFN support in Python. Expect the following 8 lines:'
print '2'
print '4'
print '6'
print '222'
print '225'
print 'test/data'
print 'Hello World'
print '1'
print '\n*** starting now:'

# ----------------------------------------------------------------------
# test cases: async function evaluation

nw.getLabeledResult("", "add 1 1", callback=printIt) # 2
nw.getLabeledResult("", "add 2 2", callback=printIt) # 4
nw.getLabeledResult("", "add 3 3", callback=printIt) # 6
# wait for the threads to have finished
time.sleep(1)

# ----------------------------------------------------------------------
# test cases: synchronous function calls

printIt(nw.getLabeledResult("", "add 1 221")) # 222
printIt(nw.getLabeledResult("", "add 2 223")) # 225

# ----------------------------------------------------------------------
# test cases: NFN locator and multiple parameters (using synchronous call)

printIt(nw.getLabeledResult("/pynfn/hello",
                            "call 2 /myNamedFunctions/getName /test/data"))

printIt(nw.getLabeledResult("/pynfn/hello",
                            "call 1 /myNamedFunctions/returnHelloWorld"))

printIt(nw.getLabeledResult("/pynfn/hello",
                            "call 2 /myNamedFunctions/wordcount /test/data"))

# ----------------------------------------------------------------------
# cleanup

os.system('killall -9 ccn-nfn-relay >/dev/null 2>&1')
os.system('killall -9 ccn-nfn-proxy.py >/dev/null 2>&1')
os.system('rm -f /tmp/mgmt1.sock /tmp/mgmt2.sock')

# eof

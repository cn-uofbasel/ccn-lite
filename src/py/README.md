# CCN-lite Python client library

preliminary readme file, code is still work-in-progress

a) start two NFN relays, configure them (setup.sh below)
   % ../ccn-nfn-relay -s ndn2013 -u 9000 -x /tmp/mgmt1.sock -t 9000 -v debug
   % ../ccn-nfn-relay -s ndn2013 -u 9001 -x /tmp/mgmt2.sock -t 9001 -v debug
   % ./setup.sh
b) start NFN server/proxy in Python at port 9002
   % ./ccn-nfn-server.py
c) run the demo:
   % ./ccn-test-nfnsrv.py
d) peek and pktdump in Python:
   % ./ccn-lite-peek.py /test/data | ../util/ccn-lite-pktdump
   % ./ccn-lite-peek.py /test/data >/tmp/pkt.bin
   % ./ccn-lite-pktdump.py </tmp/pkt.bin

TODO:
- parse argv to set UDP port, in most files
- fix ccn-lite-pktdump.py to also work for stdin
- add explicit prefix for named-functions (security!)

---

setup.sh:

#Script to set up faces, content and register prefix
#to test the simplesend.py and simpleserve.py programs
#requires two ccn-nfn-relays to run, 
#	one on port 9000 listening to /tmp/mgmt1.sock
#	one on port 9001 listening to /tmp/mgmt2.sock

#util-path directory
# P_UTIL="./../ccn-lite/util"
P_UTIL="../../util"
#directory which contains the example content file computation-content.ndntlv
P_CONTENT="."


#Connect mgmt1 with mgmt2
$P_UTIL/ccn-lite-ctrl -x /tmp/mgmt1.sock newUDPface any 127.0.0.1 9001 | $P_UTIL/ccn-lite-ccnb2xml
$P_UTIL/ccn-lite-ctrl -x /tmp/mgmt1.sock prefixreg /pynfn 2 | $P_UTIL/ccn-lite-ccnb2xml

#Connect mgmt2 with computeserver
$P_UTIL/ccn-lite-ctrl -x /tmp/mgmt2.sock newUDPface any 127.0.0.1 9002 | $P_UTIL/ccn-lite-ccnb2xml
$P_UTIL/ccn-lite-ctrl -x /tmp/mgmt2.sock prefixreg /pynfn 2 | $P_UTIL/ccn-lite-ccnb2xml

#Add content to mgmt2, register it
$P_UTIL/ccn-lite-ctrl -x /tmp/mgmt2.sock addContentToCache $P_CONTENT/computation_content.ndntlv | $P_UTIL/ccn-lite-ccnb2xml
$P_UTIL/ccn-lite-ctrl -x /tmp/mgmt1.sock prefixreg /test 2 | $P_UTIL/ccn-lite-ccnb2xml

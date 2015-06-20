# ccn-lite/src/py/ccnlite/client.py

'''
CCN-lite module for Python:
simple API for client apps

Copyright (C) 2015, Christian Tschudin, University of Basel

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

File history:
2015-06-15 created
'''

import cStringIO
import logging
import Queue
import socket
import threading

import ndn2013 as ndn
import util

logging.basicConfig()

class Access:

    def __init__(self, suite='ndn2013'):
        if suite != 'ndn2013':
            raise Error
        self._suite = suite

    def _peek(self, q, cb, compList, raw, maxchunktime, maxretries):
        # we are lazy and let UDP do the per-request demux (instead of
        # managing a local PIT): each request has its own socket
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.settimeout(maxchunktime)
        req = ndn.mkInterest(compList)
        pkt = None
        for i in range(maxretries):
            s.sendto(req, self._ap)
            try:
                pkt, addr = s.recvfrom(8192)
                break
            except socket.timeout:
                pass
        s.close()
        if pkt and not raw:
              f = cStringIO.StringIO(pkt)
              pkt = ndn.parseData(f)[1]
        if cb != None:
            cb([pkt])
        else:
            q.put(pkt)

    def _getLabeledX(self, compList, callback, maxchunktime, raw):
        if callback:
            a = (None, callback, compList, raw, maxchunktime, 3)
        else:
            q = Queue.Queue()
            a = (q, None, compList, raw, maxchunktime, 3)
        t = threading.Thread(target=self._peek, args=a)
        t.daemon = True
        t.start()
        if callback:
            return None
        pkt = q.get()
        return [pkt]

    def connect(self, ip, port):
        self._ap = (ip, port)

    def getICNsuite(self):
        return self._suite

    def getLabeledContent(self, lci, callback=None, maxchunktime=1.5,
                          raw=False):
        '''
        Implements classic ICN content lookup via name:
          without callback, this function
            returns a list of byte arrays (if raw==False)
            or a list of raw ICN packets (if raw==True)
            or None in case of a timeout
          with callback, the callback is invoked with
            onData(byteArray, moreToCome) if raw==False
            or onData(icnPacket, moreToCome) if raw==True
            or onTimeout() in case of a timeout
        '''
        name = util.str2lci(lci)
        return self._getLabeledX(name, callback, maxchunktime, raw=raw)

    def getLabeledResult(self, nfnlocator, expression, callback=None,
                         maxTotalTime=9000, maxchunktime=1.5):
        '''
	Interests sent to this server may have one of the following
        name formats:
        nfnlocator=|<path1>|...|<pathn>|
	(1) |<path1>|...|<pathn>|call m module.function <data1> ... <datam>|NFN
		where m equals the number of parameters to the function
		and <data1> ... <datam> specify the paths to the needed data objects
	(2) |<path1>|...|<pathn>|<component>|NFN
		where <component> may be
		- a function call without parameters: module.function()
		- a function call taking parameters: module.function(<component>,...<component>)
		- a name of a data object
		which allows composition of functions, e.g. module.functionA(<data1>, module.functionB(<data2>), <data3>)
        '''
        if nfnlocator:
            name = util.str2lci(nfnlocator)
        else:
            name = []
        if expression:
            name.append(expression)
        name.append("NFN");
#        print name
        return self._getLabeledX(name, callback, maxchunktime, raw=True)

# eof
   
    

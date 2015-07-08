#!/usr/bin/python

# ccn-lite/src/py/ccn-lite-peek.py

'''
ICN packet fetch utility (currently for NDN, other formats to follow)

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
2015-06-13 created
'''

import argparse
import cStringIO
import Queue

import ccnlite.client
import ccnlite.ndn2013 as ndn

# ----------------------------------------------------------------------
# process argv

parser = argparse.ArgumentParser(description='Fetch a single ICN packet.')

parser.add_argument('lci', metavar='LCI', type=str,
                    help='a labeled content identifier')
parser.add_argument('-c', action='store_true',
                    help='return packet content instead of raw pkt')
parser.add_argument('-s', metavar='id', type=str,
                    help='a suite identifier (default: ndn2013)',
                    default='ndn2013')
parser.add_argument('-u', metavar='host/port', type=str,
                    help='UDP addr of access router (default: 127.0.0.1/6363)',
                    default='127.0.0.1/6363')
args = parser.parse_args()

# ----------------------------------------------------------------------

if __name__ == "__main__":

    def onData(q, pkts):
        # callback: prints packet/content, unblocks the requestor via queue q
        if pkts != None and pkts[0] != None:
            print pkts[0],
        if q:
            q.put(None)

    # attach to the ICN network
    nw = ccnlite.client.Access()
    udp = args.u.split('/')
    nw.connect(udp[0], int(udp[1]))

    # retrieve content
    q = Queue.Queue()
    nw.getLabeledContent(args.lci, raw=not args.c,
                         callback=lambda data:onData(q, data))
    q.get()

    ''' Alternative: synchronous/blocking call:
    pkts = nw.getLabeledContent(args.lci, raw=not args.c)
    onData(None, pkts)
    '''

# eof

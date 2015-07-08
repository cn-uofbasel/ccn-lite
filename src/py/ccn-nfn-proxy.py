#!/usr/bin/python

# ccn-lite/src/py/ccn-nfn-proxy.py

'''
Instantiates an NFN proxy server which accepts NFN requests: The
requests are partially resolved and executed (specifically the local
named functions).

The named functions to be published must be placed in a module file in
the 'pubfunc' directory. See 'pubfunc/myNamedFunctions.py' for an example,
and ccn-lite/test/py/nfnproxy-test.py for how to invoked these functions.

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

import ccnlite.nfnproxy as proxy

# ----------------------------------------------------------------------
# process argv

parser = argparse.ArgumentParser(description='Process NFN requests')

parser.add_argument('port', metavar='PORT', type=int,
                    help='local UDP port to listen on (default: 3636)',
                    default=3636)
#parser.add_argument('-s', metavar='id', type=str,
#                    help='a suite identifier (default: ndn2013)',
#                    default='ndn2013')
parser.add_argument('-u', metavar='host/port', type=str,
                    help='UDP addr of ICN access router (default: 127.0.0.1/6363)',
                    default='127.0.0.1/6363')
args = parser.parse_args()

# ----------------------------------------------------------------------

if __name__ == "__main__":
    udp = args.u.split('/')
    proxy.NFNproxy(args.port, udp[0], int(udp[1]))

# eof

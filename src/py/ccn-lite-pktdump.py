#!/usr/bin/python

# ccn-lite/src/py/ccn-lite-pktdump.py

'''
ICN packet dump utility (currently for NDN, other formats to follow)

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
import sys

import ccnlite.ndn2013  as ndn
import ccnlite.util     as util

# ----------------------------------------------------------------------
# process argv

parser = argparse.ArgumentParser(description='Dump content of ICN packets (from files or stdin)')

parser.add_argument('fnames', metavar='FILE', type=str, nargs="*",
                    help='file name')
parser.add_argument('-s', metavar='id', type=str,
                    help='a suite identifier (default: auto-detect)',
                    default='ndn2013')
args = parser.parse_args()

# ----------------------------------------------------------------------

if __name__ == "__main__":

    def dumpFile(fn):
        if fn:
            infile = open(fn)
        else:
            fn = 'reading from stdin'
            infile = sys.stdin
        try:
            a = infile.read(1)
            b = infile.read(1)
            if a == '' or b == '':
                return
            infile.seek(-2, 1)
            suite = util.whichSuite(ord(a), ord(b))
            print "\n# %s" % fn
            if suite == None:
                return
            if suite == 'ndn2013':
                print '# packet format is ndn2013\n#'
                ndn.dump(infile, 0, -1)
            else:
                print '# unknown packet format\n#'
                util.hexDump(infile, 1, 1, -1)
        except EOFError:
            pass
        if infile != sys.stdin:
            infile.close()

    print "# ccn-lite-dump.py"

    if len(args.fnames) == 0:
        dumpFile(None)
    else:
        for fn in args.fnames:
            dumpFile(fn)

# eof

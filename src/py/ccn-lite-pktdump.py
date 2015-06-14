#!/usr/bin/python

'''
ICN packet dump utility (NDN, ...)

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

import sys

import ccnlite.ndn
import ccnlite.util

# ----------------------------------------------------------------------

if __name__ == "__main__":
    infile = sys.stdin

    print "# ccn-lite-dump.py"

    fcnt = 1
    try:
        while 1:
            if len(sys.argv) == 1:
                fn = 'reading from stdin'
                infile = sys.stdin
            elif fcnt < len(sys.argv):
                fn = 'file is ' + sys.argv[cnt]
                infile = open(sys.argv[cnt])
                fcnt += 1
            else:
                break

            a = infile.read(1)
            b = infile.read(1)
            if a == '' or b == '':
                raise EOFError
            infile.seek(-2, 1)
            suite = ccnlite.util.whichSuite(ord(a), ord(b))

            print "\n# %s" % fn

            if suite == None:
                break
            if suite == 'ndn2013':
                print '# packet format is ndn2013\n#'
                ccnlite.ndn.ndntlv_dump(infile, 0, -1)
            else:
                print '# unknown packet format\n#'
                ccnlite.util.hexDump(infile, 1, 1, -1)

    except EOFError:
        sys.stdout.flush()

# eof

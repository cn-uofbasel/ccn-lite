# ccn-lite/src/py/ccnlite/util.py

'''
CCN-lite module for Python:
utility procedures

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

from __future__ import print_function

import curses.ascii
import sys

# ----------------------------------------------------------------------

def hexDump(f, lev, doPrint, len):
    cnt = 0
    line = ''
    while len != 0:
        if cnt == 0:
            s = ''
            for i in range(0, lev):
                s = s + '  '
        c = f.read(1)
        if c == '':
            raise EOFError
        s = s + '%02x ' % ord(c)
        if sys.version_info >= (3,):
            if c[0] < 128:
                c = c.decode()
            else:
                c = '.'
        if not curses.ascii.isprint(c):
            c = '.'
        line = line + c
        len -= 1
        cnt += 1
        if cnt == 8:
            s = s + ' '
        elif len == 0 or cnt == 16:
            if doPrint:
                print("%-61s |%s|" % (s, line))
            else:
                print(s)
            cnt = 0
            line = ''
    if cnt != 0:
        if doPrint:
            print("%-61s |%s|" % (s, line))
        else:
            print(s)

def whichSuite(a, b):
    if a == 0x04:
        return 'ccnb'
    if a == 0x01 and b > 0x03:
        return 'ccnb'
    if a == 0x01 and b < 0x04:
        return 'ccnx2015'
    if a == 0x05 or a == 0x06 or a == 0x64:
        return 'ndn2013'
    return None

def str2lci(s):
    return s.split('/')[1:]

# eof

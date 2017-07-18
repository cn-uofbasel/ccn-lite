#!/usr/bin/python

'''
util/ccn-lite-ndnstream.py

A Python script to extract the content (to stdout) of a stream of NDN bytes
(c) 2014 christian.tschudin@unibas.ch

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
2014-11-26: created
'''

import struct
import sys

val2 = struct.Struct('>H')
val4 = struct.Struct('>I')
val8 = struct.Struct('>Q')

def ndntlv_readTorL(f, maxlen):
    if maxlen == 0:
        raise EOFError
    b = f.read(1)
    if b == '':
        raise EOFError
    b = ord(b[0])
    if b < 253:
        if maxlen > 0:
            maxlen = maxlen - 1
        return (b, maxlen)
    if maxlen > 0 and maxlen < 3:
        raise EOFError
    if b == 253:
        if maxlen > 0:
            maxlen = maxlen - 3
        return (val2.unpack(f.read(2)), maxlen)
    if maxlen > 0 and maxlen < 5:
            raise EOFError
    if b == 254:
        if maxlen > 0:
            maxlen = maxlen - 5
        return (val4.unpack(f.read(4)), maxlen)
    if maxlen > 0:
        if maxlen < 9:
            raise EOFError
        maxlen = maxlen - 9
    return (val8.unpack(f.read(8)), maxlen)

def ndntlv_readTL(f, maxlen):
    (t, maxlen) = ndntlv_readTorL(f, maxlen)
    (l, maxlen) = ndntlv_readTorL(f, maxlen)
    return (t, l, maxlen)

def ndntlv_file2content(f, maxlen):
    while maxlen == -1 or maxlen > 0:
        (t, l, maxlen) = ndntlv_readTL(f, maxlen)
        if t in {0x05, 0x06}:
            return ndntlv_file2content(f, l)
        if t == 0x15:
            return f.read(l)
        else:
            f.read(l)
    return ''

if __name__ == "__main__":
    try:
        while 1:
            content = ndntlv_file2content(sys.stdin, -1)
            sys.stdout.write(content)
            sys.stdout.flush()
    except EOFError:
        sys.stdout.flush()
    sys.stderr.write('\ndone.\n')


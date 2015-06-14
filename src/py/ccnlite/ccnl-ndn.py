# ccn-lite/src/py/ccnlite/ccnl-ndn.py

'''
CCN-lite module for Python:
procedures for decoding and encoding NDN-TLV data structures

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

import codecs
import curses.ascii
import struct

# ----------------------------------------------------------------------
# reading NDN TLVs

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
        return (val2.unpack(f.read(2))[0], maxlen)
    if maxlen > 0 and maxlen < 5:
            raise EOFError
    if b == 254:
        if maxlen > 0:
            maxlen = maxlen - 5
        return (val4.unpack(f.read(4))[0], maxlen)
    if maxlen > 0:
        if maxlen < 9:
            raise EOFError
        maxlen = maxlen - 9
    return (val8.unpack(f.read(8))[0], maxlen)

def ndntlv_readTL(f, maxlen):
    (t, maxlen) = ndntlv_readTorL(f, maxlen)
    (l, maxlen) = ndntlv_readTorL(f, maxlen)
    return (t, l, maxlen)

# ----------------------------------------------------------------------

ndntlv_types = {
    0x05 : 'Interest',
    0x06 : 'Data',
    0x07 : 'Name',
    0x08 : 'NameComponent',
    0x09 : 'Selectors',
    0x0a : 'Nonce',
    0x0b : 'Scope',
    0x0c : 'InterestLifeTime',
    0x0d : 'MinSuffixComp',
    0x0e : 'MaxSuffixComp',
    0x0f : 'PublisherPubKeyLoc',
    0x10 : 'Exclude',
    0x11 : 'ChildSelector',
    0x12 : 'MustBeFresh',
    0x13 : 'Any',
    0x14 : 'MetaInfo',
    0x15 : 'Content',
    0x16 : 'SignatureInfo',
    0x17 : 'SignatureValue',
    0x18 : 'ContentType',
    0x19 : 'FreshnessPeriod',
    0x1a : 'FinalBlockId',
    0x1b : 'SignatureType',
    0x1c : 'KeyLocator',
    0x1d : 'KeyLocatorDigest'
    }

ndntlv_recurseSet = { 0x05, 0x06, 0x07, 0x09, 0x16, 0x1c }
ndntlv_isPrint = { 0x08, 0x15 }

def ndntlv_dump(f, lev, maxlen):
    while maxlen == -1 or maxlen > 0:
        s = ''
        for i in range(0, lev):
            s = s + '  '
        (t, l, maxlen) = ndntlv_readTL(f, maxlen)
        if t in ndntlv_types:
            s = s + ndntlv_types[t]
        else:
            s = s + "type%d" % t
        print s + " (%d bytes)" % l
        if t in ndntlv_recurseSet:
            ndntlv_dump(f, lev+1, l)
        elif l > 0:
            ccnl_hexDump(f, lev+1, t in ndntlv_isPrint, l)
        maxlen -= l

# ----------------------------------------------------------------------
# writing NDN TLVs

def ndntlv_TorL(x):
    if x < 253:
        buf = bytearray([x])
    if x < 0x10000:
        buf = bytearray([253, 0, 0])
        val2.pack_into(buf, 1, x)
    elif x < 0x100000000L:
        buf = bytearray([254, 0, 0, 0, 0])
        val4.pack_into(buf, 1, x)
    else:
        buf = bytearray([254, 0, 0, 0, 0, 0, 0, 0, 0])
        val8.pack_into(buf, 1, x)
    return buf

def ndntlv_prependTorL(buf, offset, x):
    if offset < 1:
        raise EOFError
    if x < 253:
        buf[offset-1] = char(x)
        return 1
    if x < 0x10000:
        val2.pack_into(buf, offset-2, x)
        buf[offset-3] = char(253)
        return 3
    if x < 0x100000000L:
        val4.pack_into(buf, offset-4, x)
        buf[offset-5] = char(254)
        return 5
    val8.pack_into(buf, offset-8, x)
    buf[offset-9] = char(255)
    return 9

def ndntlv_prependTL(buf, offset, t, l):
    oldoffset = offset
    offset -= ndntlv_prepentTorL(buf, offset, l)
    offset -= ndntlv_prepentTorL(buf, offset, t)
    return oldoffset - offset

def ndntlv_prependBlob(buf, offset, t, blob):
    oldoffset = offset
    offset -= len(blob)
    buf[offset:] = blob
    offset -= ndntlv_prependTL(buf, offset, t, len)
    return oldoffset - offset

# ----------------------------------------------------------------------
# creating NDN packets

# def ndntlv_mkI(name):

# def ndntlv_mkC(name, blob):
  

# eof

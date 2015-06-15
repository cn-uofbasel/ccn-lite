# ccn-lite/src/py/ccnlite/ndn2013.py

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
import struct

import util

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

ndntlv_recurseSet = { 0x05, 0x06, 0x07, 0x09, 0x14, 0x16, 0x1c }
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
            util.hexDump(f, lev+1, t in ndntlv_isPrint, l)
        maxlen -= l

def ndntlv_isInterest(f):
    c = f.read(1);
    f.seek(-1, 1)
    if c == '':
        raise EOFError
    return ord(c) == 0x05

def ndntlv_isData(f):
    c = f.read(1);
    f.seek(-1, 1)
    if c == '':
        raise EOFError
    return ord(c) == 0x06

def ndntlv_parseName(f, maxlen):
    name = []
    while maxlen > 0:
        (t, l, maxlen) = ndntlv_readTL(f, maxlen)
        if t != 0x08:
            raise EOFError
        name.append(f.read(l))
        maxlen -= l
    return name

def ndntlv_parseInterest(f):
    (t, l, maxlen) = ndntlv_readTL(f, -1)
    if t != 0x05:
        raise EOFError
    while maxlen == -1 or maxlen > 0:
        (t, l, maxlen) = ndntlv_readTL(f, -1)
        if t == 0x07:
            return ndntlv_parseName(f, l)
        f.read(l)
    return None

def ndntlv_parseData(f):
    (t, l, maxlen) = ndntlv_readTL(f, -1)
    if t != 0x06:
        raise EOFError
    name = None
    cont = None
    while maxlen == -1 or maxlen > 0:
        (t, l, maxlen) = ndntlv_readTL(f, -1)
        if t == 0x07:
            name = ndntlv_parseName(f, l)
        if t == 0x15:
            cont = f.read(l)
        f.read(l)
    return (name, cont)

# ----------------------------------------------------------------------
# creating NDN TLVs

def ndntlv_mkTorL(x):
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

def ndntlv_mkName(name):
    n = ''
    for i in range(0, len(name)):
        c = ndntlv_mkTorL(0x08) + ndntlv_mkTorL(len(name[i])) + name[i]
        n = n + c
    n = ndntlv_mkTorL(0x07) + ndntlv_mkTorL(len(n)) + n
    return n

def ndntlv_mkInterest(name):
    n = ndntlv_mkName(name);
    return ndntlv_mkTorL(0x05) + ndntlv_mkTorL(len(n)) + n

def ndntlv_mkData(name, blob):
    n = ndntlv_mkName(name);
    nb = n + ndntlv_mkTorL(0x15) + ndntlv_mkTorL(len(blob)) + blob
    return ndntlv_mkTorL(0x06) + ndntlv_mkTorL(len(nb)) + nb
  

# eof

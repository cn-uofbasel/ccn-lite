# README-iottlv.md (part of CCN-lite)

[See also the README on multi-protocol support](README-multiprotocol.md)

## Draft IOT'2014 Encoding Rules

christian.tschudin@unibas.ch, Nov 11, 2014

## 1) Introduction

In 2013, the ICN projects CCNx and NDN have switched to a TLV
encoding.  TLV encoded messages contain numerical values for type and
length with ranges that vary widely. But often, the values are small
or can be made small. Having a compact 1-Byte encoding for these
common cases is highly desirable for IoT devices e.g. sensors with
radios only supporting PDUs as small as 100 Bytes.

## 2) TL encoding

For efficiency, we define a 1-Byte combined Type+Length encoding for
small type and length values T and L. If necessary (type and/or length
too large), a variable-length encoding is used: The first byte is set
to 0, followed by the independently encoded values of T and L. Here, we
show the *encoding* rules:


```
If Type_Val < 4 &&
   Length_Val < 64 &&
   (Type_Val|Length_Val) != 80 &&  // this is the "switch encoding" magic byte
   (Type_Val|Length_Val) != 0      // this forces independent type and len enc
then

  TYPE-LENGTH-PREFIX := BYTE

        1 Byte
  +-+-+-+-+-+-+-+-+-+
  |Typ| Length      |
  +-+-+-+-+-+-+-+-+-+

else

  TYPE-LENGTH-PREFIX := BYTE(0x00) VAR-NUMBER(type) VAR-NUMBER(length)

  VAR-NUMBER := BYTE+

  The first 0-octet signals that a variable-length encoding is required.
  The encoding scheme of type and value is the one used in NDN'2013:

    if the first octet is < 253, the number is encoded in that octet
 
    if the first octet == 253, the number is encoded in the following
    2 octets, in net byte-order

    if the first octet == 254, the number is encoded in the following
    4 octets, in net byte-order
 
    if the first octet == 255, the number is encoded in the following
    8 octets, in net byte-order

fi
```

## 3) Assigned type values:

In order to keep the bits requirements for type values low, we choose
contextual type values as in CCNX'2014. The following type value
assignment is proposed.

```
Top level:
  0 reserved
  1 reserved
  2 Request (aka Interest)
  3 Reply (aka Data/Content)

Request or Reply:
  0 Optional Headers
  1 Name
  2 Payload
  3 Validation

Optional Headers:
  0 TTL
  1 Exclusions

Exclusions:
  0 KeyID
  1 ContentObjectHash

Name:
  0 PathName
  1 FlatLabel
  2 NamedFunction

PathName:
  1 NameComponent // do not assign value 0: we need to encode empty components

Payload:
  0 opt Metadata
  1 Data

Validation: (covers Name and Payload entries of a Reply or Request msg)
  0 Validation Algo Type
  1 Validation Data
```

In tree form:

```
  iot2014 type values
   `-- 0 reserved
   `-- 1 reserved
   `-- 2 Request (aka Interest)
       `-- 0 Optional Headers
           `-- 0 TTL
           `-- 1 Exclusions
               `-- 0 KeyID
               `-- 1 ContentObjectHash
       `-- 1 Name
           `-- 0 PathName
               `-- 1 NameComponent // not 0: we need to encode empty components
           `-- 1 FlatLabel
           `-- 2 NamedFunction
       `-- 2 Payload
           `-- 0 opt Metadata
           `-- 1 Data
       `-- 3 Validation (covers Name and Payload of a Reply or Request msg)
           `-- 0 Validation Algo Type
           `-- 1 Validation Data
   `-- 3 Reply (aka Data/Content)
       `-- (see Request)
```

## 4) Benchmarks

Assume an Interest message for */iot/hello/world/with/a/long/path*
which we encode according to different schemas. The resulting size
table:
```
 Bytes  Encoding

    39  iot2014
    52  ndn2013
    60  ccnb
    70  ccnx2014
```


See the Appendix for this example, including the reported packet
sizes, for the four different encodings ccnb, ndn2013, ccnx2014 and
iot2014.


## 5) Discussion, pending issues:

The example shows an impressive gain, but name space engineering
remains much more important.

As future work, we envisage to:

- introduce codes to switch the (subsequent, or lower-level) encoding? [See also the README on multi-protocol support for a first take](README-multiprotocol.md),

- (re-)define type values on the fly, that is to bind them to some "context ID" which eventually will result in header compression.


## Appendix

Packet dumps for an interest for */iot/hello/world/with/a/long/path*,
in four different encodings (in chronological order of packet format
definition):

```
% ccn-lite-mkI -s ccnb /iot/hello/world/with/a/long/path | ccn-lite-pktdump
# ccn-lite-pktdump, parsing 60 bytes
#   auto-detected CCNB format
#
0000  01 d2  -- <interest>
0002    f2  -- <name>
0003      fa  -- <component>
0004        9d  -- <data (3 bytes)>
0005          69 6f 74                                               |iot|
0008        00  -- </component>
0009      fa  -- <component>
000a        ad  -- <data (5 bytes)>
000b          68 65 6c 6c 6f                                         |hello|
0010        00  -- </component>
0011      fa  -- <component>
0012        ad  -- <data (5 bytes)>
0013          77 6f 72 6c 64                                         |world|
0018        00  -- </component>
0019      fa  -- <component>
001a        a5  -- <data (4 bytes)>
001b          77 69 74 68                                            |with|
001f        00  -- </component>
0020      fa  -- <component>
0021        8d  -- <data (1 byte)>
0022          61                                                     |a|
0023        00  -- </component>
0024      fa  -- <component>
0025        a5  -- <data (4 bytes)>
0026          6c 6f 6e 67                                            |long|
002a        00  -- </component>
002b      fa  -- <component>
002c        a5  -- <data (4 bytes)>
002d          70 61 74 68                                            |path|
0031        00  -- </component>
0032      00  -- </name>
0033    02 ca  -- <nonce>
0035      a6  -- <data (4 bytes)>
0036        ce 8e 62 54                                              |..bT|
003a      00  -- </nonce>
003b    00  -- </interest>
003c  pkt.end
```

```
% ccn-lite-mkI -s ndn2013 /iot/hello/world/with/a/long/path | ccn-lite-pktdump
# ccn-lite-pktdump, parsing 52 bytes
#   auto-detected NDN TLV format (as of Mar 2014)
#
0000  05 32 -- <Interest, len=50>
0002    07 28 -- <Name, len=40>
0004      08 03 -- <NameComponent, len=3>
0006        69 6f 74                                                 |iot|
0009      08 05 -- <NameComponent, len=5>
000b        68 65 6c 6c 6f                                           |hello|
0010      08 05 -- <NameComponent, len=5>
0012        77 6f 72 6c 64                                           |world|
0017      08 04 -- <NameComponent, len=4>
0019        77 69 74 68                                              |with|
001d      08 01 -- <NameComponent, len=1>
001f        61                                                       |a|
0020      08 04 -- <NameComponent, len=4>
0022        6c 6f 6e 67                                              |long|
0026      08 04 -- <NameComponent, len=4>
0028        70 61 74 68                                              |path|
002c    09 02 -- <Selectors, len=2>
002e      12 00 -- <MustBeFresh, len=0>
0030    0c 02 -- <InterestLifetime, len=2>
0032      0f a0                                                      |..|
0034  pkt.end
```

```
% ccn-lite-mkI -s ccnx2014 /iot/hello/world/with/a/long/path | ccn-lite-pktdump
# ccn-lite-pktdump, parsing 70 bytes
#   auto-detected CCNx TLV format (as of Sept 2014)
#
0000  hdr.vers=0
0001  hdr.mtyp=0x01 (Interest\toplevelCtx)
0002  hdr.paylen=62
0006  hdr.hdrlen=8
0008  hdr.end
0008  00 01 00 3a -- <Interest\toplevelCtx, len=58>
000c    00 00 00 36 -- <Name\globalCtx, len=54>
0010      00 01 00 03 -- <NameSegment\nameCtx, len=3>
0014        69 6f 74                                                 |iot|
0017      00 01 00 05 -- <NameSegment\nameCtx, len=5>
001b        68 65 6c 6c 6f                                           |hello|
0020      00 01 00 05 -- <NameSegment\nameCtx, len=5>
0024        77 6f 72 6c 64                                           |world|
0029      00 01 00 04 -- <NameSegment\nameCtx, len=4>
002d        77 69 74 68                                              |with|
0031      00 01 00 01 -- <NameSegment\nameCtx, len=1>
0035        61                                                       |a|
0036      00 01 00 04 -- <NameSegment\nameCtx, len=4>
003a        6c 6f 6e 67                                              |long|
003e      00 01 00 04 -- <NameSegment\nameCtx, len=4>
0042        70 61 74 68                                              |path|
0046  pkt.end
```

```
% ccn-lite-mkI -s iot2014 /iot/hello/world/with/a/long/path | ccn-lite-pktdump
# ccn-lite-pktdump, parsing 39 bytes
#   auto-detected IOT TLV format (as of Nov 2014)
#
0000  a6 -- <Request\toplevelCtx, len=38>
0001    02 -- <Header\ReqRepCtx, len=2>
0002      01 -- <TimeToLive\HeaderCtx, len=1>
0003        10                                                       |.|
0004    62 -- <Name\ReqRepCtx, len=34>
0005      21 -- <PathName\NameCtx, len=33>
0006        43 -- <Component\PathNameCtx, len=3>
0007          69 6f 74                                               |iot|
000a        45 -- <Component\PathNameCtx, len=5>
000b          68 65 6c 6c 6f                                         |hello|
0010        45 -- <Component\PathNameCtx, len=5>
0011          77 6f 72 6c 64                                         |world|
0016        44 -- <Component\PathNameCtx, len=4>
0017          77 69 74 68                                            |with|
001b        41 -- <Component\PathNameCtx, len=1>
001c          61                                                     |a|
001d        44 -- <Component\PathNameCtx, len=4>
001e          6c 6f 6e 67                                            |long|
0022        44 -- <Component\PathNameCtx, len=4>
0023          70 61 74 68                                            |path|
0027  pkt.end
```

// eof

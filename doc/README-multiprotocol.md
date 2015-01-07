# README-multiprotocol.md (part of CCN-lite)

## Draft Encoding Switching Mechanism for ICN packet formats

christian.tschudin@unibas.ch, Dec 22, 2014

## 1) Introduction

CCN-lite is more than a platform for experimenting with different ICN
protocols: it has an active agenda to use Named-Function-Networking as
universal ICN glue.

This means that a CCN-lite relay, listing on a given UDP port, is
potentially confronted with packets encoded in different schemas like
ccnb, CCNx2014 or NDN2013. In the past it was possible to discriminate
among these packets because luckily enough the "type values" were
sparsely populated and the programmers had chosen code points that do
not overlap. But with the compact IOT2014 format, there is no sparsity
anymore. Instead of letting the relay do clever guess and check work
for deducing a packet's format, we suggest an explicit way of
announcing a packet's encoding.

## 2) Switching Encoding Style

We state as a first requirement that encoding can be switched not only
at the outer level, but potentially also inside an ICN packet. While
the later can be handled through conventions (e.g. using a switching
mechanism *inside* the opaque payload field), this is not possible at
the outer level: The very first bytes have to act like magic bytes.

For the second requirement, we assume that a "frame" (e.g. Ethernet,
UDP datagram) is used to carry multiple ICN packets which are placed
back-to-back. Ideally, one "switch" would apply for the whole frame
i.e., all ICN packets would be of the same sort. However, we wish to
also switch the packet format inside a frame.

The third requirement is that code switching is done in the most
byte-saving way possible. We therefore abandoned the path of using a
TLV schema for doing the encoding and opt instead for a switch
statement that carries only the switch signal while leaving the length
encoding to the packets themselves.

This results in the following frame layout (example):

```
 Byte 0 ...                                                         Byte N-1
+---------------------------------------------------------------------------+
| Enc(rpc) | rpc-packet | Enc(ccnx2014) | ccnx2014-packet | ccnx2014-packet |
+---------------------------------------------------------------------------+

where Enc(fmt) stands for the byte sequence to signal a switch to the
encoding according to packet format "fmt".
```

## 3) Assigned numbers

A 1+N byte schema is used to represent a switch signal, where N depends on the
number that has been given to a packet format.
```
             +------+---...+------+
Enc(fmt) :== | 0x80 | VARINT(fmt) |
             +------+---...+------+

  The VARINT representation for the fmt value is according to NDN2013:

    if the first octet is < 253, the format is encoded in that octet
 
    if the first octet == 253, the format is encoded in the following
    2 octets, in net byte-order

    if the first octet == 254, the format is encoded in the following
    4 octets, in net byte-order
 
    if the first octet == 255, the format is encoded in the following
    8 octets, in net byte-order

```

The following format numbers have been assigned so far:
```
  0  CCNL_ENC_CCNB
  1  CCNL_ENC_NDN2013
  2  CCNL_ENC_CCNX2014
  3  CCNL_ENC_IOT2014
  4  CCNL_ENC_LOCALRPC
```

## 4) Backwards compatibility

The switch value 0x80 has been chosen such that it does not collide
with any of the pre-existing packet formats CCNB, NDN2013 and
CCNX2014. It is suggested that these protocol suites leave out any
type values that would lead to a starting 0x80 byte.

In this way, the new Encoding-Switch style can be run in parallel and
as a transition: existing code (not understanding packets starting
with 0x80 byte) will drop them; new code can accept both plain CCNB,
NDN2013 and CCNX2014 packets as well as serving the switch signal.

// eof

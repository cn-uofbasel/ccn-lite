/*
 * @f ccnl-pkt-switch.c
 * @b CCN lite - encoding for switching packet formats
 *
 * Copyright (C) 2014, Christian Tschudin, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2014-12-21 created
 */

// see ccnl-defs.h for the ENC constants

int
ccnl_switch_dehead(unsigned char **buf, int *len, int *code)
{
    if (*len < 2 || **buf != 0x80)
        return -1;
    if ((*buf)[1] < 253) {
        *code = (*buf)[1];
        *buf += 2;
        *len -= 2;
        return 0;
    }
    // higher code values not implemented yet
    return -1;
}

int
ccnl_enc2suite(int enc)
{
    switch(enc) {
#ifdef USE_SUITE_CCNB
    case CCNL_ENC_CCNB:      return CCNL_SUITE_CCNB;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_ENC_NDN2013:   return CCNL_SUITE_NDNTLV;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_ENC_CCNX2014:  return CCNL_SUITE_CCNTLV;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_ENC_CISCO2015: return CCNL_SUITE_CISTLV;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_ENC_IOT2014:   return CCNL_SUITE_IOTTLV;
#endif
#ifdef USE_SUITE_LOCALRPC
    case CCNL_ENC_LOCALRPC:  return CCNL_SUITE_LOCALRPC;
#endif
    default:
        break;
    }

    return -1;
}

// ----------------------------------------------------------------------

#ifdef NEEDS_PACKET_CRAFTING

int
ccnl_switch_prependCodeVal(unsigned long val, int *offset, unsigned char *buf)
{
    int len, i, t;

    if (val < 253)
        len = 0, t = val;
    else if (val <= 0xffff)
        len = 2, t = 253;
    else if (val <= 0xffffffffL)
        len = 4, t = 254;
    else
        len = 8, t = 255;
    if (*offset < (len+1))
        return -1;

    for (i = 0; i < len; i++) {
        buf[--(*offset)] = val & 0xff;
        val = val >> 8;
    }
    buf[--(*offset)] = t;
    return len + 1;
}

int
ccnl_switch_prependCoding(unsigned int code, int *offset, unsigned char *buf)
{
    int len;

    if (*offset < 2)
        return -1;
    len = ccnl_switch_prependCodeVal(code, offset, buf);
    if (len < 0 || *offset < 1)
        return -1;
    *offset -= 1;
    buf[*offset] = 0x80;

    return len+1;
}

#endif // NEEDS_PACKET_CRAFTING

// eof

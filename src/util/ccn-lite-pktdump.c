/*
 * @f util/ccn-lite-pktdump.c
 * @b CCN lite - dumps CCNB, CCN-TLV and NDN-TLV encoded packets
 *               as well as RPC data structures
 *
 * Copyright (C) 2014-15, Christian Tschudin, University of Basel
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
 * 2014-03-29 created (merging three old dump programs)
 *
 */

#define USE_FRAG

#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_CISTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_LOCALRPC
#define USE_SUITE_NDNTLV

#define NEEDS_PACKET_CRAFTING // for IOTTLV

#include "ccnl-common.c"

// ----------------------------------------------------------------------

void
indent(char *s, int lev)
{
    if (s)
        printf("%s", s);
    while (lev-- >= 0)
        printf("  ");
}

void
hexdump(int lev, unsigned char *base, unsigned char *cp, int len,
        int rawxml, FILE* out)
{
    int i, maxi, lastlen=-1;
    unsigned char cmp[8], star = 0;

    while (len > 0) {
        maxi = len > 8 ? 8 : len;

        if (!rawxml) {
            if (maxi == 8) {
                if (lastlen == 8 && !memcmp(cmp, cp, 8)) {
                    if (!star) {
                        fprintf(out, "*\n");
                        star = 1;
                    }
                    cp += 8;
                    len -= 8;
                    continue;
                }
                lastlen = 8;
                memcpy(cmp, cp, 8);
                star = 0;
            }
            fprintf(out, "%04zx  ", cp - base);
        }

        indent(NULL, lev);
        for (i = 0; i < 8; i++){
            if (i < maxi)
                    fprintf(out, "%02x ", cp[i]);
            else
                    fprintf(out, "   ");
        }
        if (!rawxml) {
            for (i = 79 - 6 - 2*(lev+1) - 8*3 - 12; i > 0; i--)
                fprintf(out, " ");
            fprintf(out, "  |");
            for (i = 0; i < maxi; i++, cp++)
                fprintf(out, "%c", isprint(*cp) ? *cp : '.');
            fprintf(out, "|\n");
        } else {
            fprintf(out, "\n");
        }

        len -= maxi;
    }
}

void
base64dump(int lev, unsigned char *base, unsigned char *cp, int len, int rawxml, FILE* out) {
    int encodedLen = -1;
    int i;
    for(i = 0; i < lev + 1; i++) {
        fprintf(out, "  ");
    }
    fprintf(out, "%s\n", base64_encode((char*)cp, len, (size_t*)&encodedLen));
    cp += len;
    len = 0;
}

// ----------------------------------------------------------------------
// CCNB

int
ccnb_deheadAndPrint(int lev, unsigned char *base, unsigned char **buf,
            int *len, int *num, int *typ, int rawxml, FILE* out)
{
    int i, val = 0;

    if (*len <= 0) {
        return -1;
    }

    if (!rawxml) {
        fprintf(out, "%04zx  ", *buf - base);
    }

    for (i = 0; i < lev; i++) {
        fprintf(out, "  ");
    }
    if (**buf == 0) {
        if (!rawxml) {
            fprintf(out, "00 ");
        }
        *num = *typ = 0;
        *buf += 1;
        *len -= 1;
        return 0;
    }
    for (i = 0; i < (int)sizeof(i) && i < *len; i++) {
        unsigned char c = (*buf)[i];
        if (!rawxml) {
            fprintf(out, "%02x ", c);
        }
        if ( c & 0x80 ) {
            *num = (val << 4) | ((c >> 3) & 0xf);
            *typ = c & 0x7;
            *buf += i+1;
            *len -= i+1;
            return 0;
        }
        val = (val << 7) | c;
    }
    if (!rawxml) {
        fprintf(out, "?decoding problem?\n");
    }
    return -1;
}

static int
ccnb_must_recurse(/* work in progress */)
{
    return 0;
}

static char*
ccnb_dtag2name(int num)
{
    char *n;

    switch (num) {
    case CCN_DTAG_ANY:           n = "any"; break;
    case CCN_DTAG_NAME:          n = "name"; break;
    case CCN_DTAG_COMPONENT:     n = "component"; break;
    case CCN_DTAG_CONTENT:       n = "content"; break;
    case CCN_DTAG_SIGNEDINFO:    n = "signedinfo"; break;
    case CCN_DTAG_INTEREST:      n = "interest"; break;
    case CCN_DTAG_KEY:           n = "key"; break;
    case CCN_DTAG_KEYLOCATOR:    n = "keylocator"; break;
    case CCN_DTAG_KEYNAME:       n = "keyname"; break;
    case CCN_DTAG_SIGNATURE:     n = "signature"; break;
    case CCN_DTAG_TIMESTAMP:     n = "timestamp"; break;
    case CCN_DTAG_TYPE:          n = "type"; break;
    case CCN_DTAG_NONCE:         n = "nonce"; break;
    case CCN_DTAG_SCOPE:         n = "scope"; break;
    case CCN_DTAG_WITNESS:       n = "witness"; break;
    case CCN_DTAG_SIGNATUREBITS: n = "signaturebits"; break;
    case CCN_DTAG_FRESHNESS:     n = "freshnessSeconds"; break;
    case CCN_DTAG_FINALBLOCKID:  n = "finalblockid"; break;
    case CCN_DTAG_PUBPUBKDIGEST: n = "publisherPubKeyDigest"; break;
    case CCN_DTAG_PUBCERTDIGEST: n = "publisherCertDigest"; break;
    case CCN_DTAG_CONTENTOBJ:    n = "contentobj"; break;
    case CCN_DTAG_ACTION:        n = "action"; break;
    case CCN_DTAG_FACEID:        n = "faceID"; break;
    case CCN_DTAG_IPPROTO:       n = "ipProto"; break;
    case CCN_DTAG_HOST:          n = "host"; break;
    case CCN_DTAG_PORT:          n = "port"; break;
    case CCN_DTAG_FWDINGFLAGS:   n = "forwardingFlags"; break;
    case CCN_DTAG_FACEINSTANCE:  n = "faceInstance"; break;
    case CCN_DTAG_FWDINGENTRY:   n = "forwardingEntry"; break;
    case CCN_DTAG_MINSUFFCOMP:   n = "minSuffixComponents"; break;
    case CCN_DTAG_MAXSUFFCOMP:   n = "maxSuffixComponents"; break;
    case CCN_DTAG_SEQNO:         n = "sequenceNumber"; break;
    case CCN_DTAG_FragA:         n = "FragA (Type)"; break;  // frag Type
    case CCN_DTAG_FragB:         n = "FragB (SeqNr)"; break; // frag SeqNr
    case CCN_DTAG_FragC:         n = "FragC (Flag)"; break;  // frag h2h Flag
    case CCN_DTAG_FragD:         n = "FragD"; break;
    case CCN_DTAG_FragP:         n = "FragP (Fragment)"; break; // frag
    case CCN_DTAG_CCNPDU:        n = "ccnProtocolDataUnit"; break;

/*
    case CCNL_DTAG_MACSRC:   n = "MACsrc"; break;
    case CCNL_DTAG_IP4SRC:   n = "IP4src"; break;
    case CCNL_DTAG_FRAG:     n = "fragmentation"; break;
    case CCNL_DTAG_FACEFLAGS:    n = "faceFlags"; break;
    case CCNL_DTAG_DEBUGREQUEST: n = "debugRequest"; break;
    case CCNL_DTAG_DEBUGACTION:  n = "debugAction"; break;

    case CCNL_DTAG_FRAG2012_OLOSS:   n = "fragmentOurLoss"; break;
    case CCNL_DTAG_FRAG2012_YSEQN:   n = "fragmentYourSeqNo"; break;
*/

    default:
    n = NULL;
    }
    return n;
}

static int
ccnb_parse_lev(int lev, unsigned char *base, unsigned char **buf,
           int *len, char *cur_tag, int rawxml, FILE* out)
{
    int num, typ = -1;
    char *next_tag;

    while (ccnb_deheadAndPrint(lev, base, buf, len, &num, &typ, rawxml, out) == 0) {
        switch (typ) {
        case CCN_TT_BLOB:
        case CCN_TT_UDATA:
            if (rawxml) {
                fprintf(out, "<data ");
                fprintf(out, "size=\"%i\" dt=\"binary.base64\"", num);
            } else {
                fprintf(out, " -- <data (%d byte%s)", num, num > 1 ? "s" : "");
            }
            if (ccnb_must_recurse(/* work in progress */)) {
              if (!rawxml) {
                fprintf(out, ", recursive decoding>\n");
              }
            // ...
    //      *buf += num;
    //      *len -= num;
            }
            fprintf(out, ">\n");
            if (!rawxml) {
                hexdump(lev, base, *buf, num, rawxml, out);
            } else {
                base64dump(lev, base, *buf, num, rawxml, out);
            }
    /*
            for (i = 0; i < num; i++) {
                if ((i%8) == 0) {
                fprintf(out, "\n%04zx  ", *buf - base + i);
                for (j = 0; j < lev; j++) fprintf(out, "  ");
                }
                fprintf(out, " %02x", (*buf)[i]);
            }
            fprintf(out, "\n");
    */
            *buf += num;
            *len -= num;
            if (rawxml) {
                int i;
                for(i = 0; i < lev ; i++)
                    fprintf(out, "  ");
                fprintf(out, "</data>\n");
            }
            break;
        case CCN_TT_DTAG:
            next_tag = ccnb_dtag2name(num);
            if (next_tag) {
                if (!rawxml) {
                    fprintf(out, " -- <%s>\n", next_tag);
                } else {
                    fprintf(out, "<%s>\n", next_tag);
                }
            }
            else {
                if (!rawxml) {
                    fprintf(out, " -- <unknown tt=%d num=%d>\n", typ, num);
                } else {
                    fprintf(out, "<unknown>\n");
                }
            }
            if (ccnb_parse_lev(lev+1, base, buf, len, next_tag, rawxml, out) < 0) {
                return -1;
            }
            if (lev == 0) {
                if (*len > 0) {
                    fprintf(out, "\n");
                }
            }
            break;
        case 0:
            if (num == 0 || cur_tag != NULL) { // end tag
                if (!rawxml) {
                    fprintf(out, " -- </%s>\n", cur_tag);
                } else {
                    fprintf(out, "</%s>\n", cur_tag );
                }
                return 0;
            }
        default:
            if (!rawxml) {
                fprintf(out, "-- tt=%d num=%d not implemented yet\n", typ, num);
            } else {
                fprintf(out, "=%d num=%d not implemented yet\n", typ, num);
            }
            break;
        }
    }
    if (cur_tag != NULL) {
        fprintf(out, "</%s>\n", cur_tag );
    }
    return 0;
}


void
ccnb_parse(int lev, unsigned char *data, int len, int rawxml, FILE* out)
{
    unsigned char *buf = data;

    ccnb_parse_lev(lev, data, &buf, &len, NULL, rawxml, out);
    if (!rawxml)
        fprintf(out, "%04zx  pkt.end\n", buf - data);
}

// ----------------------------------------------------------------------
// CCNTLV

enum {
    CTX_GLOBAL = 1,
    CTX_TOPLEVEL,
    CTX_MSG,
    CTX_NAME,
    CTX_METADATA,
    CTX_VALIDALGO,
    CTX_VALIDALGODEPEND
};

static char ccntlv_recurse[][3] = {
    {CTX_TOPLEVEL, CCNX_TLV_TL_Interest, CTX_MSG},
    {CTX_TOPLEVEL, CCNX_TLV_TL_Object, CTX_MSG},
    {CTX_TOPLEVEL, CCNX_TLV_TL_ValidationAlgo, CTX_VALIDALGO},
    {CTX_MSG, CCNX_TLV_M_Name, CTX_NAME},
    {CTX_NAME, CCNX_TLV_N_Meta, CTX_METADATA},
    {CTX_VALIDALGO, CCNX_VALIDALGO_HMAC_SHA256, CTX_VALIDALGODEPEND},
    {0,0,0}
};

static char
ccntlv_must_recurse(char ctx, char typ)
{
    int i;
    for (i = 0; ccntlv_recurse[i][0]; i++) {
        if (ccntlv_recurse[i][0] == ctx && ccntlv_recurse[i][1] == typ) {
            return ccntlv_recurse[i][2];
        }
    }
    return 0;
}


// ----------------------------------------------------------------------

static char*
ccnl_ccntlv_type2name(unsigned char ctx, unsigned int type, int rawxml)
{
    char *cn = "globalCtx", *tn = NULL;
    static char tmp[50];

//    fprintf(stderr, "t2n %d %d\n", ctx, type);

    if (type == CCNX_TLV_M_Name) {
        tn = "Name";
    } else if (type == CCNX_TLV_G_Pad) {
        tn = "Pad";
    } else {
        switch (ctx) {

            case CTX_GLOBAL:
                cn = "global";
                switch (type) {
                case CCNX_TLV_G_Pad:        tn = "Pad"; break;
                default: break;
                }
                break;

            case CTX_TOPLEVEL:
                cn = "toplevelCtx";
                switch (type) {
                case CCNX_TLV_TL_Interest:          tn = "Interest"; break;
                case CCNX_TLV_TL_Object:            tn = "Object"; break;
                case CCNX_TLV_TL_ValidationAlgo:    tn = "ValidationAlgo"; break;
                case CCNX_TLV_TL_ValidationPayload: tn = "ValidationPayload"; break;
                default: break;
                }
                break;
            case CTX_MSG:
                cn = "msgCtx";
                switch (type) {
                case CCNX_TLV_M_Name:       tn = "Name"; break;
                case CCNX_TLV_M_Payload:    tn = "Payload"; break;
                case CCNX_TLV_M_ENDChunk:   tn = "EndChunk"; break;
                default: break;
                }
                break;
            case CTX_NAME:
                cn = "nameCtx";
                switch (type) {
                case CCNX_TLV_N_NameSegment:    tn = "NameSegment"; break;
                case CCNX_TLV_N_IPID:           tn = "InterestPID"; break;
                  /*
                case CCNX_TLV_N_NameKey:        tn = "NameKey"; break;
                case CCNX_TLV_N_ObjHash:        tn = "ObjHash"; break;
                  */
                case CCNX_TLV_N_Chunk:          tn = "Chunk"; break;
                case CCNX_TLV_N_Meta:           tn = "MetaData"; break;
                default: break;
                }
                break;
            case CTX_METADATA:
                cn = "metaDataCtx";
                switch (type) {
                  /*
                case CCNX_TLV_M_KeyID:       tn = "KeyId"; break;
                case CCNX_TLV_M_ObjHash:     tn = "ObjHash"; break;
                case CCNX_TLV_M_PayldType:   tn = "PayloadType"; break;
                case CCNX_TLV_M_Create:      tn = "Create"; break;
                  */
                default: break;
                }
                break;
            case CTX_VALIDALGO:
                cn = "validAlgoCtx";
                switch (type) {
                case CCNX_VALIDALGO_CRC32C:
                    tn = "CRC32C"; break;
                case CCNX_VALIDALGO_HMAC_SHA256:
                    tn = "HMAC_SHA256"; break;
                case CCNX_VALIDALGO_VMAC_128:
                    tn = "VMAC_128"; break;
                case CCNX_VALIDALGO_RSA_SHA256:
                    tn = "RSA_SHA256"; break;
                case CCNX_VALIDALGO_EC_SECP_256K1:
                    tn = "EC_SECP_256K1"; break;
                case CCNX_VALIDALGO_EC_SECP_384R1:
                    tn = "EC_SECP_384R1"; break;
                default: break;
                }
                break;
            case CTX_VALIDALGODEPEND:
                cn = "validAlgoDependendCtx";
                switch (type) {
                case CCNX_VALIDALGO_KEYID:
                    tn = "KeyID"; break;
                default: break;
                }
                break;
            default:
                cn = NULL;
                break;
        }
    }
    if (tn) {
        if(!rawxml)
            sprintf(tmp, "%s\\%s", tn, cn);
        else
            sprintf(tmp, "%s", tn);
    } else if (cn) {
        sprintf(tmp, "type=0x%04x\\%s", type, cn);
    } else {
        sprintf(tmp, "type=0x%04x\\ctx=%d", type, ctx);
    }
    return tmp;
}


static int
ccntlv_parse_sequence(int lev, unsigned char ctx, unsigned char *base,
              unsigned char **buf, int *len, char *cur_tag, int rawxml, FILE* out)
{
    unsigned int vallen, typ;
    int i;
    unsigned char ctx2, *cp;
    char *n, n_old[100], tmp[100];

    while (*len > 0) {
        cp = *buf;
        if (ccnl_ccntlv_dehead(buf, len, &typ, &vallen) < 0) {
            return -1;
        }

        if (vallen > *len) {
          fprintf(stderr, "\n%04zx ** CCNTLV length problem:\n"
              "  type=0x%04hx, len=0x%04hx larger than %d available bytes\n",
              *buf - base, (unsigned short)typ, (unsigned short)vallen, *len);
          exit(-1);
        }

        n = ccnl_ccntlv_type2name(ctx, typ, rawxml);
        if (!n) {
            sprintf(tmp, "type=%hu", (unsigned short)typ);
            n = tmp;
        }


        if(!rawxml)
            fprintf(out, "%04zx  ", cp - base);
        for (i = 0; i < lev; i++) {
                fprintf(out, "  ");
        }
        for (; cp < *buf; cp++) {
            if(!rawxml)
                fprintf(out, "%02x ", *cp);
        }
        if(!rawxml)
            fprintf(out, "-- <%s, len=%d>\n", n, vallen);

        // if(rawxml)
        //     fprintf(out, "</%s>\n", n);
        ctx2 = ccntlv_must_recurse(ctx, typ);
        if (ctx2) {
            if (rawxml)
                fprintf(out, "<%s>\n", n);
            *len -= vallen;
            i = vallen;
            strcpy(n_old, n);
            if (ccntlv_parse_sequence(lev+1, ctx2, base, buf, &i,
                                                        n, rawxml, out) < 0)
                return -1;

            if(rawxml) {
                for (i = 0; i < lev; i++) {
                        fprintf(out, "  ");
                }
                fprintf(out, "</%s>\n", n_old);
            }
        } else {
            if (rawxml && vallen > 0) {
                fprintf(out, "<%s size=\"%i\" dt=\"binary.base64\">\n", n, vallen);
                base64dump(lev, base, *buf, vallen, rawxml, out);
                for (i = 0; i < lev; i++) {
                        fprintf(out, "  ");
                }
                fprintf(out, "</%s>\n", n);
            } else
                hexdump(lev, base, *buf, vallen, rawxml, out);
            *buf += vallen;
            *len -= vallen;
        }
    }

    return 0;
}

void
ccntlv_2015(int lev, unsigned char *data, int len, int rawxml, FILE* out)
{
    unsigned char *buf;
    char *mp;
    unsigned short hdrlen, pktlen; // payloadlen;
    struct ccnx_tlvhdr_ccnx2015_s *hp;

    hp = (struct ccnx_tlvhdr_ccnx2015_s*) data;
    hdrlen = hp->hdrlen; // ntohs(hp->hdrlen);
    pktlen = ntohs(hp->pktlen);

    if (pktlen != len) {
        fprintf(stderr, "length mismatch\n");
    }

    if (!rawxml)
        fprintf(out, "%04zx  hdr.vers=%d\n",
            (unsigned char*) &(hp->version) - data, hp->version);
    if (hp->pkttype == CCNX_PT_Interest)
        mp = rawxml ? "Interest" : "Interest\\toplevelCtx";
    else if (hp->pkttype == CCNX_PT_Data)
        mp = rawxml ? "Content" : "Content\\toplevelCtx";
    else if (hp->pkttype == CCNX_PT_NACK)
        mp = rawxml ? "InterestReturn" : "InterestReturn\\toplevelCtx";
    else if (hp->pkttype == CCNX_PT_Fragment)
        mp = rawxml ? "Fragment" : "Fragment\\toplevelCtx";
    else
        mp = "unknown";
    if (!rawxml) {
        fprintf(out, "%04zx  hdr.pkttyp=0x%02x (%s)\n",
                (unsigned char*) &(hp->pkttype) - data, hp->pkttype, mp);
        fprintf(out, "%04zx  hdr.pktlen=%d\n",
                (unsigned char*) &(hp->pktlen) - data, pktlen);
        if (hp->pkttype == CCNX_PT_Interest || hp->pkttype == CCNX_PT_NACK)
            fprintf(out, "%04zx  hdr.hoplim=%d\n",
                    (unsigned char*) &(hp->hoplimit) - data, hp->hoplimit);
        if (hp->pkttype == CCNX_PT_Fragment) {
            struct ccnx_tlvhdr_ccnx2015_s *fp;

            fp = (struct ccnx_tlvhdr_ccnx2015_s*) hp;
            fprintf(out, "%04zx  hdr.frag: seqnr=0x%05x ",
                    fp->fill - data, ntohs(*(uint16_t*)fp->fill) & 0x03fff);
            if ((fp->fill[0] >> 6) == 0x0)
                fprintf(out, "MID\n");
            else if ((fp->fill[0] >> 6) == 0x1)
                fprintf(out, "BEGIN\n");
            else if ((fp->fill[0] >> 6) == 0x2)
                fprintf(out, "END\n");
            else
                fprintf(out, "SINGLE\n");
        }
        fprintf(out, "%04zx  hdr.hdrlen=%d\n",
                (unsigned char*) &(hp->hdrlen) - data, hdrlen);
        if (hp->pkttype == CCNX_PT_NACK) {
            fprintf(out, "%04zx  hdr.errorc=%d\n",
                    (unsigned char*) &hp->fill - data, hp->fill[0]);
        }
    }

    buf = data + sizeof(struct ccnx_tlvhdr_ccnx2015_s);
    /*
    if (hp->pkttype == CCNX_PT_Interest || hp->pkttype == CCNX_PT_NACK)
        buf += sizeof(struct ccnx_tlvhdr_ccnx2015nack_s);
    */
    // dump the sequence of TLV fields of the optional header
    len = buf - data;
    // if (len > 0) {
    //     ccntlv_parse_sequence(0, CTX_HOP, data, &buf, &len,
    //                                                     "header", rawxml, out);
    //     if (len != 0) {
    //         fprintf(stderr, "%d left over bytes in header\n", len);
    //     }
    // }
    if (!rawxml) {
        fprintf(out, "%04zx", buf - data);
        indent(NULL, lev);
        fprintf(out, "hdr.end\n");
    }

    buf = data + hdrlen;
    len = pktlen - hdrlen;
    if (hp->pkttype == CCNX_PT_Interest ||
             hp->pkttype == CCNX_PT_Data || hp->pkttype == CCNX_PT_NACK) {

        // dump the sequence of TLV fields of the message body
        ccntlv_parse_sequence(lev, CTX_TOPLEVEL, data, &buf, &len,
                                                        "message", rawxml, out);
    } else {
        hexdump(lev, data, buf, len, rawxml, out);
        buf += len;
    }

    if (!rawxml) {
        fprintf(out, "%04zx", buf - data);
        indent(NULL, lev);
        fprintf(out, "pkt.end\n");
    }
}


#ifdef OBSOLETE
void
ccntlv_201411(unsigned char *data, int len, int rawxml, FILE* out)
{
    unsigned char *buf;
    char *mp;
    unsigned short hdrlen, payloadlen;
    struct ccnx_tlvhdr_ccnx201411_s *hp;

    hp = (struct ccnx_tlvhdr_ccnx201411_s*) data;
    hdrlen = hp->hdrlen; // ntohs(hp->hdrlen);
    payloadlen = ntohs(hp->payloadlen);

    if (!rawxml)
        fprintf(out, "%04zx  hdr.vers=%d\n",
            (unsigned char*) &(hp->version) - data, hp->version);
    if (hp->packettype == CCNX_PT_Interest)
        if (!rawxml)
            mp = "Interest\\toplevelCtx";
        else
            mp = "Interest";
    else if (hp->packettype == CCNX_PT_ContentObject)
        if (!rawxml)
            mp = "Object\\toplevelCtx";
        else
            mp = "Object";
    else
        mp = "unknown";
    if (!rawxml) {
        fprintf(out, "%04zx  hdr.mtyp=0x%02x (%s)\n",
                (unsigned char*) &(hp->packettype) - data, hp->packettype, mp);
        fprintf(out, "%04zx  hdr.paylen=%d\n",
                (unsigned char*) &(hp->payloadlen) - data, payloadlen);
        fprintf(out, "%04zx  hdr.hdrlen=%d\n",
                (unsigned char*) &(hp->hdrlen) - data, hdrlen);
    }

    if (hdrlen + payloadlen != len) {
        fprintf(stderr, "length mismatch\n");
    }

    buf = data + 8;
    // dump the sequence of TLV fields of the optional header
    len = hp->hdrlen; // ntohs(hp->hdrlen);
    // if (len > 0) {
    //     ccntlv_parse_sequence(0, CTX_HOP, data, &buf, &len,
    //                                                     "header", rawxml, out);
    //     if (len != 0) {
    //         fprintf(stderr, "%d left over bytes in header\n", len);
    //     }
    // }
    if (!rawxml)
        fprintf(out, "%04zx  hdr.end\n", buf - data);

    // dump the sequence of TLV fields of the message (formerly called payload)
    len = payloadlen;
    buf = data + hdrlen;
    ccntlv_parse_sequence(0, CTX_TOPLEVEL, data, &buf, &len,
                                                        "message", rawxml, out);
    if (!rawxml)
        fprintf(out, "%04zx  pkt.end\n", buf - data);
}
#endif

// ----------------------------------------------------------------------
// CISTLV

enum {
    CIS_CTX_TOPLEVEL = 1,
    CIS_CTX_MSG,
    CIS_CTX_NAME,
    CIS_CTX_KEYNAME
};

static char cistlv_recurse[][3] = {
    {CIS_CTX_TOPLEVEL, CISCO_TLV_Interest, CIS_CTX_MSG},
    {CIS_CTX_TOPLEVEL, CISCO_TLV_Content, CIS_CTX_MSG},
    {CIS_CTX_MSG, CISCO_TLV_Name, CIS_CTX_NAME},
    {CIS_CTX_MSG, CISCO_TLV_KeyName, CIS_CTX_KEYNAME},
    {0,0,0}
};

static char
cistlv_must_recurse(char ctx, char typ)
{
    int i;
    for (i = 0; cistlv_recurse[i][0]; i++) {
        if (cistlv_recurse[i][0] == ctx && cistlv_recurse[i][1] == typ) {
            return cistlv_recurse[i][2];
        }
    }
    return 0;
}

// ----------------------------------------------------------------------

static char*
ccnl_cistlv_type2name(unsigned char ctx, unsigned int type, int rawxml)
{
    char *cn = "globalCtx", *tn = NULL;
    static char tmp[50];

//    fprintf(stderr, "t2n %d %d\n", ctx, type);

    switch (ctx) {

    case CIS_CTX_TOPLEVEL:
        cn = "toplevelCtx";
        switch (type) {
        case CISCO_TLV_Interest:           tn = "Interest"; break;
        case CISCO_TLV_Content:            tn = "Object"; break;
        default: break;
        }
        break;

    case CIS_CTX_MSG:
        cn = "msgCtx";
        switch (type) {
        case CISCO_TLV_Name:               tn = "Name"; break;
        case CISCO_TLV_ContentData:        tn = "ContentData"; break;
        case CISCO_TLV_Certificate:        tn = "Certificate"; break;
        case CISCO_TLV_SignedInfo:         tn = "SignedInfo"; break;
        case CISCO_TLV_ContentDigest:      tn = "ContentDigest"; break;
        case CISCO_TLV_PublicKey:          tn = "PublicKey"; break;
        case CISCO_TLV_KeyName:            tn = "KeyName"; break;
        case CISCO_TLV_Signature:          tn = "Signature"; break;
        case CISCO_TLV_Timestamp:          tn = "Timestamp"; break;
        case CISCO_TLV_Witness:            tn = "Witness"; break;
        case CISCO_TLV_SignatureBits:      tn = "SignatureBits"; break;
        case CISCO_TLV_DigestAlgorithm:    tn = "DigestAlgorithm"; break;
        case CISCO_TLV_FinalSegmentID:     tn = "FinalSegmentID"; break;
        case CISCO_TLV_PublisherPublicKeyDigest: tn = "PublisherPublicKeyDigest"; break;
        case CISCO_TLV_PublisherPublicKeyName:   tn = "PublisherPublicKeyName"; break;
        case CISCO_TLV_ContentExpiration:  tn = "ContentExpiration"; break;
        case CISCO_TLV_CacheTTL:           tn = "CacheTTL"; break;
        case CISCO_TLV_VendorSpecific:     tn = "VendorSpecific"; break;
        case CISCO_TLV_VendorId:           tn = "VendorId"; break;
        default: break;
        }
        break;

    case CIS_CTX_NAME:
    case CIS_CTX_KEYNAME:
        cn =  ctx == CIS_CTX_NAME ? "nameCtx" : "keyNameCtx";
        switch (type) {
        case CISCO_TLV_NameComponent:    tn = "(Key)NameComp"; break;
        case CISCO_TLV_NameSegment:      tn = "NameSegment"; break;
        default: break;
        }
        break;

    default:
        cn = NULL;
        break;
    }

    if (tn) {
        if(!rawxml)
            sprintf(tmp, "%s\\%s", tn, cn);
        else
            sprintf(tmp, "%s", tn);
    } else if (cn) {
        sprintf(tmp, "type=0x%04x\\%s", type, cn);
    } else {
        sprintf(tmp, "type=0x%04x\\ctx=%d", type, ctx);
    }
    return tmp;
}


static int
cistlv_parse_sequence(int lev, unsigned char ctx, unsigned char *base,
              unsigned char **buf, int *len, char *cur_tag, int rawxml, FILE* out)
{
    unsigned int vallen, typ;
    int i;
    unsigned char ctx2, *cp;
    char *n, n_old[100], tmp[100];

    while (*len > 0) {
        cp = *buf;
        if (ccnl_cistlv_dehead(buf, len, &typ, &vallen) < 0) {
            return -1;
        }

        if (vallen > *len) {
          fprintf(stderr, "\n%04zx ** CISTLV length problem:\n"
              "  type=0x%04hx, len=0x%04hx larger than %d available bytes\n",
              *buf - base, (unsigned short)typ, (unsigned short)vallen, *len);
          exit(-1);
        }

        n = ccnl_cistlv_type2name(ctx, typ, rawxml);
        if (!n) {
            sprintf(tmp, "type=%hu", (unsigned short)typ);
            n = tmp;
        }


        if(!rawxml)
            fprintf(out, "%04zx  ", cp - base);
        for (i = 0; i < lev; i++) {
                fprintf(out, "  ");
        }
        for (; cp < *buf; cp++) {
            if(!rawxml)
                fprintf(out, "%02x ", *cp);
        }
        if(!rawxml)
            fprintf(out, "-- <%s, len=%d>\n", n, vallen);

        // if(rawxml)
        //     fprintf(out, "</%s>\n", n);
        ctx2 = cistlv_must_recurse(ctx, typ);
        if (ctx2) {
            if (rawxml)
                fprintf(out, "<%s>\n", n);
            *len -= vallen;
            i = vallen;
            strcpy(n_old, n);
            if (cistlv_parse_sequence(lev+1, ctx2, base, buf, &i,
                                                        n, rawxml, out) < 0)
                return -1;

            if(rawxml) {
                for (i = 0; i < lev; i++) {
                        fprintf(out, "  ");
                }
                fprintf(out, "</%s>\n", n_old);
            }
        } else {
            if (rawxml && vallen > 0) {
                fprintf(out, "<%s size=\"%i\" dt=\"binary.base64\">\n", n, vallen);
                base64dump(lev, base, *buf, vallen, rawxml, out);
                for (i = 0; i < lev; i++) {
                        fprintf(out, "  ");
                }
                fprintf(out, "</%s>\n", n);
            } else
                hexdump(lev, base, *buf, vallen, rawxml, out);
            *buf += vallen;
            *len -= vallen;
        }
    }

    return 0;
}

void
cistlv_201501(int lev, unsigned char *data, int len, int rawxml, FILE* out)
{
    unsigned char *buf;
    char *mp;
    unsigned short hdrlen, pktlen; // payloadlen;
    struct cisco_tlvhdr_201501_s *hp;

    hp = (struct cisco_tlvhdr_201501_s*) data;
    hdrlen = hp->hlen; // ntohs(hp->hdrlen);
    pktlen = ntohs(hp->pktlen);

    if (pktlen != len) {
        fprintf(stderr, "length mismatch\n");
    }

    if (!rawxml)
        fprintf(out, "%04zx  hdr.vers=%d\n",
            (unsigned char*) &(hp->version) - data, hp->version);
    if (hp->pkttype == CISCO_PT_Interest)
        mp = rawxml ? "Interest" : "Interest\\toplevelCtx";
    else if (hp->pkttype == CISCO_PT_Content)
        mp = rawxml ? "Content" : "Content\\toplevelCtx";
    else
        mp = "unknown";
    if (!rawxml) {
        fprintf(out, "%04zx  hdr.ptyp=0x%02x (%s)\n",
                (unsigned char*) &(hp->pkttype) - data, hp->pkttype, mp);
        fprintf(out, "%04zx  hdr.pktlen=%d\n",
                (unsigned char*) &(hp->pktlen) - data, pktlen);
        if (hp->pkttype == CCNX_PT_Interest)
            fprintf(out, "%04zx  hdr.hoplim=%d\n",
                    (unsigned char*) &(hp->hoplim) - data, hp->hoplim);
        fprintf(out, "%04zx  hdr.hdrlen=%d\n",
                (unsigned char*) &(hp->hlen) - data, hdrlen);
    }

    buf = data + sizeof(struct cisco_tlvhdr_201501_s);
    // dump the sequence of TLV fields of the optional header
    len = buf - data;
    if (!rawxml) {
        fprintf(out, "%04zx", buf - data);
        indent(NULL, lev);
        fprintf(out, "hdr.end\n");
    }

    // dump the sequence of TLV fields of the message body
    buf = data + hdrlen;
    len = pktlen - hdrlen;
    cistlv_parse_sequence(lev, CIS_CTX_TOPLEVEL, data, &buf, &len,
                                                        "message", rawxml, out);
    if (!rawxml) {
        fprintf(out, "%04zx", buf - data);
        indent(NULL, lev);
        fprintf(out, "pkt.end\n");
    }
}

// ----------------------------------------------------------------------
// IOTTLV

enum {
    IOT_CTX_TOPLEVEL = 1,
    IOT_CTX_FRAG,
    IOT_CTX_MSG,
    IOT_CTX_HEADER,
    IOT_CTX_EXCLUSION,
    IOT_CTX_FWDTARGET,
    IOT_CTX_NAME,
    IOT_CTX_PAYLOAD,
    IOT_CTX_VALIDATION,
    IOT_CTX_PATHNAME
};

static char iottlv_recurse[][3] = {
    {IOT_CTX_TOPLEVEL, IOT_TLV_Fragment,     IOT_CTX_FRAG},
    {IOT_CTX_TOPLEVEL, IOT_TLV_Request,      IOT_CTX_MSG},
    {IOT_CTX_TOPLEVEL, IOT_TLV_Reply,        IOT_CTX_MSG},
    {IOT_CTX_FRAG,     IOT_TLV_F_OptFragHdr, IOT_CTX_HEADER},
    {IOT_CTX_MSG,      IOT_TLV_R_OptHeader,  IOT_CTX_HEADER},
    {IOT_CTX_MSG,      IOT_TLV_R_Name,       IOT_CTX_NAME},
    {IOT_CTX_MSG,      IOT_TLV_R_Payload,    IOT_CTX_PAYLOAD},
    {IOT_CTX_MSG,      IOT_TLV_R_Validation, IOT_CTX_VALIDATION},
    {IOT_CTX_HEADER,   IOT_TLV_H_Exclusion,  IOT_CTX_EXCLUSION},
    {IOT_CTX_HEADER,   IOT_TLV_H_FwdTarget,  IOT_CTX_FWDTARGET},
    {IOT_CTX_NAME,     IOT_TLV_N_PathName,   IOT_CTX_PATHNAME},
    {0,0,0}
};

static char*
ccnl_iottlv_type2name(unsigned char ctx, unsigned int type)
{
    char *cn = "?ctx?", *tn = NULL;
    static char tmp[50];

    switch (ctx) {
    case IOT_CTX_TOPLEVEL:
        cn = "toplevelCtx";
        switch (type) {
        case IOT_TLV_Fragment:         tn = "Fragment"; break;
        case IOT_TLV_Request:          tn = "Request"; break;
        case IOT_TLV_Reply:            tn = "Reply"; break;
        default: break;
        }
        break;
    case IOT_CTX_FRAG:
        cn = "FragCtx";
        switch (type) {
        case IOT_TLV_F_OptFragHdr:     tn = "OptFragHeader"; break;
        case IOT_TLV_F_FlagsAndSeq:    tn = "FlagsAndSeq"; break;
        case IOT_TLV_F_Data:           tn = "Data"; break;
        default: break;
        }
        break;
    case IOT_CTX_MSG:
        cn = "ReqRepCtx";
        switch (type) {
        case IOT_TLV_R_OptHeader:      tn = "OptHeader"; break;
        case IOT_TLV_R_Name:           tn = "Name"; break;
        case IOT_TLV_R_Payload:        tn = "Payload"; break;
        case IOT_TLV_R_Validation:     tn = "Validation"; break;
        default: break;
        }
        break;
    case IOT_CTX_HEADER:
        cn = "HeaderCtx";
        switch (type) {
        case IOT_TLV_H_HopLim:         tn = "HopLimit"; break;
        case IOT_TLV_H_Exclusion:      tn = "Exclusion"; break;
        case IOT_TLV_H_FwdTarget:      tn = "FwdTarget"; break;
        default: break;
        }
        break;
    case IOT_CTX_NAME:
        cn = "NameCtx";
        switch (type) {
        case IOT_TLV_N_PathName:       tn = "PathName"; break;
        case IOT_TLV_N_FlatLabel:      tn = "FlatLabel"; break;
        case IOT_TLV_N_NamedFunction:  tn = "NamedFunction"; break;
        default: break;
        }
        break;
    case IOT_CTX_PATHNAME:
        cn = "PathNameCtx";
        switch (type) {
        case IOT_TLV_PN_Component:     tn = "Component"; break;
        default: break;
        }
        break;
    case IOT_CTX_PAYLOAD:
        cn = "PayloadCtx";
        switch (type) {
        case IOT_TLV_PL_Data:          tn = "Data"; break;
        case IOT_TLV_PL_Metadata:      tn = "Metadata"; break;
        default: break;
        }
        break;
     default:
        cn = NULL;
        break;
    }
    if (tn) {
        sprintf(tmp, "%s\\%s", tn, cn);
    } else if (cn) {
        sprintf(tmp, "type=0x%04x\\%s", type, cn);
    } else {
        sprintf(tmp, "type=0x%04x\\ctx=%d", type, ctx);
    }
    return tmp;
}

static char
iottlv_must_recurse(char ctx, char typ)
{
    int i;
    for (i = 0; iottlv_recurse[i][0]; i++) {
        if (iottlv_recurse[i][0] == ctx && iottlv_recurse[i][1] == typ) {
            return iottlv_recurse[i][2];
        }
    }
    return 0;
}

static int
iottlv_parse_sequence(int lev, unsigned char ctx, unsigned char *base,
                      unsigned char **buf, int *len, char *cur_tag,
                      int rawxml, FILE* out)
{
    int i;
    unsigned int vallen, typ;
    unsigned char ctx2, *cp;
    char *n, tmp[100];

    while (*len > 0) {
        cp = *buf;
        if (ccnl_iottlv_dehead(buf, len, &typ, &vallen) < 0) {
            return -1;
        }

        if (vallen > *len) {
            fprintf(stderr, "\n%04zx ** IOTTLV length problem:\n"
              "  type=0x%04hx, len=0x%04hx larger than %d available bytes\n",
              *buf - base, (unsigned short)typ, (unsigned short)vallen, *len);
            exit(-1);
        }

        n = ccnl_iottlv_type2name(ctx, typ);
        if (!n) {
            sprintf(tmp, "type=%hu", (unsigned short)typ);
            n = tmp;
        }

        fprintf(out, "%04zx  ", cp - base);
        for (i = 0; i < lev; i++) {
            fprintf(out, "  ");
        }
        for (; cp < *buf; cp++) {
            fprintf(out, "%02x ", *cp);
        }
        fprintf(out, "-- <%s, len=%d>\n", n, vallen);

        ctx2 = iottlv_must_recurse(ctx, typ);
        if (ctx2) {
            *len -= vallen;
            i = vallen;
            if (iottlv_parse_sequence(lev+1, ctx2, base, buf, &i,
                                                        n, rawxml, out) < 0)
                return -1;
        } else {
            hexdump(lev, base, *buf, vallen, rawxml, out);
            *buf += vallen;
            *len -= vallen;
        }
    }

    return 0;
}

void
iottlv_201411(int lev, unsigned char *base, unsigned char *data,
              int len, int rawxml, FILE* out)
{
    // dump the sequence of TLV fields
    iottlv_parse_sequence(lev, IOT_CTX_TOPLEVEL, base, &data, &len,
                          "payload", rawxml, out);
    if (!rawxml)
        fprintf(out, "%04x  pkt.end\n", (int)(data - base));
}

// ----------------------------------------------------------------------
// NDNTLV

#define NDN_TLV_MAX_TYPE 256
static char ndntlv_recurse[NDN_TLV_MAX_TYPE];

static char*
ndn_type2name(unsigned type)
{
    char *n;

    switch (type) {
    case NDN_TLV_Interest:          n = "Interest"; break;
    case NDN_TLV_Data:              n = "Data"; break;
    case NDN_TLV_NDNLP:             n = "NDNLP"; break; // fragment
    case NDN_TLV_Name:              n = "Name"; break;
    case NDN_TLV_NameComponent:     n = "NameComponent"; break;
    case NDN_TLV_Selectors:         n = "Selectors"; break;
    case NDN_TLV_Nonce:             n = "Nonce"; break;
    case NDN_TLV_Scope:             n = "Scope"; break;
    case NDN_TLV_InterestLifetime:  n = "InterestLifetime"; break;
    case NDN_TLV_MinSuffixComponents:   n = "MinSuffixComponents"; break;
    case NDN_TLV_MaxSuffixComponents:   n = "MaxSuffixComponents"; break;
    case NDN_TLV_PublisherPublicKeyLocator: n = "PublisherPublicKeyLocator"; break;
    case NDN_TLV_Exclude:           n = "Exclude"; break;
    case NDN_TLV_ChildSelector:     n = "ChildSelector"; break;
    case NDN_TLV_MustBeFresh:       n = "MustBeFresh"; break;
    case NDN_TLV_Any:               n = "Any"; break;
    case NDN_TLV_MetaInfo:          n = "MetaInfo"; break;
    case NDN_TLV_Content:           n = "Content"; break;
    case NDN_TLV_SignatureInfo:     n = "SignatureInfo"; break;
    case NDN_TLV_SignatureValue:    n = "SignatureValue"; break;
    case NDN_TLV_ContentType:       n = "ContentType"; break;
    case NDN_TLV_FreshnessPeriod:   n = "FreshnessPeriod"; break;
    case NDN_TLV_FinalBlockId:      n = "FinalBlockId"; break;
    case NDN_TLV_SignatureType:     n = "SignatureType"; break;
    case NDN_TLV_KeyLocator:        n = "KeyLocator"; break;
    case NDN_TLV_KeyLocatorDigest:  n = "KeyLocatorDigest"; break;
    case NDN_TLV_Frag_BeginEndFields: n = "FragBeginEndFields"; break;
    case NDN_TLV_NdnlpFragment:     n = "FragmentPayload"; break;
    default:
        n = NULL;
    }
    return n;
}

static int
ndn_parse_sequence(int lev, unsigned char *base, unsigned char **buf,
               unsigned int *len, char *cur_tag, int rawxml, FILE* out)
{
    int i, maxi;
    unsigned int typ, vallen;
    unsigned char *cp;
    char *n, tmp[100];

    while (*len > 0) {
        cp = *buf;
        if (ccnl_ndntlv_dehead(buf, (int*) len, &typ, &vallen) < 0)
            return -1;
        if (vallen > *len) {
            fprintf(stderr, "\n%04zx ** NDN_TLV length problem for %s:\n"
                "  type=%hu, len=%hu larger than %d available bytes\n",
                cp - base, base, (unsigned short)typ, (unsigned short)vallen, *len);
            exit(-1);
        }

        n = ndn_type2name(typ);
        if (!n) {
            sprintf(tmp, "type=%hu", (unsigned short)typ);
            n = tmp;
        }

        if (!rawxml) {
            fprintf(out, "%04zx  ", cp - base);
        }

        for (i = 0; i < lev; i++)
            fprintf(out, "  ");
        if (!rawxml) {
            for (; cp < *buf; cp++)
                fprintf(out, "%02x ", *cp);
        }

        if (rawxml) {
            fprintf(out, "<%s>\n", n);
        } else {
            fprintf(out, "-- <%s, len=%d>\n", n, vallen);
        }

        if (typ < NDN_TLV_MAX_TYPE && ndntlv_recurse[typ]) {
            *len -= vallen;
            if (ndn_parse_sequence(lev+1, base, buf, &vallen, n, rawxml, out) < 0) {
                return -1;
            }
            if (rawxml) {
                fprintf(out, "</%s>\n", n);
            }
            continue;
        }

        // printf("BASE: %s\n", base);
        if (rawxml && vallen > 0) {
            fprintf(out, "<data size=\"%i\" dt=\"binary.base64\">\n", vallen);
            base64dump(lev, base, *buf, vallen, rawxml, out);
            fprintf(out, "</data>\n");

            *len -= vallen;
            *buf += vallen;
        } else {

            while (vallen > 0) {
                maxi = vallen > 8 ? 8 : vallen;
                cp = *buf;
                fprintf(out, "%04zx  ", cp - base);
                for (i = 0; i < lev+1; i++)
                    fprintf(out, "  ");
                for (i = 0; i < 8; i++, cp++){
                        if (i < maxi)
                                fprintf(out, "%02x ", *cp);
                        else
                                fprintf(out, "   ");
                }
                cp = *buf;
                for (i = 79 - 6 - 2*(lev+1) - 8*3 - 12; i > 0; i--)
                fprintf(out, " ");
                fprintf(out, "  |");
                for (i = 0; i < maxi; i++, cp++)
                fprintf(out, "%c", isprint(*cp) ? *cp : '.');
                fprintf(out, "|");
                fprintf(out, "\n");

                vallen -= maxi;
                *buf += maxi;
                *len -= maxi;
            }
        }
        if (rawxml) {
            fprintf(out, "</%s>\n", n);
        }
    }
    return 0;
}

static void
ndntlv_201311(unsigned char *data, unsigned int len, int rawxml, FILE* out)
{
    unsigned char *buf = data;

    // dump the sequence of TLV fields, should start with a name TLV
    ndn_parse_sequence(0, data, &buf, &len, "payload", rawxml, out);
    if (!rawxml) {
        fprintf(out, "%04zx  pkt.end\n", buf - data);
    }
}

static void
ndn_init()
{
    ndntlv_recurse[NDN_TLV_Interest] = 1;
    ndntlv_recurse[NDN_TLV_Data] = 1;
    ndntlv_recurse[NDN_TLV_Fragment] = 1;
    ndntlv_recurse[NDN_TLV_Name] = 1;
    ndntlv_recurse[NDN_TLV_Selectors] = 1;
    ndntlv_recurse[NDN_TLV_MetaInfo] = 1;
    ndntlv_recurse[NDN_TLV_FinalBlockId] = 1;
    ndntlv_recurse[NDN_TLV_SignatureInfo] = 1;
    ndntlv_recurse[NDN_TLV_KeyLocator] = 1;
}

// ----------------------------------------------------------------------
// LOCALRPC

int
localrpc_parse(int lev, unsigned char *base, unsigned char **buf, int *len,
               int rawxml, FILE* out)
{
    int typ, vallen, i;
    unsigned char *cp;
    char *n, tmp[100], dorecurse;

    while (*len > 0) {
        cp = *buf;

        dorecurse = 0;
    /*
    if (*cp && *cp < LRPC_APPLICATION) { // private type value
        sprintf(tmp, "Variable=v%02x", *cp);
        n = tmp;
        *buf += 1;
        *len -= 1;
        vallen = 0;
    } else
    */
        {
            if (ccnl_lrpc_dehead(buf, len, &typ, &vallen) < 0)
                return -1;
            if (vallen > *len) {
                fprintf(stderr, "\n%04zx ** LRPC length problem:\n"
                        "  type=%hu, len=%hu larger than %d available bytes\n",
                        cp - base, (unsigned short)typ, (unsigned short)vallen,
                        *len);
                exit(-1);
            }
            switch(typ) {
            case LRPC_PT_REQUEST:
                n = "Request"; dorecurse = 1; break;
            case LRPC_PT_REPLY:
                n = "Reply"; dorecurse = 1; break;
            case LRPC_APPLICATION:
                n = "Application"; dorecurse = 1; break;
            case LRPC_LAMBDA:
                n = "Lambda"; dorecurse = 1; break;
            case LRPC_SEQUENCE:
                n = "Sequence"; dorecurse = 1; break;
            case LRPC_FLATNAME:
                n = "FlatName"; break;
            case LRPC_NONNEGINT:
                n = "Integer"; break;
            case LRPC_STR:
                n = "String"; break;
            case LRPC_BIN:
                n = "BinaryData"; break;
            case LRPC_NONCE:
                n = "Nonce"; break;
            default:
                sprintf(tmp, "Type=0x%x", (unsigned short)typ);
                n = tmp;
                break;
            }
        }

        printf("%04zx  ", cp - base);
        for (i = 0; i < lev; i++)
            printf("  ");
        for (; cp < *buf; cp++)
            printf("%02x ", *cp);
        printf("-- <%s, len=%d>\n", n, vallen);

        if (dorecurse) {
            *len -= vallen;
            localrpc_parse(lev+1, base, buf, &vallen, rawxml, out);
            continue;
        }

        if (typ == LRPC_NONNEGINT) {
            printf("%04zx  ", *buf - base);
            for (i = 0; i <= lev; i++)
                printf("  ");
            printf("%ld\n", ccnl_ndntlv_nonNegInt(*buf, vallen));
        } else if (typ == LRPC_FLATNAME) {
            printf("%04zx  ", *buf - base);
            for (i = 0; i <= lev; i++)
                printf("  ");
            strcpy(tmp, "\"");
            i = sizeof(tmp) - 6;
            if (vallen < i)
                i = vallen;
            memcpy(tmp+1, *buf, i);
            strcpy(tmp + i + 1, vallen > i ? "\"..." : "\"");
            printf("%s\n", tmp);
        } else if (vallen > 0)
            hexdump(lev, base, *buf, vallen, rawxml, out);
        *buf += vallen;
        *len -= vallen;
    }
    return 0;
}

static void
localrpc_201405(unsigned char *data, int len, int rawxml, FILE* out)
{
    unsigned char *buf = data;
    int origlen = len, typ, vallen;
    // int typ2, vallen2, len2;
    //    unsigned char *cp;

    if (len <= 0 || ccnl_lrpc_dehead(&buf, &len, &typ, &vallen) < 0 ||
                              (typ != LRPC_PT_REQUEST && typ != LRPC_PT_REPLY))
        return;

    /*
    cp = buf;
    len2 = vallen;
    if (ccnl_lrpc_dehead(&buf, &len2, &typ2, &vallen2) < 0)
        return;
    if (typ2 == LRPC_NONNEGINT) { // RPC return code
        printf("0000  RPC-result\n");
        printf("%04zx    INT %ld\n", cp - data,
               ccnl_lrpc_nonNegInt(buf, vallen2));
        buf += vallen2;
        len = origlen - (buf - data);
        localrpc_parse(1, data, &buf, &len, rawxml, out);
    } else { // RPC request
    */
        buf = data;
        localrpc_parse(0, data, &buf, &origlen, rawxml, out);
    //    }

    printf("%04zx  pkt.end\n", buf - data);
}

// ----------------------------------------------------------------------

void
emit_content_only(unsigned char *start, int len, int suite, int format)
{
    unsigned char *data;
    struct ccnl_pkt_s *pkt;

    // we cheat, by hardwired jump over the outermost container heads
    switch (suite) {
    case CCNL_SUITE_CCNB: {
        data = start + 2;
        len -= 2;

        pkt = ccnl_ccnb_bytes2pkt(start, &data, &len);
        break;
    }
    case CCNL_SUITE_CCNTLV: {
        int hdrlen = ccnl_ccntlv_getHdrLen(start, len);
        data = start + hdrlen;
        len -= hdrlen;

        pkt = ccnl_ccntlv_bytes2pkt(start, &data, &len);
        break;
    }
    case CCNL_SUITE_IOTTLV: {
        unsigned int pkttype, vallen;
        data = start;
        if (ccnl_iottlv_dehead(&data, &len, &pkttype, &vallen) < 0)
            return;
        pkt = ccnl_iottlv_bytes2pkt(pkttype, start, &data, &len);
        break;
    }
    case CCNL_SUITE_NDNTLV: {
        unsigned int pkttype, vallen;
        data = start;
        if (ccnl_ndntlv_dehead(&data, &len, &pkttype, &vallen) < 0)
            return;
        pkt = ccnl_ndntlv_bytes2pkt(pkttype, start, &data, &len);
        break;
    }
    default:
        return;
    }
    if (!pkt) {
        DEBUGMSG(WARNING, "extract (%s): parsing error or no prefix\n",
                 ccnl_suite2str(suite));
    }
    write(1, pkt->content, pkt->contlen);
    if (format > 2)
        write(1, "\n", 1);
    free_packet(pkt);
}

// ----------------------------------------------------------------------

void
dump_content(int lev, unsigned char *base, unsigned char *data,
             int len, int format, int suite, FILE *out)
{
    char *forced;
    int enc, oldlen = len;
    unsigned char *olddata = data;

    if (len == 0)
       goto done;

    if (suite >= 0) {
        forced = "forced";
        while (!ccnl_switch_dehead(&data, &len, &enc)) {
            if (format)
                continue;
            printf("%04x", (int)(olddata - base));
            indent(NULL, lev);
            while (olddata < data)
                printf("0x%02x ", *(olddata++));
            printf("-- ignored: switch to %s\n", ccnl_enc2str(enc));
            olddata = data;
        }
    } else {
        forced = "auto-detected";
        while (!ccnl_switch_dehead(&data, &len, &enc)) {
            suite = ccnl_enc2suite(enc);
            if (format)
                continue;
            printf("%04x", (int)(olddata - base));
            indent(NULL, lev);
            while (olddata < data)
                printf("0x%02x ", *(olddata++));
            printf("--> switch to %s\n", ccnl_enc2str(enc));
            forced = "explicit";
            olddata = data;
        }
        if (suite < 0)
            suite = ccnl_pkt2suite(olddata, oldlen, NULL);
    }

    if (format >= 2) {
        emit_content_only(data, len, suite, format);
        return;
    }

    switch (suite) {
    case CCNL_SUITE_CCNB:
        if (format == 0) {
            indent("#   ", lev);
            printf("%s CCNB format\n#\n", forced);
        }
        ccnb_parse(lev, data, len, format == 1, out);
        break;
    case CCNL_SUITE_CCNTLV:
        if (format == 0) {
            indent("#   ", lev);
            printf("%s CCNx TLV format (as of Mar 2015)\n#\n", forced);
        }
        ccntlv_2015(lev, data, len, format == 1, out);
        break;
    case CCNL_SUITE_CISTLV:
        if (format == 0) {
            indent("#   ", lev);
            printf("%s Cisco TLV format (as of Jan 2015)\n#\n", forced);
        }
        cistlv_201501(lev, data, len, format == 1, out);
        break;
    case CCNL_SUITE_IOTTLV:
        if (format == 0) {
            indent("#   ", lev);
            printf("%s IOT TLV format (as of Nov 2014)\n#\n", forced);
        }
        iottlv_201411(lev, base, data, len, format == 1, out);
        break;
    case CCNL_SUITE_LOCALRPC:
        if (format == 0) {
            indent("#   ", lev);
            printf("%s local RPC format (Dec 2014, with NDNTLV encoding)\n#\n",
                   forced);
        }
        localrpc_201405(data, len, format == 1, out);
        break;
    case CCNL_SUITE_NDNTLV:
        if (format == 0) {
            indent("#   ", lev);
            printf("%s NDN TLV format (as of Mar 2014)\n#\n", forced);
        }
        ndntlv_201311(data, len, format == 1, out);
        break;
    default:
        if (format == 0) {
            indent("#   ", lev);
            printf("unknown pkt format, showing plain hex\n");
        }
        hexdump(-1, data, data, len, format == 1, out);
done:
        if (format == 0) {
            printf("%04x", (int)(data - base));
            indent(NULL, lev);
            printf("pkt.end\n");
        }
        break;
    }
}

// ----------------------------------------------------------------------

#ifndef USE_JNI_LIB

int
main(int argc, char *argv[])
{
    int opt, rc;
    unsigned char data[64*1024];
    int len, maxlen, suite = -1, format = 0;
    FILE *out = stdout;

    ndn_init();
    while ((opt = getopt(argc, argv, "hs:f:v:")) != -1) {
        switch (opt) {
        case 's':
            suite = ccnl_str2suite(optarg);
            if (suite < 0 || suite >= CCNL_SUITE_LAST)
                goto help;
            break;
        case 'f':
            format = atoi(optarg);
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;
        default:
help:
            fprintf(stderr,
                    "usage: %s [options] <encoded_data\n"
                    "  -f FORMAT    (0=readable, 1=rawxml, 2=content, 3=content+newline)\n"
                    "  -h           this help\n"
                    "  -s SUITE     (ccnb, ccnx2015, iot2014, ndn2013)\n"
#ifdef USE_LOGGING
                    "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, trace, verbose)\n"
#endif
                    ,
                    argv[0]);
            exit(1);
        }
    }

    if (argv[optind])
        goto help;

    len = 0;
    maxlen = sizeof(data);
    while (maxlen > 0) {
        rc = read(0, data+len, maxlen);
        if (rc == 0)
            break;
        if (rc < 0) {
            perror("read");
            return 1;
        }
        len += rc;
        maxlen -= rc;
    }

    if (format == 0)
        printf("# ccn-lite-pktdump, parsing %d byte%s\n", len, len!=1 ? "s":"");

    dump_content(0, data, data, len, format, suite, out);

    return 0;
}
#endif

// eof

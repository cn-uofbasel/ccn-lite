//
// Created by Balázs Faludi on 27.06.17.
//

#ifdef USE_NFN

#include <ccnl-pkt-switch.h>
#include <ccnl-pkt-ccnb.h>
#include <ccnl-pkt-cistlv.h>
#include <ccnl-pkt-ccntlv.h>
#include <ccnl-pkt-iottlv.h>
#include <ccnl-os-time.h>
#include <ccnl-pkt-ndntlv.h>
#include "ccnl-nfnutil.h"

#include "ccnl-malloc.h"
#include "ccnl-defs.h"
#include "ccnl-buf.h"
#include "ccnl-logging.h"

#ifdef NEEDS_PACKET_CRAFTING

struct ccnl_buf_s*
ccnl_mkSimpleInterest(struct ccnl_prefix_s *name, int *nonce)
{
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, offs;

    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    switch (name->suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        len = ccnl_ccnb_fillInterest(name, NULL, tmp, CCNL_MAX_PACKET_SIZE);
        offs = 0;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        len = ccnl_ccntlv_prependInterestWithHdr(name, &offs, tmp);
        break;
#endif
#ifdef USE_SUITE_CISTLV
    case CCNL_SUITE_CISTLV:
        len = ccnl_cistlv_prependInterestWithHdr(name, &offs, tmp);
        break;
#endif
#ifdef USE_SUITE_IOTTLV
    case CCNL_SUITE_IOTTLV: {
        int rc = ccnl_iottlv_prependRequest(name, NULL, &offs, tmp);
        if (rc <= 0)
            break;
        len = rc;
        rc = ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offs, tmp);
        len = (rc <= 0) ? 0 : len + rc;
        break;
    }
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        len = ccnl_ndntlv_prependInterest(name, -1, nonce, &offs, tmp);
        break;
#endif
    default:
        break;
    }

    if (len > 0)
        buf = ccnl_buf_new(tmp + offs, len);
    ccnl_free(tmp);

    return buf;
}


struct ccnl_buf_s*
ccnl_mkSimpleContent(struct ccnl_prefix_s *name,
                     unsigned char *payload, int paylen, int *payoffset)
{
    struct ccnl_buf_s *buf = NULL;
    unsigned char *tmp;
    int len = 0, contentpos = 0, offs;

    char *s = NULL;
    DEBUGMSG_CUTL(DEBUG, "mkSimpleContent (%s, %d bytes)\n",
                  (s = ccnl_prefix_to_path(name)), paylen);
    ccnl_free(s);

    tmp = (unsigned char*) ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    offs = CCNL_MAX_PACKET_SIZE;

    switch (name->suite) {
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
        len = ccnl_ccnb_fillContent(name, payload, paylen, &contentpos, tmp);
        offs = 0;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV: {
        unsigned int lcn = 0; // lastchunknum
        len = ccnl_ccntlv_prependContentWithHdr(name, payload, paylen, &lcn,
                                                &contentpos, &offs, tmp);
        break;
    }
#endif
#ifdef USE_SUITE_CISTLV
        case CCNL_SUITE_CISTLV:
        len = ccnl_cistlv_prependContentWithHdr(name, payload, paylen,
                                                NULL, // lastchunknum
                                                &offs, &contentpos, tmp);
        break;
#endif
#ifdef USE_SUITE_IOTTLV
        case CCNL_SUITE_IOTTLV: {
        int rc = ccnl_iottlv_prependReply(name, payload, paylen,
                                         &offs, &contentpos, NULL, tmp);
        if (rc <= 0)
            break;
        len = rc;
        rc = ccnl_switch_prependCoding(CCNL_ENC_IOT2014, &offs, tmp);
        len = (rc <= 0) ? 0 : len + rc;
        break;
    }
#endif
#ifdef USE_SUITE_NDNTLV
        case CCNL_SUITE_NDNTLV:
        len = ccnl_ndntlv_prependContent(name, payload, paylen,
                                         &contentpos, NULL, &offs, tmp);
        break;
#endif
        default:
            break;
    }

    if (len) {
        buf = ccnl_buf_new(tmp + offs, len);
        if (payoffset)
            *payoffset = contentpos;
    }
    ccnl_free(tmp);

    return buf;
}

#endif // NEEDS_PACKET_CRAFTING
#endif // USE_NFN
// eof

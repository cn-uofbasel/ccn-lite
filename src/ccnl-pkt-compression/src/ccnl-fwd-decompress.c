/*
 * @f ccnl-fwd-decompress.c
 * @b CCN lite (CCNL), extract compressed packet formats first (for IoT)
 *
 * Copyright (C) 2011-17, University of Basel
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
 */

#include "ccnl-fwd-decompress.h"

#include "ccnl-pkt-ndn-compression.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-fwd.h"

#ifdef USE_SUITE_COMPRESSED
#ifdef USE_SUITE_NDNTLV

int
ccnl_ndntlv_forwarder_decompress(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                      unsigned char **data, int *datalen)
{
    int rc = -1;
    struct ccnl_pkt_s *pkt, *pkt_decompressed = NULL;

    DEBUGMSG_CFWD(DEBUG, "ccnl_ndntlv_forwarder_decompress (%d bytes left)\n", *datalen);

    pkt = ccnl_ndntlvCompressed_bytes2pkt(data, datalen);

    if (!pkt) {
        DEBUGMSG_CFWD(INFO, "  ndntlv packet compressed coding problem\n");
        goto Done;
    }
    int typ = pkt->type;
    pkt_decompressed = ccnl_pkt_ndn_decompress(pkt);
    ccnl_pkt_free(pkt);
    pkt = 0;

    switch (typ) {
    case NDN_TLV_Interest:
        if (ccnl_fwd_handleInterest(relay, from, &pkt_decompressed, ccnl_ndntlv_cMatch))
            goto Done;
        break;
    case NDN_TLV_Data:
        if (ccnl_fwd_handleContent(relay, from, &pkt_decompressed))
            goto Done;
        break;
#ifdef USE_FRAG
    case NDN_TLV_Fragment:
        if (ccnl_fwd_handleFragment(relay, from, &pkt_decompressedpkt, ccnl_ndntlv_forwarder))
            goto Done;
        break;
#endif
    default:
        DEBUGMSG_CFWD(INFO, "  unknown packet type %d, dropped\n", typ);
        break;
    }
    rc = 0;
Done:
    ccnl_pkt_free(pkt_decompressed);
    pkt_decompressed = 0;
    return rc;
}
#endif  //USE_SUITE_NDNTLV

#endif //USE_SUITE_COMPRESSED

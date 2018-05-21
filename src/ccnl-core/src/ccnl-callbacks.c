/*
 * @f ccnl-callbacks.c
 * @b Callback functions
 *
 * Copyright (C) 2018 HAW Hamburg
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
 * 2018-05-21 created (based on ccnl-producer.c)
 */

#include "ccnl-callbacks.h"

/**
 * callback function for inbound on-data events
 */
static ccnl_cb_on_data _cb_rx_on_data = NULL;

/**
 * callback function for outbound on-data events
 */
static ccnl_cb_on_data _cb_tx_on_data = NULL;

void
ccnl_set_cb_rx_on_data(ccnl_cb_on_data func)
{
    _cb_rx_on_data = func;
}

void
ccnl_set_cb_tx_on_data(ccnl_cb_on_data func)
{
    _cb_tx_on_data = func;
}

int
ccnl_callback_rx_on_data(struct ccnl_relay_s *relay,
                         struct ccnl_face_s *from,
                         struct ccnl_pkt_s *pkt)
{
    if (_cb_rx_on_data) {
        return _cb_rx_on_data(relay, from, pkt);
    }

    return 0;
}

int
ccnl_callback_tx_on_data(struct ccnl_relay_s *relay,
                         struct ccnl_face_s *to,
                         struct ccnl_pkt_s *pkt)
{
    if (_cb_tx_on_data) {
        return _cb_tx_on_data(relay, to, pkt);
    }

    return 0;
}

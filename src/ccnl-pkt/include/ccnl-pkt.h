/*
 * @f ccnl-core.h
 * @b Pkt Lib Header file
 *
 * Copyright (C) 2017, University of Basel
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
 * 2015-04-25 created
 */

#ifndef CCNL_PKT_H
#define CCNL_PKT_H

#ifdef USE_SUITE_CCNB
#include "ccnl-pkt-ccnb.h"
#endif

#ifdef USE_SUITE_CCNTLV
#include "ccnl-pkt-ccntlv.h"
#endif

#ifdef USE_SUITE_CISTLV
#include "ccnl-pkt-iottlv.h"
#endif

#ifdef USE_SUITE_IOTTLV
#include "ccnl-pkt-localrpc.h"
#endif

#ifdef USE_SUITE_NDNTLV
#include "ccnl-pkt-ndntlv.h"
#endif

#include "ccnl-pkt-switch.h"

#endif //CCNL_PKT_H
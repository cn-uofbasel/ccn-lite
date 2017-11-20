/*
 * @f ccnl-ext-echo.h
 * @b CCN lite extension: echo/ping service - send back run-time generated data
 *
 * Copyright (C) 2015, Christian Tschudin, University of Basel
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
 * 2015-01-12 created
 */

#ifndef CCN_LITE_CCNL_ECHO_H
#define CCN_LITE_CCNL_ECHO_H

#include "ccnl-core.h"

void
ccnl_echo_request(struct ccnl_relay_s *relay, struct ccnl_face_s *inface,
                  struct ccnl_prefix_s *pfx, struct ccnl_buf_s *buf);

int
ccnl_echo_add(struct ccnl_relay_s *relay, struct ccnl_prefix_s *pfx);

void
ccnl_echo_cleanup(struct ccnl_relay_s *relay);

#endif //CCN_LITE_CCNL_ECHO_H

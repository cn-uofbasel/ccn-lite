/*
 * @f ccnl-pkt-switch.h
 * @b CCN lite (CCNL), fwd header file (internal data structures)
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
 *
 * File history:
 * 2017-06-19 created
 */

#ifndef CCNL_PKT_SWITCH_H
#define CCNL_PKT_SWITCH_H

#ifndef CCNL_LINUXKERNEL
#include <stdint.h>
#else
#include <linux/types.h>
#endif
#include <stddef.h>

int8_t
ccnl_switch_dehead(uint8_t **buf, size_t *len, int32_t *code);

int
ccnl_enc2suite(int enc);

#ifdef NEEDS_PACKET_CRAFTING
#ifndef CCNL_LINUXKERNEL
int
ccnl_switch_prependCodeVal(unsigned long val, int *offset, unsigned char *buf);
#endif
int8_t
ccnl_switch_prependCoding(uint64_t code, size_t *offset, uint8_t *buf, size_t *res);

#endif


#endif // eof

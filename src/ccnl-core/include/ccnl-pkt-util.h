/*
 * @f ccnl-pkt-util.h
 * @b Helper functions for identifying packets
 *
 * Copyright (C) 2011-18 University of Basel
 * Copyright (C) 2018    Safety IO
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


#ifndef CCNL_PKT_UTIL_H
#define CCNL_PKT_UTIL_H

#ifndef CCNL_LINUXKERNEL
#include <stdint.h>
#else
#include <linux/types.h>
#endif
#include <stddef.h>

#include "ccnl-pkt.h"

uint8_t
ccnl_isSuite(int suite);

int
ccnl_suite2defaultPort(int suite);

const char*
ccnl_suite2str(int suite);

int
ccnl_str2suite(char *cp);

int
ccnl_pkt2suite(uint8_t *data, size_t len, size_t *skip);

/**
 * Returns the integer representation of a string
 *
 * @param[in] cmp The string representation of a number
 * @param[in] cmplen The length of the string
 *
 * @return Upon success returns the converted integral number as a long int value
 * @return Upon failure the function returns 0 (e.g. if no valid conversion could be performed)
 */
int
ccnl_cmp2int(unsigned char *cmp, size_t cmplen);

/**
 * Returns the Interest lifetime in seconds
 *
 * @param[in] pkt Pointer to the Interest packet
 *
 * @return        The interest lifetime in seconds
 */
uint64_t
ccnl_pkt_interest_lifetime(const struct ccnl_pkt_s *pkt);

#endif

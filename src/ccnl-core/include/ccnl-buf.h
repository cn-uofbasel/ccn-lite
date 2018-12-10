/**
 * @file ccnl-buf.h
 * @brief CCN lite (CCNL), core header file (internal data structures)
 *
 * @copyright Copyright (C) 2011-17, University of Basel
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef CCNL_BUF_H
#define CCNL_BUF_H

#ifndef CCNL_LINUXKERNEL
#include <unistd.h> //FIXME: SWITCH HERE
#include <string.h>
#endif
#include <stddef.h>


struct ccnl_relay_s;

struct ccnl_buf_s {
    struct ccnl_buf_s *next;
    size_t datalen;
    unsigned char data[1];
};

/**
 * 
 */
struct ccnl_buf_s*
ccnl_buf_new(void *data, size_t len);

#define buf_dup(B)      (B) ? ccnl_buf_new(B->data, B->datalen) : NULL
#define buf_equal(X,Y)  ((X) && (Y) && (X->datalen==Y->datalen) &&\
                         !memcmp(X->data,Y->data,X->datalen))

void
ccnl_core_cleanup(struct ccnl_relay_s *ccnl);
    
#endif //CCNL_BUF_H

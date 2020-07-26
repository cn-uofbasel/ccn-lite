/*
 * @f ccnl-if.c
 * @b CCN lite, core CCNx protocol logic
 *
 * Copyright (C) 2011-18 University of Basel
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
 * 2017-06-16 created
 */


#ifndef CCNL_LINUXKERNEL
#include "ccnl-if.h"
#include "ccnl-os-time.h"
#include "ccnl-malloc.h"
#include "ccnl-logging.h"
#include <sys/socket.h>
#ifndef CCNL_RIOT
#include <sys/un.h>
#else
#include "net/packet.h"
#endif
#include <unistd.h>
#else
#include "../include/ccnl-if.h"
#include "../include/ccnl-os-time.h"
#include "../include/ccnl-malloc.h"
#include "../include/ccnl-logging.h"
#endif

void
ccnl_interface_cleanup(struct ccnl_if_s *i)
{
    size_t j;
    DEBUGMSG_CORE(TRACE, "ccnl_interface_cleanup\n");

    ccnl_sched_destroy(i->sched);
    for (j = 0; j < i->qlen; j++) {
        struct ccnl_txrequest_s *r = i->queue + (i->qfront+j)%CCNL_MAX_IF_QLEN;
        ccnl_free(r->buf);
    }
#if !defined(CCNL_RIOT) && !defined(CCNL_ANDROID) && !defined(CCNL_LINUXKERNEL)
    ccnl_close_socket(i->sock);
#endif
}

#if !defined(CCNL_RIOT) && !defined(CCNL_ANDROID) && !defined(CCNL_LINUXKERNEL)
int
ccnl_close_socket(int s)
{
    struct sockaddr_un su;
    socklen_t len = sizeof(su);

    if (!getsockname(s, (struct sockaddr*) &su, &len) &&
                                        su.sun_family == AF_UNIX) {
        unlink(su.sun_path);
    }
    return close(s);
}
#endif

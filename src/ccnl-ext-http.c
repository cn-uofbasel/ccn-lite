/*
 * @f ccnl-ext-http.c
 * @b CCN lite extension: web server to display the relay's status
 *
 * Copyright (C) 2013, Christian Tschudin, University of Basel
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
 * 2013-04-11 created
 */

#ifdef USE_HTTP_STATUS

struct ccnl_http_s {
    int server, client; // socket
    unsigned char in[512], *out; // ring buffers
    int inoffs, outoffs, inlen, outlen;
};

int ccnl_http_status(struct ccnl_relay_s *ccnl, struct ccnl_http_s *http);

// ----------------------------------------------------------------------

struct ccnl_http_s*
ccnl_http_new(struct ccnl_relay_s *ccnl, int serverport)
{
    int s, i = 1;
    struct sockaddr_in me;
    struct ccnl_http_s *http;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!s) {
        DEBUGMSG(INFO, "could not create socket for http server\n");
        return NULL;
    }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));

    me.sin_family = AF_INET;
    me.sin_port = htons(serverport);
    me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr*) &me, sizeof(me)) < 0) {
        close(s);
        DEBUGMSG(INFO, "could not bind socket for http server\n");
        return NULL;
    }
    listen(s, 2);

    http = (struct ccnl_http_s*) ccnl_calloc(1, sizeof(*http));
    if (!http) {
        close(s);
        return NULL;
    }
    http->server = s;

    DEBUGMSG(INFO, "HTTP status server listening at TCP port %d\n", serverport);

    return http;
}


struct ccnl_http_s*
ccnl_http_cleanup(struct ccnl_http_s *http)
{
    if (!http)
        return NULL;
    if (http->server)
        close(http->server);
    if (http->client)
        close(http->client);
    ccnl_free(http);
    return NULL;
}


int
ccnl_http_anteselect(struct ccnl_relay_s *ccnl, struct ccnl_http_s *http,
                     fd_set *readfs, fd_set *writefs, int *maxfd)
{
    if (!http)
        return -1;
    if (!http->client) {
        FD_SET(http->server, readfs);
        if (*maxfd <= http->server)
            *maxfd = http->server + 1;
    } else {
        if (http->inlen < sizeof(http->in))
            FD_SET(http->client, readfs);
        if (http->outlen > 0)
            FD_SET(http->client, writefs);
        if (*maxfd <= http->client)
            *maxfd = http->client + 1;
    }
    return 0;
}


int
ccnl_http_postselect(struct ccnl_relay_s *ccnl, struct ccnl_http_s *http,
                     fd_set *readfs, fd_set *writefs)
{
    if (!http)
        return -1;
    // accept only one client at the time:
    if (!http->client && FD_ISSET(http->server, readfs)) {
        struct sockaddr_in peer;
        socklen_t len = sizeof(peer);
        http->client = accept(http->server, (struct sockaddr*) &peer, &len);
        if (http->client < 0)
            http->client = 0;
        else {
            DEBUGMSG(INFO, "accepted web server client %s\n",
                     ccnl_addr2ascii((sockunion*)&peer));
            http->inlen = http->outlen = http->inoffs = http->outoffs = 0;
        }
    }
    if (http->client && FD_ISSET(http->client, readfs)) {
        int len = sizeof(http->in) - http->inlen - 1;
        len = recv(http->client, http->in + http->inlen, len, 0);
        if (len == 0) {
            DEBUGMSG(INFO, "web client went away\n");
            close(http->client);
            http->client = 0;
        } else if (len > 0) {
            http->in[len] = 0;
            ccnl_http_status(ccnl, http);
        }
    }
    if (http->client && FD_ISSET(http->client, writefs) && http->out) {
        int len = send(http->client, http->out + http->outoffs,
                       http->outlen, 0);
        if (len > 0) {
            http->outlen -= len;
            http->outoffs += len;
            if (http->outlen == 0) {
                close(http->client);
                http->client = 0;
            }
        }
    }
    return 0;
}

int
ccnl_cmpfaceid(const void *a, const void *b)
{
    int aa = (*(struct ccnl_face_s**)a)->faceid;
    int bb = (*(struct ccnl_face_s**)b)->faceid;
    return  aa - bb;
}

int
ccnl_cmpfib(const void *a, const void *b)
{
    struct ccnl_prefix_s *p1 = (*(struct ccnl_forward_s**)a)->prefix;
    struct ccnl_prefix_s *p2 = (*(struct ccnl_forward_s**)b)->prefix;
    int i, len, r;
    for (i = 0; ; i++) {
        if (p1->compcnt <= i) {
            return p2->compcnt <= i ? 0 : -1;
        }
        if (p2->compcnt <= i) {
            return 1;
        }
        len = p1->complen[i];
        if (len > p2->complen[i])
            len = p2->complen[i];
        r = memcmp(p1->comp[i], p2->comp[i], len);
        if (r)
            return r;
        if (p1->complen[i] > len)
            return 1;
        if (p2->complen[i] > len)
            return -1;
    }
}

int
ccnl_http_status(struct ccnl_relay_s *ccnl, struct ccnl_http_s *http)
{
#define CCNL_HTTP_STATUS_BUFSIZE 64000
    static char txt[CCNL_HTTP_STATUS_BUFSIZE];
    char *buf = txt;
    unsigned int remLen = CCNL_HTTP_STATUS_BUFSIZE;
    unsigned int totalLen = 0;
    int numChars;

    char *cp;
    int i, j, cnt;
    time_t t;
    struct utsname uts;
    struct ccnl_face_s *f;
    struct ccnl_forward_s *fwd;
    struct ccnl_interest_s *ipt;
    struct ccnl_buf_s *bpt;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s",
        "HTTP/1.1 200 OK\n\r"
        "Content-Type: text/html; charset=utf-8\n\r"
        "Connection: close\n\r\n\r");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s",
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>ccn-lite-relay status</title>\n"
        "<style type=\"text/css\">\n"
        "body {font-family: sans-serif;}\n"
        "</style>\n"
        "<meta content=\"text/html; charset=utf-8\" http-equiv=\"content-type\">\n"
        "</head>\n<body>\n"
        "<table borders=0>\n"
        "<tr><td><a href=\"\">[refresh]</a>&nbsp;&nbsp;</td>");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    uname(&uts);
    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<td>ccn-lite-relay Status Page &nbsp;&nbsp; node <strong>%s (%d)</strong></td></tr>\n",
        uts.nodename, getpid());
    if (numChars < 0) goto fail;
    totalLen += numChars;

    t = time(NULL);
    cp = ctime(&t);
    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<tr><td></td><td><font size=-1>%s &nbsp;&nbsp;", cp);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    cp = ctime(&ccnl->startup_time);
    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        " (started %s)</font></td></tr>\n</table>\n", cp);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s",
        "\n<p><table borders=0 width=100%% bgcolor=#e0e0ff>"
        "<tr><td><em>Forwarding table</em></td></tr></table>\n<ul>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    for (fwd = ccnl->fib, cnt = 0; fwd; fwd = fwd->next, cnt++);
    if (cnt > 0) {
        struct ccnl_forward_s **fwda;
        fwda = (struct ccnl_forward_s**) ccnl_malloc(cnt * sizeof(fwd));
        for (fwd = ccnl->fib, i = 0; fwd; fwd = fwd->next, i++)
            fwda[i] = fwd;
        qsort(fwda, cnt, sizeof(fwd), ccnl_cmpfib);
        for (i = 0; i < cnt; i++) {
            char fname[16];
#ifdef USE_ECHO
            if (fwda[i]->tap)
                numChars = snprintf(fname, 16, "%s", "'echoserver'");
            else
#endif
            if(fwda[i]->face)
                numChars = snprintf(fname, 16, "f%d", fwda[i]->face->faceid);
            else
                numChars = snprintf(fname, 16, "?");
            if (numChars < 0) goto fail;

            numChars = ccnl_snprintfAndForward(&buf, &remLen,
                "<li>via %4s: <font face=courier>%s</font></li>\n",
                fname, ccnl_prefix_to_path(fwda[i]->prefix));
            if (numChars < 0) goto fail;
            totalLen += numChars;
        }
        ccnl_free(fwda);
    }
    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s", "</ul>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s",
        "\n<p><table borders=0 width=100%% bgcolor=#e0e0ff>"
        "<tr><td><em>Faces</em></td></tr></table>\n<ul>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    for (f = ccnl->faces, cnt = 0; f; f = f->next, cnt++);
    if (cnt > 0) {
        struct ccnl_face_s **fa;
        fa = (struct ccnl_face_s**) ccnl_malloc(cnt * sizeof(f));
        for (f = ccnl->faces, i = 0; f; f = f->next, i++)
            fa[i] = f;
        qsort(fa, cnt, sizeof(f), ccnl_cmpfaceid);
        for (i = 0; i < cnt; i++) {
            numChars = ccnl_snprintfAndForward(&buf, &remLen,
                "<li><strong>f%d</strong> (via i%d) &nbsp;"
                "peer=<font face=courier>%s</font> &nbsp;ttl=",
                fa[i]->faceid, fa[i]->ifndx, ccnl_addr2ascii(&(fa[i]->peer)));
            if (numChars < 0) goto fail;
            totalLen += numChars;

            if (fa[i]->flags & CCNL_FACE_FLAGS_STATIC)
                numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s", "static");
            else
                numChars = ccnl_snprintfAndForward(&buf, &remLen, "%.1fsec",
                    fa[i]->last_used + CCNL_FACE_TIMEOUT - CCNL_NOW());
            if (numChars < 0) goto fail;
            totalLen += numChars;

            for (j = 0, bpt = fa[i]->outq; bpt; bpt = bpt->next, j++);
            numChars = ccnl_snprintfAndForward(&buf, &remLen, " &nbsp;qlen=%d</li>\n", j);
            if (numChars < 0) goto fail;
            totalLen += numChars;
        }
        ccnl_free(fa);
    }
    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s", "</ul>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s",
        "\n<p><table borders=0 width=100%% bgcolor=#e0e0ff>"
        "<tr><td><em>Interfaces</em></td></tr></table>\n<ul>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    for (i = 0; i < ccnl->ifcount; i++) {
        numChars = ccnl_snprintfAndForward(&buf, &remLen,
            "<li><strong>i%d</strong>&nbsp;&nbsp;"
            "addr=<font face=courier>%s</font>&nbsp;&nbsp;"
            "qlen=%d/%d",
            i, ccnl_addr2ascii(&ccnl->ifs[i].addr),
            ccnl->ifs[i].qlen, CCNL_MAX_IF_QLEN);
        if (numChars < 0) goto fail;
        totalLen += numChars;

#ifdef USE_STATS
        numChars = ccnl_snprintfAndForward(&buf, &remLen,
            "&nbsp;&nbsp;rx=%u&nbsp;&nbsp;tx=%u",
            ccnl->ifs[i].rx_cnt, ccnl->ifs[i].tx_cnt);
        if (numChars < 0) goto fail;
        totalLen += numChars;
#endif

        numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s", "</li>\n");
        if (numChars < 0) goto fail;
        totalLen += numChars;
    }
    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s", "</ul>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s",
        "\n<p><table borders=0 width=100%% bgcolor=#e0e0ff>"
        "<tr><td><em>Misc stats</em></td></tr></table>\n<ul>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    for (cnt = 0, bpt = ccnl->nonces; bpt; bpt = bpt->next, cnt++);
    numChars = ccnl_snprintfAndForward(&buf, &remLen, "<li>Nonces: %d</li>\n", cnt);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    for (cnt = 0, ipt = ccnl->pit; ipt; ipt = ipt->next, cnt++);
    numChars = ccnl_snprintfAndForward(&buf, &remLen, "<li>Pending interests: %d</li>\n", cnt);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<li>Content chunks: %d (max=%d)</li>\n",
        ccnl->contentcnt, ccnl->max_cache_entries);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s", "</ul>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s",
        "\n<p><table borders=0 width=100%% bgcolor=#e0e0ff>"
        "<tr><td><em>Config</em></td></tr></table><table borders=0>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<tr><td>content.timeout:</td>"
        "<td align=right> %d</td><td></td>\n", CCNL_CONTENT_TIMEOUT);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<tr><td>face.timeout:</td>"
        "<td align=right> %d</td><td></td>\n", CCNL_FACE_TIMEOUT);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<tr><td>interest.maxretransmit:</td>"
        "<td align=right> %d</td><td></td>\n", CCNL_MAX_INTEREST_RETRANSMIT);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<tr><td>interest.timeout:</td>"
        "<td align=right> %d</td><td></td>\n", CCNL_INTEREST_TIMEOUT);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<tr><td>nonces.max:</td>"
        "<td align=right> %d</td><td></td>\n", CCNL_MAX_NONCES);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<tr><td>compile.featureset:</td><td></td><td> %s</td>\n", compile_string);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<tr><td>compile.time:</td>"
        "<td></td><td>%s %s</td>\n", __DATE__, __TIME__);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen,
        "<tr><td>compile.ccnl_core_version:</td>"
        "<td></td><td>%s</td>\n", CCNL_VERSION);
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s", "</table>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    numChars = ccnl_snprintfAndForward(&buf, &remLen, "%s", "\n<p></p><hr></body></html>\n");
    if (numChars < 0) goto fail;
    totalLen += numChars;

    http->out = (unsigned char*) txt;
    http->outoffs = 0;
    http->outlen = totalLen;

    if (totalLen >= CCNL_HTTP_STATUS_BUFSIZE) {
        http->outlen = CCNL_HTTP_STATUS_BUFSIZE;
        DEBUGMSG(WARNING, "HTTP result too big for buffer, needed: %d, available: %d.\n",
                 totalLen+1, CCNL_HTTP_STATUS_BUFSIZE);
    }

    return 0;

fail:
    assert(numChars < 0);
    DEBUGMSG(ERROR, "Encoding error %d occured while creating HTTP response.\n", numChars);
    return numChars;
}

#endif // USE_HTTP_STATUS

// eof

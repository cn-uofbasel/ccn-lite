/*
 * @f util/ccn-lite-simplenfn.c
 * @b simple NFN query app (output to stdout)
 *
 * Copyright (C) 2014, Christian Tschudin, University of Basel
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
 * 2014-10-16  created (copied in large part from ccn-lite-peek.c)
 */

#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_NDNTLV

#include "ccnl-common.c"
#include "ccnl-socket.c"

int
myprint(char *str, ...)
{
    return printf("Parse error: %s\n", str);
}

// ----------------------------------------------------------------------

struct ccnl_prefix_s*
expr_to_NFNprefix(char *defaultNFNpath, int suite, char *expr)
{
    char *cp = expr, *name = 0;
    char tmp[1024];

    // trim
    while (isblank(*expr))
        expr++;
    name = expr + strlen(expr) - 1;
    while (isblank(*name)) {
        *name = '\0';
        if (name > expr)
            name--;
    }

    // walk backwards, find last name of expr (this will fail with parentheses)
    while (name >= expr && (isalnum(*name) || strchr("/._-+%", *name)))
        name--;
    if (name[1] == '/') { // found a name
        cp = strdup(name+1);
        name[1] = '\0';
        name = cp;
        cp = expr + strlen(expr) - 1;
        while (cp >= expr && isblank(*cp)) {
            *cp = '\0';
            cp--;
        }
    } else
        name = 0;

    if (name == expr)
        expr = NULL;

    if (!name) {
        name = defaultNFNpath;
/*
        // if expr has not the form (@x ...) then add it
        if (expr && !strchr(expr, '@')) {
            sprintf(tmp, "@x (%s)", expr);
            expr = tmp;
        }
*/
    }
    if (expr) {
        struct ccnl_lambdaTerm_s *lt;
//            char *cp = expr;
        lt = ccnl_lambdaStrToTerm(1, &expr, myprint);
        ccnl_lambdaTermToStr(tmp, lt, ' ');
        expr = tmp;
    }

    fprintf(stderr, "route hint is <%s>\n", name);
    fprintf(stderr, "lambda expression is <%s>\n", (expr && *expr) ? expr : NULL);
    return ccnl_URItoPrefix(name ? name : defaultNFNpath,
                            suite, (expr && *expr) ? expr : NULL);
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char out[64*1024];
    int cnt, len, opt, sock = 0, suite = CCNL_SUITE_DEFAULT;
    char *udp = "127.0.0.1/6363", *ux = NULL;
    char *defaultNFNpath = strdup("/ndn/ch/unibas/nfn");
    struct sockaddr sa;
    struct ccnl_prefix_s *prefix;
    float wait = 3.0;
    int (*mkInterest)(struct ccnl_prefix_s*, int*, int*, unsigned char*, int);
    int (*isContent)(unsigned char*, int);

    while ((opt = getopt(argc, argv, "hn:s:u:w:x:")) != -1) {
        switch (opt) {
        case 'n':
            defaultNFNpath = optarg;
            break;
        case 's':
            opt = ccnl_str2suite(optarg);
            if (opt < 0 || opt >= CCNL_SUITE_LAST)
                goto usage;
            suite = opt;
            switch (suite) {
#ifdef USE_SUITE_CCNB
            case CCNL_SUITE_CCNB:
                udp = "127.0.0.1/9695";
                break;
#endif
#ifdef USE_SUITE_CCNTLV
            case CCNL_SUITE_CCNTLV:
                udp = "127.0.0.1/9695";
                break;
#endif
#ifdef USE_SUITE_NDNTLV
            case CCNL_SUITE_NDNTLV:
                udp = "127.0.0.1/6363";
                break;
#endif
            }
            break;
        case 'u':
            udp = optarg;
            break;
        case 'w':
            wait = atof(optarg);
            break;
        case 'x':
            ux = optarg;
            break;
        case 'h':
        default:
usage:
            fprintf(stderr, "usage: %s [options] NFNexpr\n"
            "  -n NFNPATH       default prefix towards some NFN node(s)\n"
            "  -s SUITE         (ccnb, ccnx2014, ndn2013)\n"
            "  -u a.b.c.d/port  UDP destination (default is 127.0.0.1/6363)\n"
            "  -w timeout       in sec (float)\n"
            "  -x ux_path_name  UNIX IPC: use this instead of UDP\n"
            "Examples:\n"
            "%% simplenfn /ndn/edu/wustl/ping\n"
            "%% simplenfn \"echo hello world\"\n"
            "%% simplenfn \"getFromNameSpace 'ccnx2014 /ccnx/parc/info.txt\"\n"
            "%% simplenfn \"add 1 1\"\n",
            argv[0]);
            exit(1);
        }
    }

    if (!argv[optind] || argv[optind+1]) 
        goto usage;

    srandom(time(NULL));

    switch(suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        mkInterest = ccnl_ccnb_fillInterest;
        isContent = ccnb_isContent;
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:
        mkInterest = ccntlv_mkInterest;
        isContent = ccntlv_isObject;
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        mkInterest = ndntlv_mkInterest;
        isContent = ndntlv_isData;
        break;
#endif
    default:
        printf("unknown suite\n");
        exit(-1);
    }

    if (ux) { // use UNIX socket
        struct sockaddr_un *su = (struct sockaddr_un*) &sa;
        su->sun_family = AF_UNIX;
        strcpy(su->sun_path, ux);
        sock = ux_open();
    } else { // UDP
        struct sockaddr_in *si = (struct sockaddr_in*) &sa;
        udp = strdup(udp);
        si->sin_family = PF_INET;
        si->sin_addr.s_addr = inet_addr(strtok(udp, "/"));
        si->sin_port = htons(atoi(strtok(NULL, "/")));
        sock = udp_open();
    }

    prefix = expr_to_NFNprefix(defaultNFNpath, suite, argv[optind]);
    if (!prefix)
        goto done;
    for (cnt = 0; cnt < 3; cnt++) {
        int nonce = random();

        len = mkInterest(prefix, 
                         NULL, // chunknum
                         &nonce, 
                         out, sizeof(out));

        if (sendto(sock, out, len, 0, &sa, sizeof(sa)) < 0) {
            perror("sendto");
            myexit(1);
        }

        for (;;) { // wait for a content pkt (ignore interests)
            int rc;

            if (block_on_read(sock, wait) <= 0) // timeout
                break;
            len = recv(sock, out, sizeof(out), 0);
/*
            fprintf(stderr, "received %d bytes\n", len);
            if (len > 0)
                fprintf(stderr, "  suite=%d\n", ccnl_pkt2suite(out, len));
*/
            rc = isContent(out, len);
            if (rc < 0)
                goto done;
            if (rc == 0) { // it's an interest, ignore it
                fprintf(stderr, "skipping non-data packet\n");
                continue;
            }
            write(1, out, len);
            myexit(0);
        }
        if (cnt < 2)
            fprintf(stderr, "re-sending interest\n");
    }
    fprintf(stderr, "timeout\n");

done:
    close(sock);
    myexit(-1);
    return 0; // avoid a compiler warning
}

// eof

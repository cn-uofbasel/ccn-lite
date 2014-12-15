/*
 * @f util/ccn-lite-rpc.c
 * @b RPC command line interface
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
 * 2014-05-11  created
 */

#define USE_SUITE_NDNTLV
#define USE_SUITE_LOCALRPC

#include "ccnl-common.c"

/*
  use examples:

  ccn-lite-rpc '/rpc/builtin/syslog "hello syslog"' | ccn-lite-pktdump
  ccn-lite-rpc '/rpc/builtin/lookup /rpc/config/compileString' | ccn-lite-pktdump
  ccn-lite-rpc '/rpc/builtin/lookup /rpc/config/localTime' | ccn-lite-pktdump
  ccn-lite-rpc -n '(/rpc/builtin/forward /rpc/const/ndn2013 &1' thePktFile

Not working yet:
  ccn-lite-rpc '/rpc/builtin/lookup /rpc/config/thisface/defaultSuite'
  ccn-lite-rpc '/rpc/builtin/set /rpc/config/thisface/defaultSuite /rpc/const/encoding/ndn2013'

 */

// ----------------------------------------------------------------------

char *unix_path;
char **fileargs;
int filecnt;

void
myexit(int rc)
{
    if (unix_path)
        unlink(unix_path);
    exit(rc);
}

// ----------------------------------------------------------------------

int
udp_open()
{
    int s;
    struct sockaddr_in si;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("udp socket");
        exit(1);
    }
    si.sin_addr.s_addr = INADDR_ANY;
    si.sin_port = htons(0);
    si.sin_family = PF_INET;
    if (bind(s, (struct sockaddr *)&si, sizeof(si)) < 0) {
        perror("udp sock bind");
        exit(1);
    }

    return s;
}

int
ux_open()
{
static char mysockname[200];
 int sock, bufsize;
    struct sockaddr_un name;

    sprintf(mysockname, "/tmp/.ccn-lite-peek-%d.sock", getpid());
    unlink(mysockname);

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
        exit(1);
    }
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, mysockname);
    if (bind(sock, (struct sockaddr *) &name,
             sizeof(struct sockaddr_un))) {
        perror("binding path name to datagram socket");
        exit(1);
    }

    bufsize = 4 * CCNL_MAX_PACKET_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    unix_path = mysockname;
    return sock;
}

// ----------------------------------------------------------------------

int
block_on_read(int sock, float wait)
{
    fd_set readfs;
    struct timeval timeout;
    int rc;

    FD_ZERO(&readfs);
    FD_SET(sock, &readfs);
    timeout.tv_sec = wait;
    timeout.tv_usec = 1000000.0 * (wait - timeout.tv_sec);
    rc = select(sock+1, &readfs, NULL, NULL, &timeout);
    if (rc < 0)
        perror("select()");
    return rc;
}

struct rdr_ds_s*
loadFile(char **cpp)
{
    int n, fd, len, maxlen;
    struct rdr_ds_s *c;

    (*cpp)++;
    n = atoi(*cpp);
    while (isdigit(**cpp))
        (*cpp)++;
    if (n < 1 || n >= filecnt) {
        fprintf(stderr, "parse error, no file at position %d\n", n);
        return 0;
    }
    if (!n) {  // should read stdin
        fd = 0;
        maxlen = 64*1024;
    } else {
        struct stat st;
        if (stat(fileargs[n-1], &st)) {
            fprintf(stderr, "cannot access file %s\n", fileargs[n-1]);
            return 0;
        }
        maxlen = st.st_size;
        fd = open(fileargs[n-1], O_RDONLY);
        if (fd < 0) {
            perror("open file");
            return 0;
        }
    }
    c = calloc(1, sizeof(*c) + maxlen);
    if (!c) {
        fprintf(stderr, "read file: malloc problem");
        return 0;
    }
    c->type = LRPC_BIN;
    c->flatlen = -1;
    c->aux = c + 1;

    while (maxlen > 0) {
        len = read(fd, (char*)(c+1) + c->u.binlen, maxlen);
        if (len < 0) {
            free(c);
            perror("reading file");
            return 0;
        }
        if (len == 0)
            break;
        maxlen -= len;
        c->u.binlen += len;
    }
    if (fd > 0)
        close(fd);

    return c;
}

struct rdr_ds_s*
parseConst(char **cpp)
{
    struct rdr_ds_s *c;
    char *p;
    int len;

    if (**cpp == '&') // load file as BIN
        return loadFile(cpp);

    c = calloc(1, sizeof(*c));
    if (!c)
        return 0;
    c->flatlen = -1;
    c->aux = (struct rdr_ds_s*) *cpp;

    p = *cpp;
    if (isalpha(*p) || *p == '/') {
        c->type = LRPC_FLATNAME;
        while (*p == '/' || isalnum(*p) || *p == '_')
            p++;
        c->u.namelen = p - *cpp;
    } else if (isdigit(*p)) {
        c->type = LRPC_NONNEGINT;
        c->u.nonnegintval = 0;
        while (isdigit(*p)) {
            c->u.nonnegintval = 10 * c->u.nonnegintval + (*p - '0');
            p++;
        }
    } else if (*p == '\"') {
        c->type = LRPC_STR;
        p++;
        c->aux = (struct rdr_ds_s*) p;
        while (*p && *p != '\"')
            p++;
        if (*p)
            p++;
        c->u.strlen = p - *cpp - 2;
    } else {
        fprintf(stderr, "parse error, unknown constant format %s\n", *cpp);
        free(c);
        return 0;
    }
    len = p - *cpp;
    *cpp += len;

    return c;
}

struct rdr_ds_s*
parsePrefixTerm(int lev, char **cpp)
{
    struct rdr_ds_s *term = NULL, *end = 0, *t2;

    while (**cpp) {
        while (isspace(**cpp))
            *cpp += 1;

        if (**cpp == ')')
            return term;
        if (**cpp == '(') {
            *cpp += 1;
            t2 = parsePrefixTerm(lev+1, cpp);
            if (!t2)
                return 0;
            if (**cpp != ')') {
                fprintf(stderr, "parsePrefixTerm error: missing )\n");
                return 0;
            }
            *cpp += 1;
            if (!term) {
                term = t2;
                continue;
            }
        } else if (**cpp == '\\') { // lambda
            *cpp += 1;
            t2 = calloc(1, sizeof(*t2));
            t2->type = LRPC_LAMBDA;
            t2->flatlen = -1;
            t2->u.lambdavar = parseConst(cpp);
            t2->aux = parsePrefixTerm(lev+1, cpp);
            if (!term) {
                term = t2;
                continue;
            }
        } else {
            t2 = parseConst(cpp);
            if (!term) {
                term = calloc(1, sizeof(*t2));
                term->type = LRPC_APPLICATION;
                term->flatlen = -1;
                term->u.fct = t2;
                continue;
            }
        }
        if (!end)
            term->aux = t2;
        else
            end->nextinseq = t2;
        end = t2;
    }
    return term;
}

int ccnl_rdr_dump(int lev, struct rdr_ds_s *x)
{
    int i, t;
    char *n, tmp[10];

    t = ccnl_rdr_getType(x);
    if (t < LRPC_NOT_SERIALIZED)
        return t;
    if (t < LRPC_APPLICATION) {
        sprintf(tmp, "v%02x", t);
        n = tmp;
    } else switch (t) {
    case LRPC_APPLICATION:
        n = "APP"; break;
    case LRPC_LAMBDA:
        n = "LBD"; break;
    case LRPC_SEQUENCE:
        n = "SEQ"; break;
    case LRPC_FLATNAME:
        n = "VAR"; break;
    case LRPC_NONNEGINT:
        n = "INT"; break;
    case LRPC_BIN:
        n = "BIN"; break;
    case LRPC_STR:
        n = "STR"; break;
    default:
        n = "???"; break;
    }
    for (i = 0; i < lev; i++)
        fprintf(stderr, "  ");
    fprintf(stderr, "%s (0x%x, len=%d)\n", n, t, x->flatlen);

    switch (t) {
    case LRPC_APPLICATION:
        ccnl_rdr_dump(lev+1, x->u.fct);
        break;
    case LRPC_LAMBDA:
        ccnl_rdr_dump(lev+1, x->u.lambdavar);
        break;
    case LRPC_SEQUENCE:
        break;
    case LRPC_FLATNAME:
    case LRPC_NONNEGINT:
    case LRPC_BIN:
    case LRPC_STR:
    default:
        return 0;
    }
    x = x->aux;
    while (x) {
        ccnl_rdr_dump(lev+1, x);
        x = x->nextinseq;
    }
    return 0;
}


struct rdr_ds_s*
rpc_cli2rdr(char *cmd)
{
    char *cp = cmd;
    struct rdr_ds_s *e;

    e = parsePrefixTerm(0, &cp);

/*
    ccnl_rdr_getFlatLen(e);
    ccnl_rdr_dump(0, e);
*/

    return e;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char request[64*1024], reply[64*1024];
    int cnt, opt, reqlen, replen, sock = 0;
    struct sockaddr sa;
    char *udp = "127.0.0.1:6363", *ux = NULL;
    float wait = 3.0;
    struct rdr_ds_s *expr;
    char noreply = 0;

    while ((opt = getopt(argc, argv, "hnu:v:w:x:")) != -1) {
        switch (opt) {
        case 'n':
            noreply = 1 - noreply;
            break;
        case 'u':
            udp = optarg;
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;
        case 'w':
            wait = atof(optarg);
            break;
        case 'x':
            ux = optarg;
            break;
        case 'h':
        default:
Usage:
            fprintf(stderr, "usage: %s "
            "[-u host:port] [-x ux_path_name] [-w timeout] EXPRESSION [filenames]\n"
            "  -n               no-reply (just send)\n"
            "  -u a.b.c.d:port  UDP destination (default is 127.0.0.1/6363)\n"
#ifdef USE_LOGGING
            "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, trace, verbose)\n"
#endif
            "  -w timeout       in sec (float)\n"
            "  -x ux_path_name  UNIX IPC: use this instead of UDP\n"
            "EXPRESSION examples:\n"
            "  \"/rpc/builtin/lookup /rpc/config/localTime)\"\n"
            "  \"/rpc/builtin/forward /rpc/const/encoding/ndn2013 &1\"\n",
            argv[0]);
            exit(1);
        }
    }

    if (!argv[optind]) 
        goto Usage;

    fileargs = argv + optind + 1;
    filecnt = argc - optind;
    expr = rpc_cli2rdr(argv[optind]);
    if (!expr)
        return -1;

    reqlen = ccnl_rdr_serialize(expr, request, sizeof(request));
//    fprintf(stderr, "%p len=%d flatlen=%d\n", expr, reqlen, expr->flatlen);
    write(1, request, reqlen);

    srandom(time(NULL));

    if (ux) { // use UNIX socket
        struct sockaddr_un *su = (struct sockaddr_un*) &sa;
        su->sun_family = AF_UNIX;
        strcpy(su->sun_path, ux);
        sock = ux_open();
    } else { // UDP
        struct sockaddr_in *si = (struct sockaddr_in*) &sa;
        char *cp = strdup(udp);
        si->sin_family = PF_INET;
        si->sin_addr.s_addr = inet_addr(strtok(cp, ":"));
        si->sin_port = htons(atoi(strtok(NULL, ":")));
        sock = udp_open();
        free(cp);
    }

    for (cnt = 0; cnt < 3; cnt++) {
        if (sendto(sock, request, reqlen, 0, &sa, sizeof(sa)) < 0) {
            perror("sendto");
            myexit(1);
        }
        if (noreply)
            goto done;

        for (;;) { // wait for a content pkt (ignore interests)
            if (block_on_read(sock, wait) <= 0) // timeout
                break;
            replen = recv(sock, reply, sizeof(reply), 0);
/*
            fprintf(stderr, "received %d bytes\n", len);
            if (len > 0)
                fprintf(stderr, "  suite=%d\n", ccnl_pkt2suite(out, len));
*/
            if (replen <= 0)
                goto done;
            if (*reply != LRPC_APPLICATION) { // not a RPC pkt
                fprintf(stderr, "skipping non-RPC packet\n");
                continue;
            }
            write(1, reply, replen);
            myexit(0);
        }
        if (cnt < 2)
            fprintf(stderr, "re-sending RPC request\n");
    }
    fprintf(stderr, "timeout (no RPC reply received so far)\n");

done:
    close(sock);
    myexit(-1);

    return 0; // avoid a compiler warning
}

// eof

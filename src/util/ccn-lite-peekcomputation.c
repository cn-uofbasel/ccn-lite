
#include "ccnl-common.c"
#include "../krivine-common.c"

#include "ccnl-socket.c"

int
mkInterestCompute(char **namecomp, char *computation, int computationlen, char *out)
{
#ifndef USE_UTIL
    DEBUGMSG(TRACE, "mkInterestCompute()\n");
#endif
    int len = 0, k, i;
    unsigned char *cp;

    len += mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    while (*namecomp) {
    len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
    cp = (unsigned char*) strdup(*namecomp);
    k = k = strlen(*namecomp);
    len += mkHeader(out+len, k, CCN_TT_BLOB);
    memcpy(out+len, cp, k);
    len += k;
    out[len++] = 0; // end-of-component
    free(cp);
    namecomp++;
    }
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, computation, computationlen);
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "NFN");
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}

int main(int argc, char **argv){

    unsigned char out[CCNL_MAX_PACKET_SIZE];
    int i = 0, len, opt, sock = 0;
    char *namecomp[CCNL_MAX_NAME_COMP], *cp, *dest, *comp;
    char *udp = "127.0.0.1/9695", *ux = NULL;
    float wait = 3.0;
    int (*sendproc)(int,char*,unsigned char*,int);
    int print = 0;

    while ((opt = getopt(argc, argv, "hptu:v:w:x:")) != -1) {
        switch (opt) {
        case 'u':
        udp = optarg;
        break;
        case 'w':
        wait = atof(optarg);
        break;
        case 'x':
        ux = optarg;
        break;
        case 'p':
            print = 1;
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = atoi(optarg);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;
        case 'h':
        default:
Usage:
        fprintf(stderr, "usage: %s "
        "[-u host/port] [-x ux_path_name] [-w timeout] COMPUTATION URI\n"
        "  -p               print interest on console and exit\n"
        "  -u a.b.c.d/port  UDP destination (default is 127.0.0.1/9695)\n"
#ifdef USE_LOGGING
        "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
        "  -w timeout       in sec (float)\n"
        "  -x ux_path_name  UNIX IPC: use this instead of UDP\n",
        argv[0]);
        exit(1);
        }
    }
    if (!argv[optind])
        goto Usage;
    comp = argv[optind++];
    if (!argv[optind])
        goto Usage;

    struct ccnl_prefix_s *prefix = create_prefix_from_name(argv[optind]);
    len = mkInterestCompute(prefix->comp, comp, strlen(comp), out);

    if(print){
        fwrite(out, sizeof(char), len, stdout);
        return 0;
    }

    if (ux) { // use UNIX socket
    dest = ux;
    sock = ux_open();
    sendproc = ux_sendto;
    } else { // UDP
    dest = udp;
    sock = udp_open();
    sendproc = udp_sendto;
    }
    request_content(sock, sendproc, dest, out, len, wait);
    return 0;
}

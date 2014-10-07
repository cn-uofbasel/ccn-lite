/* 
Fetches the data for a CCN name.
The name can either represent a single content object or a chunked content object.
If the first object is a junk, it checks for a final block id.
If it is not available, it will sequentially fetch each chunk until 
a chunk is received with the final block id. If the chunk id is equal to the final block id
all data is fetched, otherwise windowed mode is entered.
Window mode will eventually make it possible to implement tcp-like content fetch 
for congestion control, however currently we simply send out interets for all chunks.
*/
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../ccnl.h"

#include "ccnl-common.c"
#include "../krivine-common.c"

#include "ccnl-socket.c"

// ----------------------------------------------------------------------

int
mkHeader(unsigned char *buf, unsigned int num, unsigned int tt)
{
    unsigned char tmp[100];
    int len = 0, i;

    *tmp = 0x80 | ((num & 0x0f) << 3) | tt;
    len = 1;
    num = num >> 4;

    while (num > 0) {
    tmp[len++] = num & 0x7f;
    num = num >> 7;
    }
    for (i = len-1; i >= 0; i--)
    *buf++ = tmp[i];
    return len;
}

int
ccnb_mkInterest(char **namecomp,
       char *minSuffix, char *maxSuffix,
       unsigned char *digest, int dlen,
       unsigned char *publisher, int plen,
       char *scope,
       uint32_t *nonce,
       unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest

    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    while (*namecomp) {
    len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
    k = unescape_component((unsigned char*) *namecomp);
    len += mkHeader(out+len, k, CCN_TT_BLOB);
    memcpy(out+len, *namecomp++, k);
    len += k;
    out[len++] = 0; // end-of-component
    }
    if (digest) {
    len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
    len += mkHeader(out+len, dlen, CCN_TT_BLOB);
    memcpy(out+len, digest, dlen);
    len += dlen;
    out[len++] = 0; // end-of-component
    if (!maxSuffix)
        maxSuffix = "0";
    }
    out[len++] = 0; // end-of-name

    if (minSuffix) {
    k = strlen(minSuffix);
    len += mkHeader(out+len, CCN_DTAG_MINSUFFCOMP, CCN_TT_DTAG);
    len += mkHeader(out+len, k, CCN_TT_UDATA);
    memcpy(out + len, minSuffix, k);
    len += k;
    out[len++] = 0; // end-of-minsuffcomp
    }

    if (maxSuffix) {
    k = strlen(maxSuffix);
    len += mkHeader(out+len, CCN_DTAG_MAXSUFFCOMP, CCN_TT_DTAG);
    len += mkHeader(out+len, k, CCN_TT_UDATA);
    memcpy(out + len, maxSuffix, k);
    len += k;
    out[len++] = 0; // end-of-maxsuffcomp
    }

    if (publisher) {
    len += mkHeader(out+len, CCN_DTAG_PUBPUBKDIGEST, CCN_TT_DTAG);
    len += mkHeader(out+len, plen, CCN_TT_BLOB);
    memcpy(out+len, publisher, plen);
    len += plen;
    out[len++] = 0; // end-of-component
    }

    if (scope) {
    k = strlen(scope);
    len += mkHeader(out+len, CCN_DTAG_SCOPE, CCN_TT_DTAG);
    len += mkHeader(out+len, k, CCN_TT_UDATA);
    memcpy(out + len, (unsigned char*)scope, k);
    len += k;
    out[len++] = 0; // end-of-maxsuffcomp
    }

    if (nonce) {
    len += mkHeader(out+len, CCN_DTAG_NONCE, CCN_TT_DTAG);
    len += mkHeader(out+len, sizeof(*nonce), CCN_TT_BLOB);
    memcpy(out+len, (void*)nonce, sizeof(*nonce));
    len += sizeof(*nonce);
    out[len++] = 0; // end-of-nonce
    }

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
    int thunk_request = 0;
    char *prefix[CCNL_MAX_NAME_COMP];

    while ((opt = getopt(argc, argv, "hptu:w:x:")) != -1) {
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
        case 't':
            thunk_request = 1;
            break;
        case 'h':
        default:
Usage:
        fprintf(stderr, "usage: %s "
        "[-u host/port] [-x ux_path_name] [-w timeout] COMPUTATION URI\n"
        "  -u a.b.c.d/port  UDP destination (default is 127.0.0.1/9695)\n"
        "  -x ux_path_name  UNIX IPC: use this instead of UDP\n"
        "  -w timeout       in sec (float)\n"
        "  -p               print interest on console and exit\n",
        argv[0]);
        exit(1);
        }
    }
    if (!argv[optind]) {
        goto Usage;
    }
    cp = strtok(argv[optind], "|");

    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
        prefix[i++] = cp;
        cp = strtok(NULL, "|");
    }
    prefix[i] = NULL;
        if(packettype == 0){
        len = ccnb_mkInterest(prefix,
             minSuffix, maxSuffix,
             digest, dlen,
             publisher, plen,
             scope, &nonce,
             out);
    }
    else if(packettype ==1){
        printf("Not Implemented yet\n");
    }
    else if(packettype == 2){
        len = ndntlv_mkInterest(prefix, (int*)&nonce, out, CCNL_MAX_PACKET_SIZE);
    }

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

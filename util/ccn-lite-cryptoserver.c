/*
 * @f util/ccn-lite-cryptoserver.c
 * @b cryptoserver for functions for the CCN-lite
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
 * 2013-07-22 created
 */


#include "../ccnl.h"
#include "../ccnx.h"
#include "ccnl-common.c"
#include "ccnl-crypto.c"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/un.h>

char *ux_path, *private_key, *ctrl_public_key;

int
ccnl_open_unixpath(char *path, struct sockaddr_un *ux)
{
  int sock, bufsize;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("opening datagram socket");
	return -1;
    }

    unlink(path);
    ux->sun_family = AF_UNIX;
    strcpy(ux->sun_path, path);

    if (bind(sock, (struct sockaddr *) ux, sizeof(struct sockaddr_un))) {
        perror("binding name to datagram socket");
	close(sock);
        return -1;
    }

    bufsize = 4 * CCNL_MAX_PACKET_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    return sock;

}

int 
get_tag_content(unsigned char **buf, int *len, char *content){
    int num = 0;
    while((**buf) !=  0)
    {
        content[num] = **buf;
        ++(*buf); --(*len);
        ++num;
    }
    ++(*buf); --(*len);
    return num;
}

int
handle_verify(char **buf, int *buflen){
    int num, typ, txid, verified;
    int contentlen = 0;
    int siglen = 0;
    char *txid_s = 0, *hash = 0, *content = 0;
    
    //open content
    if(dehead(buf, buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if(dehead(buf, buflen, &num, &typ)) goto Bail;
    while (dehead(buf, buflen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end
	extractStr2(txid_s, CCN_DTAG_SEQNO);
        if(!siglen)siglen = *buflen;
	extractStr2(hash, CCN_DTAG_SIGNATURE);
        siglen = siglen - (*buflen + 5);
        contentlen = *buflen;
        extractStr2(content, CCN_DTAG_CONTENTDIGEST);

	if (consume(typ, num, buf, buflen, 0, 0) < 0) goto Bail;
    }
    contentlen = contentlen - (*buflen + 4);
    
    printf("Contentlen: %d\n", contentlen);
    printf("Siglen: %d\n", siglen);
    
    
    printf("TXID:     %s\n", txid_s);
    printf("HASH:     %s\n", hash);
    printf("CONTENT:  %s\n", content);
    
    verified = verify(ctrl_public_key, content, contentlen, hash, siglen);
    printf("Verified: %d\n", verified);
    return verified;
    Bail:
    printf("FOO!\n");
    return 0;
}

int parse_crypto_packet(char *buf, int buflen){
    int num, typ;
    char component[100];
    char type[100];
    char content[64000];
    
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_INTEREST) goto Bail; 
    
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME) goto Bail; 
    
    //check if component ist ccnx
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_COMPONENT) goto Bail; 
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail; 
    memset(component, 0, sizeof(component));
    get_tag_content(&buf, &buflen, component);
    if(strcmp(component, "ccnx")) goto Bail; 
    
    //check if component is crypto
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_COMPONENT) goto Bail; 
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail; 
    memset(component, 0, sizeof(component));
    get_tag_content(&buf, &buflen, component);
    if(strcmp(component, "crypto")) goto Bail;
   
    //open content object
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_COMPONENT) goto Bail; 
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail; 
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail; 
    
    //get msg-type, what to do
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_TYPE) goto Bail; 
    if(dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail; 
    memset(type, 0, sizeof(type));
    get_tag_content(&buf, &buflen, type);
    
    if(!strcmp(type, "verify"))
        handle_verify(&buf, &buflen);
    return 1;
    Bail:
    printf("foo\n");
    return 0;
    
}

int crypto_main_loop(int sock)
{
    //receive packet async and call parse/answer...
    int len;
    char buf[64000];
    struct sockaddr_un src_addr;
    socklen_t addrlen = sizeof(struct sockaddr_un);
    
    
    len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*) &src_addr,
            &addrlen);
    int i;
    //for(i = 0; i < len; ++i)
    //    printf("%c", buf[i]);
    //printf("\n");
    
    parse_crypto_packet(buf, len);
    
    return 1;
}

int main(int argc, char **argv)
{
    struct sockaddr_un ux;
    if(argc < 3) goto Bail;
    ux_path = argv[1];
    ctrl_public_key = argv[2];
    if(argc >= 3)
        private_key = argv[3];
    
    int sock = ccnl_open_unixpath(ux_path, &ux);
    while(crypto_main_loop(sock));
    
    Bail:
    printf("Usage: %s crypto_ux_socket_path"
        " public_key [private_key]\n", argv[0]);
}

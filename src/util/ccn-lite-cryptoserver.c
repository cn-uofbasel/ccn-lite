/*
 * @f util/ccn-lite-cryptoserver.c
 * @b cryptoserver for functions for the CCN-lite
 *
 * Copyright (C) 2013, Christopher Scherb, University of Basel
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
 * 2013-07-22 created <christopher.scherb@unibas.ch>
 */

#define CCNL_UNIX

#define USE_SUITE_CCNB
#define USE_SIGNATURES

#include "ccnl-common.c"
#include "ccnl-crypto.c"

// ----------------------------------------------------------------------

char *ux_path, *private_key, *ctrl_public_key;

int ux_sendto(int sock, char *topath, unsigned char *data, int len)
{
    struct sockaddr_un name;
    int rc;

    /* Construct name of socket to send to. */
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, topath);

    /* Send message. */
    rc = sendto(sock, data, len, 0, (struct sockaddr*) &name,
                sizeof(struct sockaddr_un));
    if (rc < 0) {
      fprintf(stderr, "\tnamed pipe \'%s\'\n", topath);
      perror("\tsending datagram message");
    }
    return rc;
}

int
ccnl_crypto_ux_open(char *frompath)
{
  int sock, bufsize;
    struct sockaddr_un name;

    /* Create socket for sending */
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("\topening datagram socket");
        exit(1);
    }
    unlink(frompath);
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, frompath);
    if (bind(sock, (struct sockaddr *) &name,
             sizeof(struct sockaddr_un))) {
        perror("\tbinding name to datagram socket");
        exit(1);
    }
//    printf("socket -->%s\n", NAME);

    bufsize = 4 * CCNL_MAX_PACKET_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    return sock;
}

int
ccnl_crypto_get_tag_content(unsigned char **buf, int *len, char *content, int contentlen){
    int num = 0;
    memset(content,0,contentlen);
    while((**buf) !=  0 && num < contentlen)
    {
        content[num] = **buf;
        ++(*buf); --(*len);
        ++num;
    }
    ++(*buf); --(*len);
    return num;
}

int
handle_verify(unsigned char **buf, int *buflen, int sock, char *callback){
    int num, typ, verified = 0;
    int contentlen = 0;
    int siglen = 0;
    unsigned char *txid_s = 0, *sig = 0, *content = 0;
    int len = 0, len3 = 0;
    unsigned char *component_buf = 0, *msg = 0;
    char h[1024];

    while (ccnl_ccnb_dehead(buf, buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        extractStr2(txid_s, CCN_DTAG_SEQNO);
        if(!siglen)siglen = *buflen;
        extractStr2(sig, CCN_DTAG_SIGNATURE);
        siglen = siglen - (*buflen + 5);
        contentlen = *buflen;
        extractStr2(content, CCN_DTAG_CONTENTDIGEST);

        if (ccnl_ccnb_consume(typ, num, buf, buflen, 0, 0) < 0) goto Bail;
    }
    contentlen = contentlen - (*buflen + 4);


    printf(" \tHandeling TXID: %s; Type: Verify; Siglen: %d; Contentlen: %d;\n", txid_s, siglen, contentlen);

    verified = verify(ctrl_public_key, content, contentlen, sig, siglen);
    printf("\tResult: Verified: %d\n", verified);

    //return message object
    msg = ccnl_malloc(sizeof(char)*contentlen + 1000);
    component_buf = ccnl_malloc(sizeof(char)*contentlen + 666);

    len = ccnl_ccnb_mkHeader(msg, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // ContentObj

    len += ccnl_ccnb_mkHeader(msg+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += ccnl_ccnb_mkStrBlob(msg+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(msg+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "crypto");
    msg[len++] = 0; // end-of-name

    len3 += ccnl_ccnb_mkStrBlob(component_buf+len3, CCNL_DTAG_CALLBACK, CCN_TT_DTAG, callback);
    len3 += ccnl_ccnb_mkStrBlob(component_buf+len3, CCN_DTAG_TYPE, CCN_TT_DTAG, "verify");
    len3 += ccnl_ccnb_mkStrBlob(component_buf+len3, CCN_DTAG_SEQNO, CCN_TT_DTAG, (char*) txid_s);
    memset(h,0,sizeof(h));
    sprintf(h,"%d", verified);
    len3 += ccnl_ccnb_mkStrBlob(component_buf+len3, CCNL_DTAG_VERIFIED, CCN_TT_DTAG, h);
    len3 += ccnl_ccnb_mkBlob(component_buf + len3, CCN_DTAG_CONTENTDIGEST, CCN_TT_DTAG, (char*) content, contentlen);

    len += ccnl_ccnb_mkBlob(msg+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                            (char*) component_buf, len3);

    msg[len++] = 0; // end-of contentobj

    memset(h,0,sizeof(h));
    sprintf(h,"%s-2", ux_path);
    if(len > CCNL_MAX_PACKET_SIZE) return 0;
    ux_sendto(sock, h, msg, len);
    printf("\t complete, answered to: %s len: %d\n", ux_path, len);
    Bail:
    if(component_buf) ccnl_free(component_buf);
    if(msg) ccnl_free(msg);
    return verified;
}


int
handle_sign(unsigned char **buf, int *buflen, int sock, char *callback){
    int num, typ, ret = 0;
    unsigned int contentlen = 0;
    unsigned int siglen = 0;
    unsigned char *txid_s = 0, *sig = 0, *content = 0;
    int len = 0, len3 = 0;
    unsigned char *component_buf = 0, *msg = 0;
    char h[1024];

    while (ccnl_ccnb_dehead(buf, buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        extractStr2(txid_s, CCN_DTAG_SEQNO);
        contentlen = *buflen;
        extractStr2(content, CCN_DTAG_CONTENTDIGEST);

        if (ccnl_ccnb_consume(typ, num, buf, buflen, 0, 0) < 0) goto Bail;
    }
    contentlen = contentlen - (*buflen + 4);

    printf(" \tHandeling TXID: %s; Type: Sign; Contentlen: %d;\n", txid_s, contentlen);

    sig = ccnl_malloc(sizeof(char)*4096);

    sign(private_key, content, contentlen, sig, &siglen);
    if(siglen <= 0){
        ccnl_free(sig);
        sig = (unsigned char*) "Error";
        siglen = 6;
    }

    //return message object
    msg = ccnl_malloc(sizeof(char)*siglen + sizeof(char)*contentlen  + 1000);
    component_buf = ccnl_malloc(sizeof(char)*siglen + sizeof(char)*contentlen + 666);

    len = ccnl_ccnb_mkHeader(msg, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // ContentObj

    len += ccnl_ccnb_mkHeader(msg+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += ccnl_ccnb_mkStrBlob(msg+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(msg+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "crypto");
    msg[len++] = 0; // end-of-name

    len3 += ccnl_ccnb_mkStrBlob(component_buf+len3, CCNL_DTAG_CALLBACK, CCN_TT_DTAG, callback);
    len3 += ccnl_ccnb_mkStrBlob(component_buf+len3, CCN_DTAG_TYPE, CCN_TT_DTAG, "sign");
    len3 += ccnl_ccnb_mkStrBlob(component_buf+len3, CCN_DTAG_SEQNO, CCN_TT_DTAG, (char*) txid_s);

    len3 += ccnl_ccnb_mkBlob(component_buf+len3, CCN_DTAG_SIGNATURE, CCN_TT_DTAG,  // signature
                   (char*) sig, siglen);
    len3 += ccnl_ccnb_mkBlob(component_buf + len3, CCN_DTAG_CONTENTDIGEST,
                             CCN_TT_DTAG, (char*) content, contentlen);

    len += ccnl_ccnb_mkBlob(msg+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                            (char*) component_buf, len3);



    msg[len++] = 0; // end-o

    memset(h,0,sizeof(h));
    sprintf(h,"%s-2", ux_path);
    if(len > CCNL_MAX_PACKET_SIZE) return 0;
    ux_sendto(sock, h, msg, len);
    printf("\t complete, answered to: %s len: %d\n", ux_path, len);
    Bail:
    if(component_buf) ccnl_free(component_buf);
    if(msg) ccnl_free(msg);
    ret = 1;
    return ret;
}

int parse_crypto_packet(unsigned char *buf, int buflen, int sock){
    int num, typ;
    char component[100];
    char type[100];
    //    char content[CCNL_MAX_PACKET_SIZE];
    char callback[1024];


    printf("Crypto Request... parsing \n");
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_INTEREST) goto Bail;

    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME) goto Bail;

    //check if component ist ccnx
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_COMPONENT) goto Bail;
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    memset(component, 0, sizeof(component));
    ccnl_crypto_get_tag_content(&buf, &buflen, component, sizeof(component));
    if(strcmp(component, "ccnx")) goto Bail;

    //check if component is crypto
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_COMPONENT) goto Bail;
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    memset(component, 0, sizeof(component));
    ccnl_crypto_get_tag_content(&buf, &buflen, component, sizeof(component));
    if(strcmp(component, "crypto")) goto Bail;


    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_COMPONENT) goto Bail;
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
     //open content object
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;


    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCNL_DTAG_CALLBACK) goto Bail;
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    memset(callback, 0, sizeof(callback));
    ccnl_crypto_get_tag_content(&buf, &buflen, callback, sizeof(callback));

    printf("\tCallback function is: %s\n", callback);

    //get msg-type, what to do
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_TYPE) goto Bail;
    if(ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    memset(type, 0, sizeof(type));
    ccnl_crypto_get_tag_content(&buf, &buflen, type, sizeof(type));

    if(!strcmp(type, "verify"))
        handle_verify(&buf, &buflen, sock, callback);

    if(!strcmp(type, "sign"))
        handle_sign(&buf, &buflen, sock, callback);
    return 1;
    Bail:
    printf("\tError occured\n");
    return 0;

}

int crypto_main_loop(int sock)
{
    //receive packet async and call parse/answer...
  int len; //, pid;
    unsigned char buf[CCNL_MAX_PACKET_SIZE];
    struct sockaddr_un src_addr;
    socklen_t addrlen = sizeof(struct sockaddr_un);


    len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*) &src_addr,
            &addrlen);

    //pid = fork();
    //if(pid == 0){
        parse_crypto_packet(buf, len, sock);
    //    _exit(0);
    //}

    return 1;
}

int main(int argc, char **argv)
{
    if(argc < 3) goto Bail;
    ux_path = argv[1];
    ctrl_public_key = argv[2];
    if(argc >= 3)
        private_key = argv[3];

    int sock = ccnl_crypto_ux_open(ux_path);
    while(crypto_main_loop(sock));
    return 0;

    Bail:
    printf("Usage: %s crypto_ux_socket_path"
        " public_key [private_key]\n", argv[0]);
    return -1;
}

/*
 * @f ccnl-ext-crypto.c
 * @b CCN lite extension, crypto logic (sign, verify, encrypt, decrypt)
 *
 * Copyright (C) 2012-13, Christian Tschudin, University of Basel
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
 * 2012-10-03 created
 */
#ifndef CCNL_EXT_CRYPTO_C
#define CCNL_EXT_CRYPTO_C

#ifdef CCNL_USE_MGMT_SIGNATUES
#ifndef CCNL_LINUXKERNEL
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>

#include "ccnl-core.h"
#include "ccnl-ext-debug.c"
#include "ccnx.h"
#endif
#endif /*CCNL_USE_MGMT_SIGNATUES*/



#ifdef CCNL_USE_MGMT_SIGNATUES

/*struct ccnl_pendcrypt_s { // pending interest
    struct ccnl_pendcrypt_s *next, *prev;
    int (*function)(struct ccnl_relay_s *relay, struct ccnl_face_s *from, unsigned char **data, int *datalen);
    struct ccnl_face_s *from;
    unsigned char *data;
    int *datalen;
    int txid;
};

struct ccnl_pendcrypt_s 
*create_ccnl_pendcrypt(int (*function)(struct ccnl_relay_s *relay, struct ccnl_face_s *from, unsigned char **data, int *datalen),
        struct ccnl_face_s *from, unsigned char *data, int datalen, int txid){
    struct ccnl_pendcrypt_s *ret = ccnl_malloc(sizeof(struct ccnl_pendcrypt_s));
    ret->txid = txid;
    ret->datalen = datalen;
    ret->function = function;
    ret->from = (struct ccnl_face_s*) ccnl_malloc(sizeof(struct ccnl_face_s));
    memcpy(ret->from, from, sizeof(struct ccnl_face_s));
    ret->data = (unsigned char*) ccnl_malloc(sizeof(char)*datalen);
    memcpy(ret->data, data, sizeof(sizeof(char)*datalen));
}*/

int create_ccnl_crypto_face(struct ccnl_relay_s *relay, char *ux_path)
{
    sockunion su;
    DEBUGMSG(99, "  adding UNIX face unixsrc=%s\n", ux_path);
    su.sa.sa_family = AF_UNIX;
    strcpy(su.ux.sun_path, (char*) ux_path);
    relay->crypto_face = ccnl_get_face_or_create(relay, -1, &su.sa, sizeof(struct sockaddr_un));
    if(!relay->crypto_face) return 0;
    relay->crypto_face->flags = CCNL_FACE_FLAGS_STATIC;
    
    return 1;
}

int create_ccnl_sign_verify_msg(char *typ, int txid, char *content, int content_len,
        char *sig, int sig_len, char *msg)
{
    int len = 0, len2 = 0, len3 = 0;
    char *component_buf, *contentobj_buf;
    char h[100];
    
    component_buf = ccnl_malloc(sizeof(char)*(content_len+sig_len)+2000);
    contentobj_buf = ccnl_malloc(sizeof(char)*(content_len+sig_len)+1000);
    
    len = mkHeader(msg, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(msg+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(msg+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(msg+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "crypto");
    
    // prepare FACEINSTANCE
    
    memset(h, 0, 100);
    sprintf(h, "%d", txid);
    len3 += mkStrBlob(component_buf+len3, CCN_DTAG_SEQNO, CCN_TT_DTAG, h);
    if(sig && sig_len)
        len3 += mkBlob(component_buf+len3, CCN_DTAG_SIGNATURE, CCN_TT_DTAG,  // content
		   (char*) sig, sig_len);
    else if(strcmp(typ, "sign")) return 0;
    len3 += mkBlob(component_buf+len3, CCN_DTAG_CONTENTDIGEST, CCN_TT_DTAG,  // content
		   (char*) content, content_len);
    
    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj_buf, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkStrBlob(contentobj_buf+len2, CCN_DTAG_TYPE, CCN_TT_DTAG, typ);
    len2 += mkBlob(contentobj_buf+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) component_buf, len3);
    contentobj_buf[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len += mkBlob(msg+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj_buf, len2);

    msg[len++] = 0; // end-of-name
    msg[len++] = 0; // end-o
    
    ccnl_free(component_buf);
    ccnl_free(contentobj_buf);
    
    return len;
}

int sign(struct ccnl_relay_s *ccnl, char *content, int content_len, char *sig, int *sig_len)
{
    
    char *msg = 0; int len;
    struct ccnl_buf_s *retbuf;
    if(!ccnl->crypto_face) return 0;
    //create ccn_msg
    msg = (char *) ccnl_malloc(sizeof(char)*(content_len+*sig_len)+3000);
    
    len = create_ccnl_sign_verify_msg("sign", ccnl->crypto_txid++, content, content_len, 
            NULL, 0, msg);
    
    //send ccn_msg to crytoserver
    retbuf = ccnl_buf_new((char *)msg, len);
    
    ccnl_face_enqueue(ccnl, ccnl->crypto_face, retbuf);
    
    //receive and parse return msg
    
    if(msg) ccnl_free(msg);
    if (retbuf) ccnl_free(retbuf);
    return 1;
    
}

int verify(struct ccnl_relay_s *ccnl, char *content, int content_len, char *sig, int sig_len)
{
    char *msg = 0;
    int verified = 0, len = 0;
    struct ccnl_buf_s *retbuf;
    //if(!ccnl->crypto_face) return verified;
    msg = (char *)ccnl_malloc(sizeof(char)*(content_len+sig_len)+3000);
    //create ccn_msg
    
    DEBUGMSG(99, "CONTENTLEN: %d; SIGLEN: %d\n", content_len, sig_len);
    
    len = create_ccnl_sign_verify_msg("verify", ccnl->crypto_txid++, content, content_len, 
            sig, sig_len, msg);
    
    //send ccn_msg to crytoserver
    retbuf = ccnl_buf_new((char *)msg, len);
    ccnl_face_enqueue(ccnl, ccnl->crypto_face, retbuf);
    
    //receive and parse return msg
    if(msg) ccnl_free(msg);
    if (retbuf) ccnl_free(retbuf);
    
    return verified;
}


int add_signature(unsigned char *out, char *private_key_path, char *file, int fsize)
{
    int len;
    
    unsigned char *sig = (unsigned char *)ccnl_malloc(sizeof(char)*4096);
    int sig_len;

    len = mkHeader(out, CCN_DTAG_SIGNATURE, CCN_TT_DTAG);
    len += mkStrBlob(out + len, CCN_DTAG_NAME, CCN_TT_DTAG, "SHA256");
    len += mkStrBlob(out + len, CCN_DTAG_WITNESS, CCN_TT_DTAG, "");
    
    if(!sign(private_key_path, file, fsize, sig, &sig_len)) return 0;
    //DEBUGMSG(99, "SIGLEN: %d\n",sig_len);
    sig[sig_len]=0;
    
    //add signaturebits bits...
    len += mkHeader(out + len, CCN_DTAG_SIGNATUREBITS, CCN_TT_DTAG);
    len += addBlob(out + len, sig, sig_len);
    out[len++] = 0; // end signaturebits
    
    out[len++] = 0; // end signature
    
    /*char *publickey = "/home/blacksheeep/.ssh/publickey.pem";
    int verified = verify(publickey, file, fsize, sig, sig_len);
    printf("Verified: %d\n", verified);*/
    ccnl_free(sig);
    return len;
}

int
ccnl_crypto(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
	  struct ccnl_prefix_s *prefix, struct ccnl_face_s *from){
    
    DEBUGMSG(99, "ccnl_crypto content\n");
    return 0;
}
#endif /*CCNL_USE_MGMT_SIGNATUES*/
#endif /*CCNL_EXT_CRYPTO_C*/
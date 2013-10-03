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

#ifdef CCNL_USE_MGMT_SIGNATUES
#ifndef CCNL_LINUXKERNEL
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>
#endif
#endif /*CCNL_USE_MGMT_SIGNATUES*/

#ifdef CCNL_USE_MGMT_SIGNATUES
int sha1(void* input, unsigned long length, unsigned char* md)
{
#ifndef CCNL_LINUXKERNEL
    SHA_CTX context;
    if(!SHA1_Init(&context))
        return 0;

    if(!SHA1_Update(&context, (unsigned char*)input, length))
        return 0;

    if(!SHA1_Final(md, &context))
        return 0;

    return 1;
#endif
}

int sign(char* private_key_path, char *msg, int msg_len, char *sig, int *sig_len)
{
    #ifndef CCNL_LINUXKERNEL
    //Load private key
    FILE *fp = fopen(private_key_path, "r");
    if(!fp) {
        printf("Could not find private key\n");
        return 0;
    }
    RSA *rsa = (RSA *) PEM_read_RSAPrivateKey(fp,NULL,NULL,NULL);
    fclose(fp);
    if(!rsa) return 0;
    
    unsigned char md[SHA_DIGEST_LENGTH];
    sha1(msg, msg_len, md);
    
    //Compute signatur
    int err = RSA_sign(NID_sha1, md, SHA_DIGEST_LENGTH, sig, sig_len, rsa);
    if(!err){
        DEBUGMSG(99,"Error: %d\n", ERR_get_error());
    }
    RSA_free(rsa);
    return err;
#else
    return 0;
#endif
}

int verify(char* public_key_path, char *msg, int msg_len, char *sig, int sig_len)
{
    
#ifndef CCNL_LINUXKERNEL
    //Load public key
    FILE *fp = fopen(public_key_path, "r");
    unsigned char md[SHA_DIGEST_LENGTH];
    if(!fp) {
        printf("Could not find public key\n");
        return 0;
    }
    RSA *rsa = (RSA *) PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    if(!rsa) return 0;
    fclose(fp);
    
    //Compute Hash
    
    sha1(msg, msg_len, md);
    //Verify signature
    int verified = RSA_verify(NID_sha1, md, SHA_DIGEST_LENGTH, sig, sig_len, rsa);
    RSA_free(rsa);
    return verified;
#else
    return 0;
#endif
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
#endif /*CCNL_USE_MGMT_SIGNATUES*/
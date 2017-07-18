/*
 * @f util/ccnl-crypto.c
 * @b crypto functions for the CCN-lite utilities
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

// ----------------------------------------------------------------------


#ifdef USE_SIGNATURES
// ----------------------------------------------------------------------
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>
// ----------------------------------------------------------------------
#endif

/*
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
addBlob(unsigned char *out, char *cp, int cnt)
{
    int len;

    len = mkHeader(out, cnt, CCN_TT_BLOB);
    memcpy(out+len, cp, cnt);
    len += cnt;

    return len;
}

int
mkBlob(unsigned char *out, unsigned int num, unsigned int tt,
       char *cp, int cnt)
{
    int len;

    len = mkHeader(out, num, tt);
    len += addBlob(out+len, cp, cnt);
    out[len++] = 0;

    return len;
}

int
mkStrBlob(unsigned char *out, unsigned int num, unsigned int tt,
          char *str)
{
    return mkBlob(out, num, tt, str, strlen(str));
}

*/
#ifdef USE_SIGNATURES
int sha(void* input, unsigned long length, unsigned char* md)
{
    SHA256_CTX context;
    if(!SHA256_Init(&context))
        return 0;

    if(!SHA256_Update(&context, (unsigned char*)input, length))
        return 0;

    if(!SHA256_Final(md, &context))
        return 0;
    return 1;
}

int
sign(char* private_key_path, unsigned char *msg, int msg_len,
     unsigned char *sig, unsigned int *sig_len)
{

    //Load private key
    FILE *fp = fopen(private_key_path, "r");
    if(!fp) {
        DEBUGMSG(ERROR, "Could not find private key\n");
        return 0;
    }
    RSA *rsa = (RSA *) PEM_read_RSAPrivateKey(fp,NULL,NULL,NULL);
    fclose(fp);
    if(!rsa) return 0;

    unsigned char md[SHA256_DIGEST_LENGTH];
    sha(msg, msg_len, md);

    //Compute signatur
    int err = RSA_sign(NID_sha256, md, SHA256_DIGEST_LENGTH, (unsigned char*)sig, (unsigned int*)sig_len, rsa);
    if(!err){
        printf("Error: %ul\n", (unsigned int)ERR_get_error());
    }
    RSA_free(rsa);
    return err;
}

int
verify(char* public_key_path, unsigned char *msg, int msg_len,
       unsigned char *sig, unsigned int sig_len)
{
    //Load public key
    FILE *fp = fopen(public_key_path, "r");
    if(!fp) {
        printf("Could not find public key\n");
        return 0;
    }

    RSA *rsa = (RSA *) PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    if(!rsa) return 0;
    fclose(fp);

    //Compute Hash
    unsigned char md[SHA256_DIGEST_LENGTH];
    sha(msg, msg_len, md);

    //Verify signature
    int verified = RSA_verify(NID_sha256, md, SHA256_DIGEST_LENGTH, (unsigned char*)sig, (unsigned int)sig_len, rsa);
    if(!verified){
        printf("Error: %ul\n", (unsigned int)ERR_get_error());
    }
    RSA_free(rsa);
    return verified;
}

int
add_signature(unsigned char *out, char *private_key_path,
              unsigned char *file, unsigned int fsize)
{
    int len;

    unsigned char sig[2048];
    unsigned int sig_len;

    len = ccnl_ccnb_mkHeader(out, CCN_DTAG_SIGNATURE, CCN_TT_DTAG);
    len += ccnl_ccnb_mkStrBlob(out + len, CCN_DTAG_NAME, CCN_TT_DTAG, "SHA256");
    len += ccnl_ccnb_mkStrBlob(out + len, CCN_DTAG_WITNESS, CCN_TT_DTAG, "");

    if(!sign(private_key_path, (unsigned char*)file, fsize, (unsigned char*)sig, &sig_len)) return 0;
    //printf("SIGLEN: %d\n",sig_len);
    sig[sig_len]=0;

    //add signaturebits bits...
    len += ccnl_ccnb_mkHeader(out + len, CCN_DTAG_SIGNATUREBITS, CCN_TT_DTAG);
    len += ccnl_ccnb_addBlob(out + len, (char*)sig, sig_len);
    out[len++] = 0; // end signaturebits

    out[len++] = 0; // end signature

    /*char *publickey = "/home/blacksheeep/.ssh/publickey.pem";
    int verified = verify(publickey, file, fsize, sig, sig_len);
    printf("Verified: %d\n", verified);*/

    return len;
}
#endif /*USE_SIGNATURES*/

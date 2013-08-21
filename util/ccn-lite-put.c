/*
 * @f util/ccn-lite-peek.c
 * @b request content: send an interest, wait for reply, output to stdout
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
 * 2013-04-06  created
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "../ccnx.h"
#include "../ccnl.h"

#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>

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

int
split_string(char *in, char c, char *out)
{
    
    int i = 0, j = 0;
    if(!in[0]) return 0;
    if(in[0] == c) ++i;
    while(in[i] != c)
    {
        if(in[i] == 0) break;
        out[j] = in[i];
        ++i; ++j;
        
    }
    out[j] = 0;
    return i;  
}

time_t
get_mtime(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        perror(path);
        exit(1);
    }
    return statbuf.st_mtime;
}

int
add_ccnl_name(unsigned char *out, char *ccn_path)
{
    char comp[256];
    int len = 0, len2 = 0;
    int h;
    memset(comp, 0 , 256);
    len += mkHeader(out + len, CCN_DTAG_NAME, CCN_TT_DTAG);
    while( (h = split_string(ccn_path + len2, '/', comp)) )
    {   
        len2 += h;
        len += mkStrBlob(out + len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, comp);
        memset(comp, 0 , 256);
    }
    out[len++] = 0;
    return len;
}

int sha1(void* input, unsigned long length, unsigned char* md)
{
    SHA_CTX context;
    if(!SHA1_Init(&context))
        return 0;

    if(!SHA1_Update(&context, (unsigned char*)input, length))
        return 0;

    if(!SHA1_Final(md, &context))
        return 0;

    return 1;
}

int sign(char* private_key_path, char *msg, int msg_len, char *sig, int *sig_len)
{

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
        printf("Error: %d\n", ERR_get_error());
    }
    RSA_free(rsa);
    return err;
}

int verify(char* public_key_path, char *msg, int msg_len, char *sig, int sig_len)
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
    unsigned char md[SHA_DIGEST_LENGTH];
    sha1(msg, msg_len, md);
    
    //Verify signature
    int verified = RSA_verify(NID_sha1, md, SHA_DIGEST_LENGTH, sig, sig_len, rsa);
    if(!verified){
        printf("Error: %d\n", ERR_get_error());
    }
    RSA_free(rsa);
    return verified;
}

int add_signature(unsigned char *out, char *private_key_path, char *file, int fsize)
{
    int len;
    
    unsigned char sig[1024];
    int sig_len;

    len = mkHeader(out, CCN_DTAG_SIGNATURE, CCN_TT_DTAG);
    len += mkStrBlob(out + len, CCN_DTAG_NAME, CCN_TT_DTAG, "SHA1");
    len += mkStrBlob(out + len, CCN_DTAG_WITNESS, CCN_TT_DTAG, "");
    if(!sign(private_key_path, file, fsize, sig, &sig_len)) return 0;
    sig[sig_len]=0;
    len += mkStrBlob(out + len, CCN_DTAG_SIGNATUREBITS, CCN_TT_DTAG, sig);
    out[len++] = 0;
    
    char *publickey = "/home/blacksheeep/.ssh/publickey.pem";
    int verified = verify(publickey, file, fsize, sig, sig_len);
    printf("Done: %d\n", verified);
    
    return len;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char *out, *file;
    char *ux_path;
    char *file_uri;
    char *ccn_path;
    char *new_file_uri;
    char *private_key_path;
    int fsize, len, siglen;
    
    if(argc < 5) goto Usage;
    ux_path = argv[1];
    file_uri = argv[2];
    ccn_path = argv[3];
    private_key_path = argv[4];
    
    FILE *f = fopen(file_uri, "r");
    if(!f) goto Usage;
    
    //size vom file abmessen
    fseek(f, 0L, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    file = (unsigned char * ) malloc(sizeof(unsigned char)*fsize);
    fread(file, fsize, 1, f);
    fclose(f);
    out = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 1000);
    //header vor das file hängen
    len = mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);
    
    siglen = add_signature(out + len, private_key_path, file, fsize);
    if(!siglen)
    {
        printf("Could sign message\n");
    }
    
    len += siglen;
    len += add_ccnl_name(out + len, ccn_path);
    
    len += mkStrBlob(out + len, CCN_DTAG_CONTENT, CCN_TT_DTAG, (char *) file);
    //null ans ende hängen
    out[len++] = 0;
    //write file
    new_file_uri = (char *) malloc(sizeof(char)*1000);
    sprintf(new_file_uri, "%s.ccnb", file_uri);
    FILE *f2 = fopen(new_file_uri, "w");
    if(!f2) goto Usage;
    fwrite(out, 1L, len, f2);
    
    fclose(f2);
    free(out);
    free(file);
    return 0;
    
Usage:
    fprintf(stderr, "usage: %s " 
    "UX_SOCK_PATH_NAME URI CCN-PATH PRIVATE-KEY-PATH\n",
    argv[0]);
    exit(1);
    return 0; // avoid a compiler warning
}

// eof

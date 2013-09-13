/*
 * @f util/ccn-lite-mkC.c
 * @b CLI mkContent, write to Stdout
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
 * 2013-07-06  created
 */

#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h> // htonl()
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../ccnx.h"
#include "../ccnl.h"

// ----------------------------------------------------------------------

char *private_key_path; 
char *witness;

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

#ifdef CCNL_USE_MGMT_SIGNATUES
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>
#endif /*CCNL_USE_MGMT_SIGNATUES*/

// ----------------------------------------------------------------------


#ifdef CCNL_USE_MGMT_SIGNATUES
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
    int len, i;
    
    unsigned char sig[2048];
    int sig_len;

    len = mkHeader(out, CCN_DTAG_SIGNATURE, CCN_TT_DTAG);
    len += mkStrBlob(out + len, CCN_DTAG_NAME, CCN_TT_DTAG, "SHA1");
    if(!witness)witness = "";
    len += mkStrBlob(out + len, CCN_DTAG_WITNESS, CCN_TT_DTAG, witness);
    
    if(!sign(private_key_path, file, fsize, sig, &sig_len)) return 0;
    //printf("SIGLEN: %d\n",sig_len);
    sig[sig_len]=0;
    
    //add signaturebits bits... 
    
    len += mkHeader(out + len, CCN_DTAG_SIGNATUREBITS, CCN_TT_DTAG);
    len += addBlob(out + len, sig, sig_len);
    out[len++] = 0; // end signaturebits
    
    out[len++] = 0; // end signature
    
    /*char *publickey = "/home/blacksheeep/.ssh/publickey.pem";
    int verified = verify(publickey, file, fsize, sig, sig_len);
    printf("Verified: %d\n", verified);*/
    
    return len;
}
#endif /*CCNL_USE_MGMT_SIGNATUES*/

int
hex2int(char c)
{
    if (c >= '0' && c <= '9')
	return c - '0';
    c = tolower(c);
    if (c >= 'a' && c <= 'f')
	return c - 'a' + 0x0a;
    return 0;
}

int
unescape_component(unsigned char *comp) // inplace, returns len after shrinking
{
    unsigned char *in = comp, *out = comp;
    int len;

    for (len = 0; *in; len++) {
	if (in[0] != '%' || !in[1] || !in[2]) {
	    *out++ = *in++;
	    continue;
	}
	*out++ = hex2int(in[1])*16 + hex2int(in[2]);
	in += 3;
    }
    return len;
}

int
mkContent(char **namecomp,
	  unsigned char *publisher, int plen,
	  unsigned char *body, int blen,
	  unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // interest

    // add signature
    if(private_key_path)
        len += add_signature(out+len, private_key_path, body, blen);  

    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    while (*namecomp) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	k = unescape_component((unsigned char*) *namecomp);
	len += mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, *namecomp++, k);
	len += k;
	out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name

    if (publisher) {
	struct timeval t;
	unsigned char tstamp[6];
	uint32_t *sec;
	uint16_t *secfrac;

	gettimeofday(&t, NULL);
	sec = (uint32_t*)(tstamp + 0); // big endian
	*sec = htonl(t.tv_sec);
	secfrac = (uint16_t*)(tstamp + 4);
	*secfrac = htons(4048L * t.tv_usec / 1000000);
	len += mkHeader(out+len, CCN_DTAG_TIMESTAMP, CCN_TT_DTAG);
	len += mkHeader(out+len, sizeof(tstamp), CCN_TT_BLOB);
	memcpy(out+len, tstamp, sizeof(tstamp));
	len += sizeof(tstamp);
	out[len++] = 0; // end-of-timestamp

	len += mkHeader(out+len, CCN_DTAG_SIGNEDINFO, CCN_TT_DTAG);
	len += mkHeader(out+len, CCN_DTAG_PUBPUBKDIGEST, CCN_TT_DTAG);
	len += mkHeader(out+len, plen, CCN_TT_BLOB);
	memcpy(out+len, publisher, plen);
	len += plen;
	out[len++] = 0; // end-of-publisher
	out[len++] = 0; // end-of-signedinfo
    }

    len += mkHeader(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len += mkHeader(out+len, blen, CCN_TT_BLOB);
    memcpy(out + len, body, blen);
    len += blen;
    out[len++] = 0; // end-of-content

    out[len++] = 0; // end-of-contentobj

    return len;
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char body[64*1024];
    unsigned char out[65*1024];
    unsigned char *publisher = 0;
    char *infname = 0, *outfname = 0;
    int i = 0, f, len, opt, plen;
    char *prefix[CCNL_MAX_NAME_COMP], *cp;
    
    private_key_path = 0;
    witness = 0;

    while ((opt = getopt(argc, argv, "hi:o:p:k:w:")) != -1) {
        switch (opt) {
        case 'i':
	    infname = optarg;
	    break;
        case 'o':
	    outfname = optarg;
	    break;
        case 'k':
            private_key_path = optarg;
            break;
        case 'w':
            witness = optarg;
            break;
        case 'p':
	    publisher = (unsigned char*)optarg;
	    plen = unescape_component(publisher);
	    if (plen != 32) {
		fprintf(stderr,
		 "publisher key digest has wrong length (%d instead of 32)\n",
		 plen);
		exit(-1);
	    }
	    break;
        case 'h':
        default:
Usage:
	    fprintf(stderr, "usage: %s [options] URI\n"
	    "  -i FNAME   input file (instead of stdin)\n"
	    "  -o FNAME   output file (instead of stdout)\n"
	    "  -p DIGEST  publisher fingerprint\n"
            "  -k FNAME   publisher private key\n"  
            "  -w STRING  witness\n"       ,
	    argv[0]);
	    exit(1);
        }
    }

    if (!argv[optind]) 
	goto Usage;
    cp = strtok(argv[optind], "/");
    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
	prefix[i++] = cp;
	cp = strtok(NULL, "/");
    }
    prefix[i] = NULL;

    if (infname) {
	f = open(infname, O_RDONLY);
      if (f < 0)
	perror("file open:");
    } else
      f = 0;
    len = read(f, body, sizeof(body));
    close(f);

    len = mkContent(prefix, publisher, plen, body, len, out);

    if (outfname) {
	f = creat(outfname, 0666);
      if (f < 0)
	perror("file open:");
    } else
      f = 1;
    write(f, out, len);
    close(f);

    return 0;
}

// eof

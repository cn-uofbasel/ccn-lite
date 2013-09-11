/*
 * @f util/ccn-lite-ctrl.c
 * @b control utility to steer a ccn-lite relay via UNIX sockets
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
 * 2012-06-01  created
 * 2013-07     <christopher.scherb@unibas.ch> heavy reworking and parsing
 *             of return message
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "../ccnx.h"
#include "../ccnl.h"

#include "ccnl-common.c"
#include "../ccnl-pdu.c"

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
    len += mkStrBlob(out + len, CCN_DTAG_WITNESS, CCN_TT_DTAG, "");
    
    if(!sign(private_key_path, file, fsize, sig, &sig_len)) return 0;
    printf("SIGLEN: %d\n",sig_len);
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

int
mkDebugRequest(unsigned char *out, char *dbg)
{
    int len = 0, len2, len3;
    unsigned char contentobj[2000];
    unsigned char stmt[1000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "debug");

    // prepare debug statement
    len3 = mkHeader(stmt, CCNL_DTAG_DEBUGREQUEST, CCN_TT_DTAG);
    len3 += mkStrBlob(stmt+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, "debug");
    len3 += mkStrBlob(stmt+len3, CCNL_DTAG_DEBUGACTION, CCN_TT_DTAG, dbg);
    stmt[len3++] = 0; // end-of-debugstmt

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) stmt, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

//    ccnl_prefix_free(p);
    return len;
}

int
mkNewEthDevRequest(unsigned char *out, char *devname, char *ethtype,
		   char *frag, char *flags)
{
    int len = 0, len2, len3;
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newdev");

    // prepare DEVINSTANCE
    len3 = mkHeader(faceinst, CCNL_DTAG_DEVINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, "newdev");
    if (devname)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_DEVNAME, CCN_TT_DTAG,
			  devname);
    if (ethtype)
	len3 += mkStrBlob(faceinst+len3, CCN_DTAG_PORT, CCN_TT_DTAG, ethtype);
    if (frag)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, frag);
    if (flags)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG, flags);
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) faceinst, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}


int
mkNewUDPDevRequest(unsigned char *out, char *ip4src, char *port,
		   char *frag, char *flags)
{
    int len = 0, len2, len3;
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newdev");

    // prepare DEVINSTANCE
    len3 = mkHeader(faceinst, CCNL_DTAG_DEVINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, "newdev");
    if (ip4src)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_IP4SRC, CCN_TT_DTAG, ip4src);
    if (port)
	len3 += mkStrBlob(faceinst+len3, CCN_DTAG_PORT, CCN_TT_DTAG, port);
    if (frag)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, frag);
    if (flags)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG, flags);
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) faceinst, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}

int
mkDestroyDevRequest(unsigned char *out, char *faceid)
{
    return 0;
}

int
mkNewFaceRequest(unsigned char *out, char *macsrc, char *ip4src,
		 char *host, char *port, char *flags)
{
    int len = 0, len2, len3;
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newface");

    // prepare FACEINSTANCE
    len3 = mkHeader(faceinst, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, "newface");
    if (macsrc)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_MACSRC, CCN_TT_DTAG, macsrc);
    if (ip4src) {
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_IP4SRC, CCN_TT_DTAG, ip4src);
        len3 += mkStrBlob(faceinst+len3, CCN_DTAG_IPPROTO, CCN_TT_DTAG, "17");
    }
    if (host)
	len3 += mkStrBlob(faceinst+len3, CCN_DTAG_HOST, CCN_TT_DTAG, host);
    if (port)
	len3 += mkStrBlob(faceinst+len3, CCN_DTAG_PORT, CCN_TT_DTAG, port);
    /*
    if (frag)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, frag);
    */
    if (flags)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG, flags);
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) faceinst, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}


int
mkNewUNIXFaceRequest(unsigned char *out, char *path, char *flags)
{
    int len = 0, len2, len3;
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newface");

    // prepare FACEINSTANCE
    len3 = mkHeader(faceinst, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, "newface");
    if (path)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_UNIXSRC, CCN_TT_DTAG, path);
    /*
    if (frag)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, frag);
    */
    if (flags)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG, flags);
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) faceinst, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}


int
mkDestroyFaceRequest(unsigned char *out, char *faceid)
{
    int len = 0, len2, len3;
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];
//    char num[20];

//    sprintf(num, "%d", faceID);

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "destroyface");

    // prepare FACEINSTANCE
    len3 = mkHeader(faceinst, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, "destroyface");
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, faceid);
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) faceinst, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}


int
mkSetfragRequest(unsigned char *out, char *faceid, char *frag, char *mtu)
{
    int len = 0, len2, len3;
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];
//    char num[20];

//    sprintf(num, "%d", faceID);

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "setfrag");

    // prepare FACEINSTANCE
    len3 = mkHeader(faceinst, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, "setfrag");
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, faceid);
    len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, frag);
    len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_MTU, CCN_TT_DTAG, mtu);
    faceinst[len3++] = 0; // end-of-faceinst

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) faceinst, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}


// ----------------------------------------------------------------------

int
mkPrefixregRequest(unsigned char *out, char reg, char *path, char *faceid)
{
    int len = 0, len2, len3;
    unsigned char contentobj[2000];
    unsigned char fwdentry[2000];
    char *cp;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,
		     reg ? "prefixreg" : "prefixunreg");

    // prepare FWDENTRY
    len3 = mkHeader(fwdentry, CCN_DTAG_FWDINGENTRY, CCN_TT_DTAG);
    len3 += mkStrBlob(fwdentry+len3, CCN_DTAG_ACTION, CCN_TT_DTAG,
		      reg ? "prefixreg" : "prefixunreg");
    len3 += mkHeader(fwdentry+len3, CCN_DTAG_NAME, CCN_TT_DTAG); // prefix

    cp = strtok(path, "/");
    while (cp) {
	len3 += mkBlob(fwdentry+len3, CCN_DTAG_COMPONENT, CCN_TT_DTAG,
		       cp, strlen(cp));
	cp = strtok(NULL, "/");
    }
    fwdentry[len3++] = 0; // end-of-prefix
    len3 += mkStrBlob(fwdentry+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, faceid);
    fwdentry[len3++] = 0; // end-of-fwdentry

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) fwdentry, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

//    ccnl_prefix_free(p);
    return len;
}

int
createCCNXFile(char *file_uri, char *ccn_path, char *private_key_path)
{
    long fsize, len, siglen;
    char *out, *file, *new_file_uri;
    FILE *f = fopen(file_uri, "r");
    if(!f) return 0;
    
    //size vom file abmessen
    fseek(f, 0L, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    file = (unsigned char * ) malloc(sizeof(unsigned char)*fsize);
    fread(file, fsize, 1, f);
    fclose(f);
    out = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 1000);

    
    len = mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG); //content object
    
#ifdef CCNL_USE_MGMT_SIGNATUES
    siglen = add_signature(out + len, private_key_path, file, fsize);
    if(!siglen)
    {
        printf("Could sign message\n");
    }
    len += siglen;
#endif /*CCNL_USE_MGMT_SIGNATUES*/
    
    len += add_ccnl_name(out + len, ccn_path);
    
    len += mkStrBlob(out + len, CCN_DTAG_CONTENT, CCN_TT_DTAG, (char *) file);

    out[len++] = 0; //content object end
    
    //write file
    new_file_uri = (char *) malloc(sizeof(char)*1024);
    sprintf(new_file_uri, "%s.ccnb", file_uri);
    FILE *f2 = fopen(new_file_uri, "w");
    if(!f2) return 0;
    fwrite(out, 1L, len, f2);
    fclose(f2);
    free(out);
    free(file);
    
    return 1;
}

int
mkAddToRelayCacheRequest(unsigned char *out, char *file_uri, char *private_key_path)
{
    long len = 0, len2 = 0, len3 = 0;
    long fsize;
    char *ccnb_file, *contentobj, *stmt;
    FILE *f = fopen(file_uri, "r");
    if(!f) return 0;
    
    //determine size of the file
    fseek(f, 0L, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    ccnb_file = (unsigned char *) malloc(sizeof(unsigned char)*fsize);
    fread(ccnb_file, fsize, 1, f);
    fclose(f);
   
    //Create ccn-lite-ctrl interest object with signature to add content...
    //out = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 5000);
    contentobj = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 4000);
    stmt = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 1000);
    
    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "addcacheobject");
    
    //add content to interest...
    len3 += mkHeader(stmt+len3, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len3 += addBlob(stmt+len3, ccnb_file, fsize);
    stmt[len3++] = 0; // end content
    
    len2 += mkHeader(contentobj+len2, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
#ifdef CCNL_USE_MGMT_SIGNATUES
    if(private_key_path) len2 += add_signature(contentobj+len2, private_key_path, stmt, len3);
#endif /*CCNL_USE_MGMT_SIGNATUES*/
 
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) stmt, len3);
    contentobj[len2++] = 0; // end-of-contentobj
    
    
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);
    
    out[len++] = 0; //name end
    out[len++] = 0; //interest end
    
    Bail:
    free(ccnb_file);
    free(contentobj);
    free(stmt);
    return len;
}

int
mkRemoveFormRelayCacheRequest(unsigned char *out, char *ccn_path, char *private_key_path){
    
    int len = 0, len2 = 0, len3 = 0;
    
    unsigned char contentobj[2000];
    unsigned char stmt[1000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    //signatur nach hier, über den rest
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "removecacheobject");

    // prepare debug statement
    //len3 = mkHeader(stmt, CCNL_DTAG_DEBUGREQUEST, CCN_TT_DTAG);
    len3 += add_ccnl_name(stmt+len3, ccn_path);
    
    //stmt[len3++] = 0; // end-of-debugstmt

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    
#ifdef CCNL_USE_MGMT_SIGNATUES
    if(private_key_path)len2 += add_signature(contentobj+len2, private_key_path, stmt, len3);
#endif 
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) stmt, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    
    
    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

//    ccnl_prefix_free(p);
    
    return len;
 
}

// ----------------------------------------------------------------------

int
ux_open(char *frompath)
{
  int sock, bufsize;
    struct sockaddr_un name;

    /* Create socket for sending */
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
	perror("opening datagram socket");
	exit(1);
    }
    unlink(frompath);
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, frompath);
    if (bind(sock, (struct sockaddr *) &name,
	     sizeof(struct sockaddr_un))) {
	perror("binding name to datagram socket");
	exit(1);
    }
//    printf("socket -->%s\n", NAME);

    bufsize = 4 * CCNL_MAX_PACKET_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    return sock;
}

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
      fprintf(stderr, "named pipe \'%s\'\n", topath);
      perror("sending datagram message");
    }
    return rc;
}

int
main(int argc, char *argv[])
{
    char mysockname[200], *progname=argv[0];
    char *ux = CCNL_DEFAULT_UNIXSOCKNAME;
    unsigned char out[64000];
    int len;
    int sock = 0;
    
    char *unix_socket_path;
    char *file_uri;
    char *ccn_path;
    char *private_key_path = 0;
    
    if (argv[1] && !strcmp(argv[1], "-x") && argc > 2) {
	ux = argv[2];
	argv += 2;
	argc -= 2;
    }

    if (argc < 2) goto Usage;

    // socket for receiving
    sprintf(mysockname, "/tmp/.ccn-light-ctrl-%d.sock", getpid());
    sock = ux_open(mysockname);
    if (!sock) {
	fprintf(stderr, "cannot open UNIX receive socket\n");
	exit(-1);
    }

    if (!strcmp(argv[1], "debug")) {
	if (argc < 3)  goto Usage;
	len = mkDebugRequest(out, argv[2]);
    } else if (!strcmp(argv[1], "newETHdev")) {
	if (argc < 3)  goto Usage;
	len = mkNewEthDevRequest(out, argv[2],
				 argc > 3 ? argv[3] : "0x88b5",
				 argc > 4 ? argv[4] : "0",
				 argc > 5 ? argv[5] : "0");
    } else if (!strcmp(argv[1], "newUDPdev")) {
	if (argc < 3)  goto Usage;
	len = mkNewUDPDevRequest(out, argv[2],
				 argc > 3 ? argv[3] : "9695",
				 argc > 4 ? argv[4] : "0",
				 argc > 5 ? argv[5] : "0");
    } else if (!strcmp(argv[1], "destroydev")) {
	if (argc < 3) goto Usage;
	len = mkDestroyDevRequest(out, argv[2]);
    } else if (!strcmp(argv[1], "newETHface")||!strcmp(argv[1], "newUDPface")) {
	if (argc < 5)  goto Usage;
	len = mkNewFaceRequest(out,
			       !strcmp(argv[1], "newETHface") ? argv[2] : NULL,
			       !strcmp(argv[1], "newUDPface") ? argv[2] : NULL,
			       argv[3], argv[4],
			       argc > 5 ? argv[5] : "0x0001");
    } else if (!strcmp(argv[1], "newUNIXface")) {
	if (argc < 3)  goto Usage;
	len = mkNewUNIXFaceRequest(out, argv[2],
				   argc > 3 ? argv[3] : "0x0001");
    } else if (!strcmp(argv[1], "setfrag")) {
	if (argc < 5)  goto Usage;
	len = mkSetfragRequest(out, argv[2], argv[3], argv[4]);
    } else if (!strcmp(argv[1], "destroyface")) {
	if (argc < 3) goto Usage;
	len = mkDestroyFaceRequest(out, argv[2]);
    } else if (!strcmp(argv[1], "prefixreg")) {
	if (argc < 4) goto Usage;
	len = mkPrefixregRequest(out, 1, argv[2], argv[3]);
    } else if (!strcmp(argv[1], "prefixunreg")) {
	if (argc < 4) goto Usage;
	len = mkPrefixregRequest(out, 0, argv[2], argv[3]);
    } else if (!strcmp(argv[1], "add")){
        if(argc < 3) goto Usage;
        file_uri = argv[2];  
        if(argc > 3)private_key_path = argv[3];
        len = mkAddToRelayCacheRequest(out, file_uri, private_key_path);
    } else if(!strcmp(argv[1], "remove")){
        if(argc < 3) goto Usage;
        ccn_path = argv[2];  
        if(argc > 3)private_key_path = argv[3];
        len = mkRemoveFormRelayCacheRequest(out, ccn_path, private_key_path);
    } else if(!strcmp(argv[1], "create"))
    {
        if(argc < 5) goto Usage;
        file_uri = argv[2];
        ccn_path = argv[3];
        private_key_path = argv[4];
        if(!createCCNXFile(file_uri, ccn_path, private_key_path)) goto Usage;
        return 0;
    } else{
	printf("unknown command %s\n", argv[1]);
	goto Usage;
    }

    if (len > 0) {
	ux_sendto(sock, ux, out, len);

//	sleep(1);
	len = recv(sock, out, sizeof(out), 0);
	if (len > 0)
	    out[len] = '\0';

	write(1, out, len);
        printf("\n");
        
	printf("received %d bytes.\n", len);
    } else
	printf("nothing to send, program terminates\n");

    close(sock);
    unlink(mysockname);

    return 0;

Usage:
    fprintf(stderr, "usage: %s [-x ux_path] CMD, where CMD either of\n"
	   "  newETHdev     DEVNAME [ETHTYPE [FRAG [DEVFLAGS]]]\n"
	   "  newUDPdev     IP4SRC|any [PORT [FRAG [DEVFLAGS]]]\n"
	   "  destroydev    DEVNDX\n"
	   "  newETHface    MACSRC|any MACDST ETHTYPE [FACEFLAGS]\n"
	   "  newUDPface    IP4SRC|any IP4DST PORT [FACEFLAGS]\n"
	   "  newUNIXface   PATH [FACEFLAGS]\n"
           "  setfrag       FACEID FRAG MTU\n"
	   "  destroyface   FACEID\n"
	   "  prefixreg     PREFIX FACEID\n"
	   "  prefixunreg   PREFIX FACEID\n"
	   "  debug         dump\n"
	   "  debug         halt\n"
	   "  debug         dump+halt\n"
           "  add           ccn-file [private-key]\n"
           "  remove        ccn-path [private-key]\n"
           "  create        file ccn-path private-key\n"
	   "where FRAG in none, seqd2012, ccnx2013\n",
	progname);

    if (sock) {
	close(sock);
	unlink(mysockname);
    }
    return -1;
}

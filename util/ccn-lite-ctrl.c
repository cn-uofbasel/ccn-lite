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

#define USE_SIGNATURES
#define USE_SUITE_CCNB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../ccnl.h"

#include "ccnl-common.c"
#include "../pkt-ccnb.h"
#include "../pkt-ccnb-dec.c"
#include "../pkt-ccnb-enc.c"

#include "ccnl-crypto.c"

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
mkDebugRequest(unsigned char *out, char *dbg, char *private_key_path)
{
    int len = 0, len1 = 0, len2 = 0, len3 = 0;
    unsigned char out1[CCNL_MAX_PACKET_SIZE];
    unsigned char contentobj[2000];
    unsigned char stmt[1000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "debug");

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
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

//    ccnl_prefix_free(p);
    return len;
}

int
mkNewEthDevRequest(unsigned char *out, char *devname, char *ethtype,
		   char *frag, char *flags, char *private_key_path)
{
    int len = 0, len1 = 0, len2 = 0, len3 = 0;
    unsigned char out1[CCNL_MAX_PACKET_SIZE];
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newdev");

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
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}


int
mkNewUDPDevRequest(unsigned char *out, char *ip4src, char *port,
		   char *frag, char *flags, char *private_key_path)
{
    int len = 0, len1 = 0, len2 = 0, len3 = 0;
    unsigned char out1[CCNL_MAX_PACKET_SIZE];
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newdev");

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
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}

int
mkDestroyDevRequest(unsigned char *out, char *faceid, char *private_key_path)
{
    return 0;
}

int
mkNewFaceRequest(unsigned char *out, char *macsrc, char *ip4src,
		 char *host, char *port, char *flags, char *private_key_path)
{
    int len = 0, len1 = 0, len2 = 0, len3 = 0;
    unsigned char out1[CCNL_MAX_PACKET_SIZE];
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newface");

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
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}


int
mkNewUNIXFaceRequest(unsigned char *out, char *path, char *flags, char *private_key_path)
{
    int len = 0, len1 = 0, len2 = 0, len3 = 0;
    unsigned char out1[CCNL_MAX_PACKET_SIZE];
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 = mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newface");

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
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}


int
mkDestroyFaceRequest(unsigned char *out, char *faceid, char *private_key_path)
{
    int len = 0, len1 = 0, len2 = 0, len3 = 0;
    unsigned char out1[CCNL_MAX_PACKET_SIZE];
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];
//    char num[20];

//    sprintf(num, "%d", faceID);

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "destroyface");

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
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest
    
    return len;
}


int
mkSetfragRequest(unsigned char *out, char *faceid, char *frag, char *mtu, char *private_key_path)
{
    int len = 0, len1 = 0, len2 = 0, len3 = 0;
    unsigned char out1[CCNL_MAX_PACKET_SIZE];
    unsigned char contentobj[2000];
    unsigned char faceinst[2000];
//    char num[20];

//    sprintf(num, "%d", faceID);

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "setfrag");

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
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;
}


// ----------------------------------------------------------------------

int
mkPrefixregRequest(unsigned char *out, char reg, char *path, char *faceid, char *private_key_path)
{
    int len = 0, len1 = 0, len2 = 0, len3 = 0;
    unsigned char out1[CCNL_MAX_PACKET_SIZE];
    unsigned char contentobj[2000];
    unsigned char fwdentry[2000];
    char *cp;

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,
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
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest
    
//    ccnl_prefix_free(p);
    return len;
}

int
mkAddToRelayCacheRequest(unsigned char *out, char *file_uri, char *private_key_path)
{
    long len = 0, len1 = 0, len2 = 0, len3 = 0;
    long fsize;
    unsigned char *ccnb_file, *contentobj, *stmt, *out1;
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
    out1 = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 5000);
    contentobj = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 4000);
    stmt = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 1000);
    
    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "addcacheobject");
    
    //add content to interest...
    len3 += mkHeader(stmt+len3, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len3 += addBlob(stmt+len3, (char*)ccnb_file, fsize);
    stmt[len3++] = 0; // end content
    
    len2 += mkHeader(contentobj+len2, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj

    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) stmt, len3);
    contentobj[len2++] = 0; // end-of-contentobj
    
    
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);
    
#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    out[len++] = 0; //name end
    out[len++] = 0; //interest end
    printf("Contentlen %ld\n", len1);

    free(ccnb_file);
    free(contentobj);
    free(stmt);
    return len;
}

int
mkRemoveFormRelayCacheRequest(unsigned char *out, char *ccn_path, char *private_key_path){
    
    int len = 0, len1 = 0, len2 = 0, len3 = 0;
    
    unsigned char out1[CCNL_MAX_PACKET_SIZE];
    unsigned char contentobj[10000];
    unsigned char stmt[2000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len1 = mkStrBlob(out1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    //signatur nach hier, über den rest
    len1 += mkStrBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "removecacheobject");

    // prepare debug statement
    len3 = mkHeader(stmt, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len3 += add_ccnl_name(stmt+len3, ccn_path);
    
    stmt[len3++] = 0; // end-of-debugstmt
    
    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) stmt, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    // add CONTENTOBJ as the final name component
    len1 += mkBlob(out1+len1, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

#ifdef USE_SIGNATURES
    if(private_key_path) len += add_signature(out+len, private_key_path, (char*)out1, len1);
#endif /*USE_SIGNATURES*/ 
    memcpy(out+len, out1, len1);
    len += len1;
    
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

//    ccnl_prefix_free(p);
    return len;
 
}

// ----------------------------------------------------------------------

int udp_open(int port, struct sockaddr_in *si)
{
    int s;
    unsigned int len;

    s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
	perror("udp socket");
	return -1;
    }

    si->sin_addr.s_addr = htonl(INADDR_ANY);
    si->sin_port = htons(port);
    si->sin_family = PF_INET;
    if(bind(s, (struct sockaddr *)si, sizeof(*si)) < 0) {
        perror("udp sock bind");
	return -1;
    }
    len = sizeof(*si);
    getsockname(s, (struct sockaddr*) si, &len);
    return s;
}

int
ccnl_crypto_ux_open(char *frompath)
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

int udp_sendto(int sock, char *address, int port, char *data, int len){
    int rc;
    struct sockaddr_in si_other;
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(port);
    if (inet_aton(address, &si_other.sin_addr)==0) {
          fprintf(stderr, "inet_aton() failed\n");
          exit(1);
    }
    rc = sendto(sock, data, len, 0, (const struct sockaddr *)&si_other, sizeof(si_other));
    return rc;
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
make_next_seg_debug_interest(int num, char *out)
{
    int len = 0;
    unsigned char cp[100];
    
    sprintf((char*)cp, "seqnum-%d", num);

    len = mkHeader((unsigned char*)out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader((unsigned char*)out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    
    len += mkStrBlob((unsigned char*)out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "mgmt");
    len += mkStrBlob((unsigned char*)out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, (char*)cp);
    
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest
    
    return len;
    
}

int
handle_ccn_signature(unsigned char **buf, int *buflen, char *relay_public_key)
{
   int num, typ, verified = 0, siglen;
   char *sigtype = 0, *sig = 0; 
   while (dehead(buf, buflen, &num, &typ) == 0) {
        
        if (num==0 && typ==0)
	    break; // end
        
        extractStr2(sigtype, CCN_DTAG_NAME);
        siglen = *buflen;
        extractStr2(sig, CCN_DTAG_SIGNATUREBITS);
        
        if (consume(typ, num, buf, buflen, 0, 0) < 0) goto Bail;
    }
    siglen = siglen-((*buflen)+4);
    unsigned char *buf2 = *buf;
    int buflen2 = *buflen - 1;
    if(relay_public_key)
    {
        verified = verify(relay_public_key, buf2, buflen2, sig, siglen);
    }
    Bail:
    return verified;
}

/**
 * Extract content, verify sig if public key is given as parameter
 * @param len
 * @param buf
 * @return 
 */
int
check_has_next(unsigned char *buf, int len, char **recvbuffer, int *recvbufferlen, char *relay_public_key, int *verified){

    int ret = 1;
    int contentlen = 0;
    int num, typ;
    char *newbuffer;
    
    if(dehead(&buf, &len, &num, &typ)) return 0; 
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) return 0;
    
    if(dehead(&buf, &len, &num, &typ)) return 0; 
    if(num == CCN_DTAG_SIGNATURE)
    {
        if (typ != CCN_TT_DTAG || num != CCN_DTAG_SIGNATURE) return 0;
        *verified = handle_ccn_signature(&buf,&len, relay_public_key);
        if(dehead(&buf, &len, &num, &typ)) return 0;
    }
   
    if (typ != CCN_TT_DTAG || num != CCNL_DTAG_FRAG) return 0;
    
    //check if there is a marker for the last segment
    if(dehead(&buf, &len, &num, &typ)) return 0;
    if(num == CCN_DTAG_ANY){
        char buf2[5];
        if(dehead(&buf, &len, &num, &typ)) return 0; 
        memcpy(buf2, buf, num);
        buf2[4] = 0;
        if(!strcmp(buf2, "last")){
            ret = 0;
        }
        buf+=num+1;
        len-=(num+1);
        if(dehead(&buf, &len, &num, &typ)) return 0; 
    }
    
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTDIGEST) return 0;
    if(dehead(&buf, &len, &num, &typ)) return 0; 
    if(typ != CCN_TT_BLOB) return 0; 
    contentlen = num;
        
    newbuffer = realloc(*recvbuffer, (*recvbufferlen+contentlen)*sizeof(char));
    *recvbuffer = newbuffer;
    memcpy(*recvbuffer+*recvbufferlen, buf, contentlen);
    *recvbufferlen += contentlen;    
    
    return ret;
}

#ifndef USE_JNI_LIB
int
main(int argc, char *argv[])
{
    char mysockname[200], *progname=argv[0];
    char *recvbuffer = 0, *recvbuffer2 = 0;
    int recvbufferlen = 0, recvbufferlen2 = 0;
    char *ux = CCNL_DEFAULT_UNIXSOCKNAME;
    char *udp = "0.0.0.0";
    int port = 0;
    int use_udp = 0;
    unsigned char out[CCNL_MAX_PACKET_SIZE];
    int len;
    int sock = 0;
    int verified_i = 0;
    int verified = 1;
    int numOfParts = 1;

    char *file_uri;
    char *ccn_path;
    char *private_key_path = 0, *relay_public_key = 0;
    struct sockaddr_in si;
    
    if (argv[1] && !strcmp(argv[1], "-x") && argc > 2) {
	ux = argv[2];
	argv += 2;
	argc -= 2;
    }
    else if (argv[1] && !strcmp(argv[1], "-u") && argc > 3) {
	udp = argv[2];
        port = strtol(argv[3], NULL, 0);
	argv += 3;
	argc -= 3;
        use_udp = 1;
    }
    if(argv[1] && !strcmp(argv[1], "-p") && argc > 2)
    {
        private_key_path = argv[2];
        argv += 2;
	argc -= 2;
    }
     if(argv[1] && !strcmp(argv[1], "-k") && argc > 2)
    {
        relay_public_key = argv[2];
        argv += 2;
	argc -= 2;
    }
    
    if (argc < 2) goto Usage;

    // socket for receiving
    sprintf(mysockname, "/tmp/.ccn-light-ctrl-%d.sock", getpid());
   
    if(!use_udp)
        sock = ccnl_crypto_ux_open(mysockname);
    else
        sock = udp_open(getpid()%65536+1025, &si);
    if (!sock) {
	fprintf(stderr, "cannot open UNIX/UDP receive socket\n");
	exit(-1);
    }

    if (!strcmp(argv[1], "debug")) {
	if (argc < 3)  goto Usage;
	len = mkDebugRequest(out, argv[2], private_key_path);
    } else if (!strcmp(argv[1], "newETHdev")) {
	if (argc < 3)  goto Usage;
	len = mkNewEthDevRequest(out, argv[2],
				 argc > 3 ? argv[3] : "0x88b5",
				 argc > 4 ? argv[4] : "0",
				 argc > 5 ? argv[5] : "0", private_key_path);
    } else if (!strcmp(argv[1], "newUDPdev")) {
	if (argc < 3)  goto Usage;
	len = mkNewUDPDevRequest(out, argv[2],
				 argc > 3 ? argv[3] : "9695",
				 argc > 4 ? argv[4] : "0",
				 argc > 5 ? argv[5] : "0", private_key_path);
    } else if (!strcmp(argv[1], "destroydev")) {
	if (argc < 3) goto Usage;
	len = mkDestroyDevRequest(out, argv[2], private_key_path);
    } else if (!strcmp(argv[1], "newETHface")||!strcmp(argv[1], "newUDPface")) {
	if (argc < 5)  goto Usage;
	len = mkNewFaceRequest(out,
			       !strcmp(argv[1], "newETHface") ? argv[2] : NULL,
			       !strcmp(argv[1], "newUDPface") ? argv[2] : NULL,
			       argv[3], argv[4],
			       argc > 5 ? argv[5] : "0x0001", private_key_path);
    } else if (!strcmp(argv[1], "newUNIXface")) {
	if (argc < 3)  goto Usage;
	len = mkNewUNIXFaceRequest(out, argv[2],
				   argc > 3 ? argv[3] : "0x0001", 
                                   private_key_path);
    } else if (!strcmp(argv[1], "setfrag")) {
	if (argc < 5)  goto Usage;
	len = mkSetfragRequest(out, argv[2], argv[3], argv[4], private_key_path);
    } else if (!strcmp(argv[1], "destroyface")) {
	if (argc < 3) goto Usage;
	len = mkDestroyFaceRequest(out, argv[2], private_key_path);
    } else if (!strcmp(argv[1], "prefixreg")) {
	if (argc < 4) goto Usage;
	len = mkPrefixregRequest(out, 1, argv[2], argv[3], private_key_path);
    } else if (!strcmp(argv[1], "prefixunreg")) {
	if (argc < 4) goto Usage;
	len = mkPrefixregRequest(out, 0, argv[2], argv[3], private_key_path);
    } else if (!strcmp(argv[1], "addContentToCache")){
        if(argc < 3) goto Usage;
        file_uri = argv[2];  
        len = mkAddToRelayCacheRequest(out, file_uri, private_key_path);
    } else if(!strcmp(argv[1], "removeContentFromCache")){
        if(argc < 3) goto Usage;
        ccn_path = argv[2];  
        len = mkRemoveFormRelayCacheRequest(out, ccn_path, private_key_path);
    } else{
	printf("unknown command %s\n", argv[1]);
	goto Usage;
    }

    if (len > 0) {
        
        if(!use_udp)
            ux_sendto(sock, ux, out, len);
        else
            udp_sendto(sock, udp, port, (char*)out, len);
        
//	sleep(1);
       
        unsigned int slen = 0; int num = 1; int len2 = 0;
        int hasNext = 0;

        memset(out, 0, sizeof(out));
        if(!use_udp)
            len = recv(sock, out, sizeof(out), 0);
        else
            len = recvfrom(sock, out, sizeof(out), 0, (struct sockaddr *)&si, &slen);
        hasNext = check_has_next(out, len, &recvbuffer, &recvbufferlen, relay_public_key, &verified_i);
        if(!verified_i) verified = 0;

        while(hasNext){
           //send an interest for debug packets... and store content in a array...
           char interest2[100];
           len2 = make_next_seg_debug_interest(num++, interest2);
           if(!use_udp)
                ux_sendto(sock, ux, (unsigned char*)interest2, len2);
           else
                udp_sendto(sock, udp, port, interest2, len2);
           memset(out, 0, sizeof(out));
           if(!use_udp)
                len = recv(sock, out, sizeof(out), 0);
           else
                len = recvfrom(sock, out, sizeof(out), 0, (struct sockaddr *)&si, &slen);
           hasNext =  check_has_next(out+2, len-2, &recvbuffer, &recvbufferlen, relay_public_key, &verified_i);
           if(!verified_i) verified = 0;
           ++numOfParts;

        }
        recvbuffer2 = malloc(sizeof(char)*recvbufferlen +1000);
        recvbufferlen2 += mkHeader((unsigned char*)recvbuffer2+recvbufferlen2, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);
        if(relay_public_key && use_udp)
        {
            char sigoutput[200];
            
            if(verified){
                sprintf(sigoutput, "All parts (%d) have been verified", numOfParts);
                recvbufferlen2 += mkStrBlob((unsigned char*)recvbuffer2+recvbufferlen2, CCN_DTAG_SIGNATURE, CCN_TT_DTAG, sigoutput);
            }else{
                sprintf(sigoutput, "NOT all parts (%d) have been verified", numOfParts);
                recvbufferlen2 += mkStrBlob((unsigned char*)recvbuffer2+recvbufferlen2, CCN_DTAG_SIGNATURE, CCN_TT_DTAG, sigoutput);
            }
        }
        memcpy(recvbuffer2+recvbufferlen2, recvbuffer, recvbufferlen);
        recvbufferlen2+=recvbufferlen;
        recvbuffer2[recvbufferlen2++] = 0; //end of content
                
	write(1, recvbuffer2, recvbufferlen2);
        printf("\n");
        
	printf("received %d bytes.\n", recvbufferlen);
    } else
	printf("nothing to send, program terminates\n");

    if(recvbuffer2)free(recvbuffer2);
    if(recvbuffer2)free(recvbuffer);
    close(sock);
    unlink(mysockname);

    return 0;

Usage:
    fprintf(stderr, "usage: %s [-x ux_path | -u ip-address port] [-p private-key] [-k relay-public-key] CMD, where CMD either of\n"
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
           "  addContentToCache             ccn-file\n"
           "  removeContentFromCache        ccn-path\n"
	   "where FRAG in none, seqd2012, ccnx2013\n",
	progname);

    if (sock) {
	close(sock);
	unlink(mysockname);
    }
    return -1;
}
#endif

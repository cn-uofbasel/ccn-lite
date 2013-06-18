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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../ccnx.h"
#include "../ccnl.h"

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


// ----------------------------------------------------------------------

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
		   char *encaps, char *flags)
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
    if (encaps)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_ENCAPS, CCN_TT_DTAG, encaps);
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
		   char *encaps, char *flags)
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
    if (encaps)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_ENCAPS, CCN_TT_DTAG, encaps);
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
    if (encaps)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_ENCAPS, CCN_TT_DTAG, encaps);
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
    if (encaps)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_ENCAPS, CCN_TT_DTAG, encaps);
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
mkSetencapsRequest(unsigned char *out, char *faceid, char *encaps, char *mtu)
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
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "setencaps");

    // prepare FACEINSTANCE
    len3 = mkHeader(faceinst, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, "setencaps");
    len3 += mkStrBlob(faceinst+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, faceid);
    len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_ENCAPS, CCN_TT_DTAG, encaps);
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

const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

int
dehead(unsigned char **buf, int *len, int *num, int *typ)
{
    int i;
    int val = 0;

    if (*len > 0 && **buf == 0) { // end
	*num = *typ = 0;
	*buf += 1;
	*len -= 1;
	return 0;
    }
    for (i = 0; (unsigned int) i < sizeof(i) && i < *len; i++) {
	unsigned char c = (*buf)[i];
	if ( c & 0x80 ) {
	    *num = (val << 4) | ((c >> 3) & 0xf);
	    *typ = c & 0x7;
	    *buf += i+1;
	    *len -= i+1;
	    return 0;
	}
	val = (val << 7) | c;
    } 
    return -1;
}

void
print_offset(int offset){
    int it;
    for(it = 0; it < offset; ++it)
        printf(" ");
}

int
extract_ccn_reply(unsigned char *buf, int len, int offset, int num_of_components);

void handleCompnent(unsigned char *buf, int len, int offset, char *tag, int isStrBlob, int num_of_components)
{
    int num, typ;
    print_offset(offset); printf("<%s>", tag);
    if(isStrBlob){
        dehead(&buf, &len, &num, &typ);
        while(*buf != 0)
        {
            //len++; buf++; 
            //if((*buf >="a" && *buf <= "z") || (*buf >="A" && *buf <= "Z") || (*buf >="0" && *buf <= "9"))
            printf("%c", *buf);
            len++; buf++;
        }
        len++; buf++;
        printf("</%s>\n", tag);
        extract_ccn_reply(buf, len, offset, num_of_components);
    }
    else{
        printf("\n");
        extract_ccn_reply(buf, len, offset+4, num_of_components);
        print_offset(offset); printf("</%s>\n", tag);
    }
}

int  
extract_ccn_reply(unsigned char *buf, int len, int offset, int num_of_components){
    
    int num, typ;
    if(len <= 0) return 1;
    if (dehead(&buf, &len, &num, &typ)) return -1;
    
    
    switch(num)
    {
        case CCN_DTAG_ANY:
            //extract_ccn_reply(buf, len, offset, num_of_components);
            //break;
        case CCN_DTAG_NAME:
            handleCompnent(buf, len, offset, "NAME", 0, num_of_components);
            break;
        case CCN_DTAG_COMPONENT:
            ++num_of_components;
            handleCompnent(buf, len, offset, "COMPONENT", num_of_components < 4, num_of_components);
            break;
        case CCN_DTAG_CONTENT:
            handleCompnent(buf, len, offset, "CONTENT", 0, num_of_components);
            break;
        case CCN_DTAG_SIGNEDINFO:
            handleCompnent(buf, len, offset, "SIGNEDINFO", 1, num_of_components);
            break;
        case CCN_DTAG_INTEREST:
            handleCompnent(buf, len, offset, "INTEREST", 0, num_of_components);
            break;
        case CCN_DTAG_KEY:
            handleCompnent(buf, len, offset, "KEY", 1, num_of_components);
            break;
        case CCN_DTAG_KEYLOCATOR:
            handleCompnent(buf, len, offset, "KEYLOCATOR", 1, num_of_components);
            break; 
        case CCN_DTAG_KEYNAME:
            handleCompnent(buf, len, offset, "KEYNAME", 1, num_of_components);
            break;
        case CCN_DTAG_SIGNATURE:
            handleCompnent(buf, len, offset, "SIGNATURE", 1, num_of_components);
            break;
        case CCN_DTAG_TIMESTAMP:
            handleCompnent(buf, len, offset, "TIMESTAMP", 1, num_of_components);
            break;
        case CCN_DTAG_TYPE:
            handleCompnent(buf, len, offset, "TYPE", 1, num_of_components);
            break;
        case CCN_DTAG_NONCE:
            handleCompnent(buf, len, offset, "NONCE", 1, num_of_components);
            break;
        case CCN_DTAG_SCOPE:
            handleCompnent(buf, len, offset, "SCOPE", 1, num_of_components);
            break;
        case CCN_DTAG_EXCLUDE:
            extract_ccn_reply(buf, len, offset, num_of_components);
            break;
        case CCN_DTAG_WITNESS:
            extract_ccn_reply(buf, len, offset, num_of_components);
            break;
        case CCN_DTAG_SIGNATUREBITS:
            handleCompnent(buf, len, offset, "SIGNATUREBITS", 1, num_of_components);
            break;
        case CCN_DTAG_FRESHNESS:
            extract_ccn_reply(buf, len, offset, num_of_components);
            break;
        case CCN_DTAG_FINALBLOCKID:
            handleCompnent(buf, len, offset, "FINALBLOCKID", 1, num_of_components);
            break;
        case CCN_DTAG_PUBPUBKDIGEST:
            extract_ccn_reply(buf, len, offset, num_of_components);
            break;
        case CCN_DTAG_PUBCERTDIGEST:
            handleCompnent(buf, len, offset, "PUBCERTDIGEST", 1, num_of_components);
            break;
        case CCN_DTAG_CONTENTOBJ:
            handleCompnent(buf, len, offset, "CONTENTOBJ", 0, num_of_components);
            break;
        case CCN_DTAG_ACTION:
            handleCompnent(buf, len, offset, "ACTION", 1, num_of_components);
            break;
        case CCN_DTAG_FACEID:
            handleCompnent(buf, len, offset, "FACEID", 1, num_of_components);
            break;
        case CCN_DTAG_IPPROTO:
            handleCompnent(buf, len, offset, "IPPROTO", 1, num_of_components);
            break;
        case CCN_DTAG_HOST:
            handleCompnent(buf, len, offset, "HOST", 1, num_of_components);
            break;
        case CCN_DTAG_PORT:
            handleCompnent(buf, len, offset, "PORT", 1, num_of_components);
            break;
        case CCN_DTAG_FWDINGFLAGS:
            handleCompnent(buf, len, offset, "FWDINGFLAGS", 1, num_of_components);
            break;
        case CCN_DTAG_FACEINSTANCE:
            dehead(&buf, &len, &num, &typ);
            handleCompnent(buf, len, offset, "FACEINSTANCE", 1, num_of_components);
            break;
        case CCN_DTAG_FWDINGENTRY:
            extract_ccn_reply(buf, len, offset, num_of_components);
            break;
        case CCN_DTAG_MINSUFFCOMP:
            handleCompnent(buf, len, offset, "MINSUFFCOMP", 1, num_of_components);
            break;
        case CCN_DTAG_MAXSUFFCOMP:
            handleCompnent(buf, len, offset, "MAXSUFFCOMP", 1, num_of_components);
            break;
        case CCN_DTAG_SEQNO:
            handleCompnent(buf, len, offset, "SEQNO", 1, num_of_components);
            break;
        case CCN_DTAG_CCNPDU:
            handleCompnent(buf, len, offset, "CCNPDU", 1, num_of_components);
            break;
        case CCNL_DTAG_MACSRC:
            handleCompnent(buf, len, offset, "MACSRC", 1, num_of_components);
            break;
        case CCNL_DTAG_IP4SRC:
            handleCompnent(buf, len, offset, "IP4SRC", 1, num_of_components);
            break;
        case CCNL_DTAG_UNIXSRC:
            handleCompnent(buf, len, offset, "UNIXSRC", 1, num_of_components);
            break;
        case CCNL_DTAG_ENCAPS:
            handleCompnent(buf, len, offset, "ENCAPS", 1, num_of_components);
            break;
        case CCNL_DTAG_FACEFLAGS:
            handleCompnent(buf, len, offset, "FACEFLAGS", 1, num_of_components);
            break;
        case CCNL_DTAG_DEVINSTANCE:
            extract_ccn_reply(buf, len, offset, num_of_components);
            break;
        case CCNL_DTAG_DEVNAME:
            handleCompnent(buf, len, offset, "DEVNAME", 1, num_of_components);
            break;
        case CCNL_DTAG_DEVFLAGS:
            handleCompnent(buf, len, offset, "DEVFLAGS", 1, num_of_components);
            break;
        case CCNL_DTAG_MTU:
            handleCompnent(buf, len, offset, "MTU", 1, num_of_components);
            break;
        case CCNL_DTAG_DEBUGREQUEST:
            dehead(&buf, &len, &num, &typ);
            handleCompnent(buf, len, offset, "DEBUGREQUEST", 1, num_of_components);
            break;
        case CCNL_DTAG_DEBUGACTION:
            handleCompnent(buf, len, offset, "DEBUGACTION", 1, num_of_components);
            break;
        case CCNL_DTAG_DEBUGREPLY:
            handleCompnent(buf, len, offset, "DEBUGREPLY", 1, num_of_components);
            break;
        case 0:
            extract_ccn_reply(buf, len, offset-4, num_of_components);
            break;
        default:
            //print_offset(offset); printf("Unknown num: %i %i\n", num, typ);
            extract_ccn_reply(buf, len, offset, num_of_components);
            break;
    }
}

int
main(int argc, char *argv[])
{
    char mysockname[200], *progname=argv[0];
    char *ux = CCNL_DEFAULT_UNIXSOCKNAME;
    unsigned char out[2000];
    int len;
    int sock = 0;

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
    } else if (!strcmp(argv[1], "setencaps")) {
	if (argc < 5)  goto Usage;
	len = mkSetencapsRequest(out, argv[2], argv[3], argv[4]);
    } else if (!strcmp(argv[1], "destroyface")) {
	if (argc < 3) goto Usage;
	len = mkDestroyFaceRequest(out, argv[2]);
    } else if (!strcmp(argv[1], "prefixreg")) {
	if (argc < 4) goto Usage;
	len = mkPrefixregRequest(out, 1, argv[2], argv[3]);
    } else if (!strcmp(argv[1], "prefixunreg")) {
	if (argc < 4) goto Usage;
	len = mkPrefixregRequest(out, 0, argv[2], argv[3]);
    } else {
	printf("unknown command %s\n", argv[1]);
	goto Usage;
    }

    if (len > 0) {
	ux_sendto(sock, ux, out, len);

//	sleep(1);
	len = recv(sock, out, sizeof(out), 0);
	if (len > 0)
	    out[len] = '\0';
            
        printf("CCN-PACKET:\n");
        extract_ccn_reply(out, len, 2, 0);
        printf("\n");
        
	printf("received %d bytes.", len);
    } else
	printf("nothing to send, program terminates\n");

    close(sock);
    unlink(mysockname);

    return 0;

Usage:
    fprintf(stderr, "usage: %s [-x ux_path] CMD, where CMD either of\n"
	   "  newETHdev     DEVNAME [ETHTYPE [ENCAPS [DEVFLAGS]]]\n"
	   "  newUDPdev     IP4SRC|any [PORT [ENCAPS [DEVFLAGS]]]\n"
	   "  destroydev    DEVNDX\n"
	   "  newETHface    MACSRC|any MACDST ETHTYPE [FACEFLAGS]\n"
	   "  newUDPface    IP4SRC|any IP4DST PORT [FACEFLAGS]\n"
	   "  newUNIXface   PATH [FACEFLAGS]\n"
           "  setencaps     FACEID ENCAPS MTU\n"
	   "  destroyface   FACEID\n"
	   "  prefixreg     PREFIX FACEID\n"
	   "  prefixunreg   PREFIX FACEID\n"
	   "  debug         dump\n"
	   "  debug         halt\n"
	   "  debug         dump+halt\n"
	   "where ENCAPS in none, seqd2012, ccnp2013\n",
	progname);

    if (sock) {
	close(sock);
	unlink(mysockname);
    }
    return -1;
}

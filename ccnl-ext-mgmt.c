/*
 * @f ccnl-ext-mgmt.c
 * @b CCN lite extension, management logic (face mgmt and registration)
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
 * 2012-05-06 created 
 * 2013-10-21 extended for crypto <christopher.scherb@unibas.ch>
 */


#ifndef CCNL_EXT_MGMT_C
#define CCNL_EXT_MGMT_C

#ifdef USE_MGMT

#ifdef CCNL_LINUXKERNEL
#include <linux/string.h>
#else
#include <stdlib.h>
#include <string.h>
#endif

#include "pkt-ccnb.h"
#include "pkt-ccnb-enc.c"
#include "ccnl.h"
#include "ccnl-core.h"
#include "ccnl-ext-debug.c"

#include "ccnl-ext-crypto.c"
#include "ccnl-ext.h"

unsigned char contentobj_buf[2000];
unsigned char faceinst_buf[2000];
unsigned char out_buf[2000];
unsigned char fwdentry_buf[2000];
unsigned char out1[2000], out2[1000], out3[500];

// ----------------------------------------------------------------------

int
ccnl_is_local_addr(sockunion *su)
{
    if (!su)
	return 0;
    if (su->sa.sa_family == AF_UNIX)
	return -1;
    if (su->sa.sa_family == AF_INET)
	return su->ip4.sin_addr.s_addr == htonl(0x7f000001);
    return 0;
}

int 
ccnl_mgmt_send_return_split(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, 
                int len, char *buf)
{
    
    int it;
    int size = CCNL_MAX_PACKET_SIZE/2;
    int numPackets = len/(size/2) + 1;
    
    for(it = 0; it < numPackets; ++it){
      //        int id = -it; 
        
        int packetsize = size/2;
        unsigned char *packet = (unsigned char*) ccnl_malloc(sizeof(char)*packetsize * 2);
        int len4 = 0;
        len4 += mkHeader(packet+len4, CCNL_DTAG_FRAG, CCN_TT_DTAG); 
        if(it == numPackets - 1) {
            len4 += mkStrBlob(packet+len4, CCN_DTAG_ANY, CCN_TT_DTAG, "last");
        }
        len4 += mkBlob(packet+len4, CCN_DTAG_CONTENTDIGEST, CCN_TT_DTAG, buf + it*packetsize, packetsize);
        packet[len4++] = 0;
        
//#ifdef USE_SIGNATURES
	//        if(it == 0) id = from->faceid;
    
#ifdef USE_SIGNATURES
        if(!ccnl_is_local_addr(&from->peer))
	  //                ccnl_crypto_sign(ccnl, packet, len4, "ccnl_mgmt_crypto", id);     
	    ccnl_crypto_sign(ccnl, packet, len4, "ccnl_mgmt_crypto",
			     it ? -it : from->faceid);     
        else
        {
#endif
            //send back the first part,
            //store the other parts in cache, after checking the pit
            unsigned char *buf2 = ccnl_malloc(CCNL_MAX_PACKET_SIZE*sizeof(char));
            int len5 = 0;
            len5 += mkHeader(buf2+len5, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // content
            memcpy(buf2+len5, packet, len4);
            len5 +=len4;
            buf2[len5++] = 0; // end-of-interest
            
            
            if(it == 0){
                struct ccnl_buf_s *retbuf;
                retbuf = ccnl_buf_new((char *)buf2, len5);
                ccnl_face_enqueue(ccnl, from, retbuf); 
            }
            else
            {
                //put to cache
                struct ccnl_prefix_s *prefix_a = 0;
                struct ccnl_content_s *c = 0;
                struct ccnl_buf_s *nonce=0, *ppkd=0, *pkt = 0;
                unsigned char *content = 0;
                unsigned char *ht = (unsigned char *) ccnl_malloc(sizeof(char)*20);
                int contlen;
                pkt = ccnl_ccnb_extract(&buf2, &len5, 0, 0, 0, 0,
				&prefix_a, &nonce, &ppkd, &content, &contlen);

                if (!pkt) {
                     //DEBUGMSG(6, " parsing error\n"); goto Done;
                }
                prefix_a->compcnt = 2;
                prefix_a->comp = (unsigned char **) ccnl_malloc(sizeof(unsigned char*)*2);
                prefix_a->comp[0] = (unsigned char *)"mgmt";
                sprintf((char*)ht, "seqnum-%d", it);
                prefix_a->comp[1] = ht;
                prefix_a->complen = (int *) ccnl_malloc(sizeof(int)*2);
                prefix_a->complen[0] = strlen("mgmt");
                prefix_a->complen[1] = strlen((char*)ht);
                c = ccnl_content_new(ccnl, CCNL_SUITE_CCNB, &pkt, &prefix_a,
				     &ppkd, content, contlen);
                //if (!c) goto Done;

                ccnl_content_serve_pending(ccnl, c);
                ccnl_content_add2cache(ccnl, c);
                //Done:
                //continue;
            }
            ccnl_free(buf2);
#ifdef USE_SIGNATURES
        }
#endif
        ccnl_free(packet);
    }
    return 1;
}

struct ccnl_prefix_s*
ccnl_prefix_clone(struct ccnl_prefix_s *p)
{
    int i, len;
    struct ccnl_prefix_s *p2;

    p2 = (struct ccnl_prefix_s*) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p2) return NULL;
    for (i = 0, len = 0; i < p->compcnt; len += p->complen[i++]);
    p2->path = (unsigned char*) ccnl_malloc(len);
    p2->comp = (unsigned char**) ccnl_malloc(p->compcnt*sizeof(char *));
    p2->complen = (int*) ccnl_malloc(p->compcnt*sizeof(int));
    if (!p2->comp || !p2->complen || !p2->path) goto Bail;
    p2->compcnt = p->compcnt;
    for (i = 0, len = 0; i < p->compcnt; len += p2->complen[i++]) {
	p2->complen[i] = p->complen[i];
	p2->comp[i] = p2->path + len;
	memcpy(p2->comp[i], p->comp[i], p2->complen[i]);
    }
    return p2;
Bail:
    free_prefix(p2);
    return NULL;
}

// ----------------------------------------------------------------------
// management protocols

#define extractStr(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
	char *s; unsigned char *valptr; int vallen; \
	if (consume(typ, num, &buf, &buflen, &valptr, &vallen) < 0) goto Bail; \
	s = ccnl_malloc(vallen+1); if (!s) goto Bail; \
	memcpy(s, valptr, vallen); s[vallen] = '\0'; \
	ccnl_free(VAR); \
	VAR = (unsigned char*) s; \
	continue; \
    } do {} while(0)


void ccnl_mgmt_return_ccn_msg(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, 
                    char *component_type, char* answer)
{
    int len = 0, len3 = 0;

    len = mkHeader(out1+len, CCN_DTAG_NAME, CCN_TT_DTAG); 
    len += mkStrBlob(out1+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out1+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out1+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, component_type);
    out1[len++] = 0;
    
    // prepare FWDENTRY
    len3 = mkStrBlob(out3, CCN_DTAG_ACTION, CCN_TT_DTAG, answer);

    len += mkBlob(out1+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) out3, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (char *)out1);
    return;
}


static int
ccnl_mgmt_create_interface_stmt(int num_interfaces, int *interfaceifndx, long *interfacedev,
        int *interfacedevtype, int *interfacereflect, char **interfaceaddr, unsigned char *stmt, int len3)
{
    int it; 
    char str[100];
    for(it = 0; it < num_interfaces; ++it) // interface content
        {
            len3 += mkHeader(stmt+len3, CCNL_DTAG_INTERFACE, CCN_TT_DTAG);
             
            memset(str, 0, 100);
            sprintf(str, "%d", interfaceifndx[it]);
            len3 += mkStrBlob(stmt+len3, CCNL_DTAG_IFNDX, CCN_TT_DTAG, str);
            
            memset(str, 0, 100);
            sprintf(str, "%s", interfaceaddr[it]);
            len3 += mkStrBlob(stmt+len3, CCNL_DTAG_ADDRESS, CCN_TT_DTAG, str);
            
            memset(str, 0, 100);
            if(interfacedevtype[it] == 1)
            {
                 sprintf(str, "%p", (void *) interfacedev[it]);
                 len3 += mkStrBlob(stmt+len3, CCNL_DTAG_ETH, CCN_TT_DTAG, str);
            }
            else if(interfacedevtype[it] == 2)
            {
                 sprintf(str, "%p", (void *) interfacedev[it]);
                 len3 += mkStrBlob(stmt+len3, CCNL_DTAG_SOCK, CCN_TT_DTAG, str);
            }
            else{
                 sprintf(str, "%p", (void *) interfacedev[it]);
                 len3 += mkStrBlob(stmt+len3, CCNL_DTAG_SOCK, CCN_TT_DTAG, str);
            }
            
            memset(str, 0, 100);
            sprintf(str, "%d", interfacereflect[it]);
            len3 += mkStrBlob(stmt+len3, CCNL_DTAG_REFLECT, CCN_TT_DTAG, str);
            
            stmt[len3++] = 0; //end of fwd;
        }
    return len3;
}

static int
ccnl_mgmt_create_faces_stmt(int num_faces, int *faceid, long *facenext,
		      long *faceprev, int *faceifndx, int *faceflags,
		      int *facetype, char **facepeer, char **facefrag,
		      unsigned char *stmt, int len3)
{
    int it;
    char str[100];
    for(it = 0; it < num_faces; ++it) //FACES CONTENT
    {
        len3 += mkHeader(stmt+len3, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);

        memset(str, 0, 100);
        sprintf(str, "%d", faceid[it]);
        len3 += mkStrBlob(stmt+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str,"%p", (void *)facenext[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_NEXT, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str,"%p", (void *)faceprev[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_PREV, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str,"%d", faceifndx[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_IFNDX, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str,"%02x", faceflags[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        if(facetype[it] == AF_INET)
            len3 += mkStrBlob(stmt+len3, CCNL_DTAG_IP, CCN_TT_DTAG, facepeer[it]);
#ifdef USE_ETHERNET
        else if(facetype[it] == AF_PACKET)
            len3 += mkStrBlob(stmt+len3, CCNL_DTAG_ETH, CCN_TT_DTAG, facepeer[it]);
#endif
        else if(facetype[it] == AF_UNIX)
            len3 += mkStrBlob(stmt+len3, CCNL_DTAG_UNIX, CCN_TT_DTAG, facepeer[it]);
        else{
            sprintf(str,"peer=?");
            len3 += mkStrBlob(stmt+len3, CCNL_DTAG_PEER, CCN_TT_DTAG, str);
        }
	// FIXME: dump frag information if present

        stmt[len3++] = 0; //end of faceinstance;    
    }
     return len3;
}

static int
ccnl_mgmt_create_fwds_stmt(int num_fwds, long *fwd, long *fwdnext, long *fwdface, int *fwdfaceid,
        int *fwdprefixlen, char **fwdprefix, unsigned char *stmt, int len3)
{
    int it;
    char str[100];
    for(it = 0; it < num_fwds; ++it) //FWDS content
    {
         len3 += mkHeader(stmt+len3, CCN_DTAG_FWDINGENTRY, CCN_TT_DTAG);

         memset(str, 0, 100);
         sprintf(str, "%p", (void *)fwd[it]);
         len3 += mkStrBlob(stmt+len3, CCNL_DTAG_FWD, CCN_TT_DTAG, str);

         memset(str, 0, 100);
         sprintf(str, "%p", (void *)fwdnext[it]);
         len3 += mkStrBlob(stmt+len3, CCNL_DTAG_NEXT, CCN_TT_DTAG, str);

         memset(str, 0, 100);
         sprintf(str, "%p", (void *)fwdface[it]);
         len3 += mkStrBlob(stmt+len3, CCNL_DTAG_FACE, CCN_TT_DTAG, str);

         memset(str, 0, 100);
         sprintf(str, "%d", fwdfaceid[it]);
         len3 += mkStrBlob(stmt+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, str);
         
         len3 += mkStrBlob(stmt+len3, CCNL_DTAG_PREFIX, CCN_TT_DTAG, fwdprefix[it]);

         stmt[len3++] = 0; //end of fwd;

    }
    return len3;
}

static int
ccnl_mgmt_create_interest_stmt(int num_interests, long *interest, long *interestnext, long *interestprev,
        int *interestlast, int *interestmin, int *interestmax, int *interestretries,
        long *interestpublisher, int* interestprefixlen, char **interestprefix,
        unsigned char *stmt, int len3)
{
    int it;
    char str[100];
    for(it = 0; it < num_interests; ++it) // interest content
    {
        len3 += mkHeader(stmt+len3, CCN_DTAG_INTEREST, CCN_TT_DTAG);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) interest[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_INTERESTPTR, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) interestnext[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_NEXT, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) interestprev[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_PREV, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", interestlast[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_LAST, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", interestmin[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_MIN, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", interestmax[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_MAX, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", interestretries[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_RETRIES, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) interestpublisher[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_PUBLISHER, CCN_TT_DTAG, str);
        
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_PREFIX, CCN_TT_DTAG, interestprefix[it]);

        stmt[len3++] = 0; //end of interest;
    }
    return len3;
}

static int
ccnl_mgmt_create_content_stmt(int num_contents, long *content, long *contentnext, 
        long *contentprev, int *contentlast_use, int *contentserved_cnt, 
        char **ccontents, char **cprefix, unsigned char *stmt, int len3)
{
    int it;
    char str[100];
    for(it = 0; it < num_contents; ++it)   // content content
    {
        len3 += mkHeader(stmt+len3, CCN_DTAG_CONTENT, CCN_TT_DTAG);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) content[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_CONTENTPTR, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) contentnext[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_NEXT, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) contentprev[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_PREV, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", contentlast_use[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_LASTUSE, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", contentserved_cnt[it]);
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_SERVEDCTN, CCN_TT_DTAG, str);
        
        len3 += mkStrBlob(stmt+len3, CCNL_DTAG_PREFIX, CCN_TT_DTAG, cprefix[it]);
        
        stmt[len3++] = 0; //end of content;
    }
    return len3;
}

int
ccnl_mgmt_debug(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    unsigned char *buf, *action, *debugaction;
    int it;
    int *faceid, *faceifndx, *faceflags, *facetype; //store face-info
    long *facenext, *faceprev;
    char **facepeer, **facefrag;
    
    int *fwdfaceid ,*fwdprefixlen; 
    long *fwd, *fwdnext, *fwdface;
    char **fwdprefix;
    
    int *interfaceifndx, *interfacedevtype, *interfacereflect;
    long *interfacedev;
    char **interfaceaddr;
    
    int *interestlast, *interestmin, *interestmax, *interestretries, *interestprefixlen;
    long *interest, *interestnext, *interestprev, *interestpublisher;
    char **interestprefix;
    
    int *contentlast_use, *contentserved_cnt, *cprefixlen;
    long *content, *contentnext, *contentprev;
    char **ccontents, **cprefix;
   
    int num_faces, num_fwds, num_interfaces, num_interests, num_contents;
    int buflen, num, typ;
    char *cp = "debug cmd failed";
    int rc = -1;
    
    //variables for answer
    int stmt_length, object_length, contentobject_length;
    unsigned char *out;
    unsigned char *contentobj;
    unsigned char *stmt;
    int len = 0, len3 = 0;

    //Alloc memory storage for face answer
    num_faces = get_num_faces(ccnl);
    facepeer = (char**)ccnl_malloc(num_faces*sizeof(char*));
    facefrag = (char**)ccnl_malloc(num_faces*sizeof(char*));
    for(it = 0; it <num_faces; ++it) {
        facepeer[it] = (char*)ccnl_malloc(50*sizeof(char));
        facefrag[it] = (char*)ccnl_malloc(50*sizeof(char));
    }
    faceid = (int*)ccnl_malloc(num_faces*sizeof(int));
    facenext = (long*)ccnl_malloc(num_faces*sizeof(long));
    faceprev = (long*)ccnl_malloc(num_faces*sizeof(long));
    faceifndx = (int*)ccnl_malloc(num_faces*sizeof(int));
    faceflags = (int*)ccnl_malloc(num_faces*sizeof(int));
    facetype = (int*)ccnl_malloc(num_faces*sizeof(int));
    
    //Alloc memory storage for fwd answer
    num_fwds = get_num_fwds(ccnl);
    fwd = (long*)ccnl_malloc(num_fwds*sizeof(long));
    fwdnext = (long*)ccnl_malloc(num_fwds*sizeof(long));
    fwdface = (long*)ccnl_malloc(num_fwds*sizeof(long));
    fwdfaceid = (int*)ccnl_malloc(num_fwds*sizeof(int));
    fwdprefixlen = (int*)ccnl_malloc(num_fwds*sizeof(int));
    fwdprefix = (char**)ccnl_malloc(num_faces*sizeof(char*));
    for(it = 0; it <num_fwds; ++it)
    {
        fwdprefix[it] = (char*)ccnl_malloc(256*sizeof(char));
        memset(fwdprefix[it], 0, 256);
    }
    
    //Alloc memory storage for interface answer
    num_interfaces = get_num_interface(ccnl);
    interfaceaddr = (char**)ccnl_malloc(num_interfaces*sizeof(char*));
    for(it = 0; it <num_interfaces; ++it) 
        interfaceaddr[it] = (char*)ccnl_malloc(130*sizeof(char));
    interfaceifndx = (int*)ccnl_malloc(num_interfaces*sizeof(int));
    interfacedev = (long*)ccnl_malloc(num_interfaces*sizeof(long));
    interfacedevtype = (int*)ccnl_malloc(num_interfaces*sizeof(int));
    interfacereflect = (int*)ccnl_malloc(num_interfaces*sizeof(int));
    
    //Alloc memory storage for interest answer
    num_interests = get_num_interests(ccnl);
    interest = (long*)ccnl_malloc(num_interests*sizeof(long));
    interestnext = (long*)ccnl_malloc(num_interests*sizeof(long));
    interestprev = (long*)ccnl_malloc(num_interests*sizeof(long));
    interestlast = (int*)ccnl_malloc(num_interests*sizeof(int));
    interestmin = (int*)ccnl_malloc(num_interests*sizeof(int));
    interestmax = (int*)ccnl_malloc(num_interests*sizeof(int));
    interestretries = (int*)ccnl_malloc(num_interests*sizeof(int));
    interestprefixlen = (int*)ccnl_malloc(num_interests*sizeof(int));
    interestpublisher = (long*)ccnl_malloc(num_interests*sizeof(long));
    interestprefix = (char**)ccnl_malloc(num_interests*sizeof(char*));
    for(it = 0; it < num_interests; ++it)
        interestprefix[it] = (char*)ccnl_malloc(256*sizeof(char));
    
    //Alloc memory storage for content answer
    num_contents = get_num_contents(ccnl);
    content = (long*)ccnl_malloc(num_contents*sizeof(long));
    contentnext = (long*)ccnl_malloc(num_contents*sizeof(long));
    contentprev = (long*)ccnl_malloc(num_contents*sizeof(long));
    contentlast_use = (int*)ccnl_malloc(num_contents*sizeof(int));
    contentserved_cnt = (int*)ccnl_malloc(num_contents*sizeof(int));
    cprefixlen = (int*)ccnl_malloc(num_contents*sizeof(int));
    ccontents = (char**)ccnl_malloc(num_contents*sizeof(char*));
    cprefix = (char**)ccnl_malloc(num_contents*sizeof(char*));
    for(it = 0; it < num_contents; ++it){
        ccontents[it] = (char*) ccnl_malloc(50*sizeof(char));
        cprefix[it] = (char*) ccnl_malloc(256*sizeof(char));
    }
    
    //End Alloc
    
    DEBUGMSG(99, "ccnl_mgmt_debug from=%s\n", ccnl_addr2ascii(&from->peer));
    action = debugaction = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCNL_DTAG_DEBUGREQUEST) goto Bail;

    while (dehead(&buf, &buflen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end
	extractStr(action, CCN_DTAG_ACTION);
	extractStr(debugaction, CCNL_DTAG_DEBUGACTION);

	if (consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="debug"

    if (debugaction) {
	cp = "debug cmd worked";
	DEBUGMSG(99, "  debugaction is %s\n",
	       debugaction);
	if (!strcmp((const char*)debugaction, "dump")){
	    ccnl_dump(0, CCNL_RELAY, ccnl);

            get_faces_dump(0,ccnl, faceid, facenext, faceprev, faceifndx, faceflags, facepeer, facetype, facefrag);
            get_fwd_dump(0,ccnl, fwd, fwdnext, fwdface, fwdfaceid, fwdprefixlen, fwdprefix);
            get_interface_dump(0, ccnl, interfaceifndx, interfaceaddr, interfacedev, interfacedevtype, interfacereflect);
            get_interest_dump(0,ccnl, interest, interestnext, 
                    interestprev, interestlast, interestmin,
                    interestmax, interestretries, interestpublisher, interestprefixlen, interestprefix);
            get_content_dump(0, ccnl, content, contentnext, contentprev, 
                    contentlast_use, contentserved_cnt, cprefixlen, cprefix);
            
        }
	else if (!strcmp((const char*)debugaction, "halt")){
	    ccnl->halt_flag = 1;
        }
	else if (!strcmp((const char*)debugaction, "dump+halt")) {
	    ccnl_dump(0, CCNL_RELAY, ccnl);
            
            get_faces_dump(0,ccnl, faceid, facenext, faceprev, faceifndx, faceflags, facepeer, facetype, facefrag);
            get_fwd_dump(0,ccnl, fwd, fwdnext, fwdface, fwdfaceid, fwdprefixlen, fwdprefix);
            get_interface_dump(0, ccnl, interfaceifndx, interfaceaddr, interfacedev, interfacedevtype, interfacereflect);
            get_interest_dump(0,ccnl, interest, interestnext, 
                    interestprev, interestlast, interestmin,
                    interestmax, interestretries, interestpublisher, interestprefixlen, interestprefix);
            get_content_dump(0, ccnl, content, contentnext, contentprev, 
                    contentlast_use, contentserved_cnt, cprefixlen, cprefix);
            
	    ccnl->halt_flag = 1;
	} else
	    cp = "unknown debug action, ignored";
    } else
	cp = "no debug action given, ignored";

    rc = 0;

Bail:
    /*ANSWER*/ 
    if(!debugaction) debugaction = (unsigned char *)"Error for debug cmd";
    stmt_length = 200 * num_faces + 200 * num_interfaces + 200 * num_fwds //alloc stroage for answer dynamically.
            + 200 * num_interests + 200 * num_contents;
    contentobject_length = stmt_length + 1000;
    object_length = contentobject_length + 1000;
    
    out = ccnl_malloc(object_length);
    contentobj = ccnl_malloc(contentobject_length);
    stmt = ccnl_malloc(stmt_length);

    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "debug");
    out[len++] = 0;

    // prepare debug statement
    len3 = mkHeader(stmt, CCNL_DTAG_DEBUGREQUEST, CCN_TT_DTAG);
    len3 += mkStrBlob(stmt+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, (char*) debugaction);
    len3 += mkStrBlob(stmt+len3, CCNL_DTAG_DEBUGACTION, CCN_TT_DTAG, cp);
    stmt[len3++] = 0; //end-of-debugstmt
    
    if(!strcmp((const char*)debugaction, "dump") || !strcmp((const char*)debugaction, "dump+halt")) //halt returns no content
    {
        len3 += mkHeader(stmt+len3, CCNL_DTAG_DEBUGREPLY, CCN_TT_DTAG);
        //len3 += mkStrBlob(stmt+len3, CCNL_DTAG_PREFIX, CCN_TT_DTAG, cinterfaces[it]);
        
        len3 = ccnl_mgmt_create_interface_stmt(num_interfaces, interfaceifndx, interfacedev, 
                interfacedevtype, interfacereflect, interfaceaddr, stmt, len3);
        
        len3 = ccnl_mgmt_create_faces_stmt(num_faces, faceid, facenext, faceprev, faceifndx,
			faceflags, facetype, facepeer, facefrag, stmt, len3);
        
        len3 = ccnl_mgmt_create_fwds_stmt(num_fwds, fwd, fwdnext, fwdface, fwdfaceid, 
                fwdprefixlen, fwdprefix, stmt, len3);
        
        len3 = ccnl_mgmt_create_interest_stmt(num_interests, interest, interestnext, interestprev,
                interestlast, interestmin, interestmax, interestretries,
                interestpublisher, interestprefixlen, interestprefix, stmt, len3);
        
        len3 = ccnl_mgmt_create_content_stmt(num_contents, content, contentnext, contentprev,
                contentlast_use, contentserved_cnt, ccontents, cprefix, stmt, len3);     
    }
    
    stmt[len3++] = 0; //end of debug reply
   
    len += mkBlob(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) stmt, len3);
    
    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (char*)out);
    
    /*END ANWER*/
    
    //FREE STORAGE
    ccnl_free(faceid);
    ccnl_free(facenext);
    ccnl_free(faceprev);
    ccnl_free(faceifndx);
    ccnl_free(fwdprefixlen);
    ccnl_free(faceflags);
    ccnl_free(facetype);
    ccnl_free(fwd);
    ccnl_free(fwdnext);
    ccnl_free(fwdface);
    ccnl_free(fwdfaceid);
    ccnl_free(interfaceifndx);
    ccnl_free(interfacedev);
    ccnl_free(interfacedevtype);
    ccnl_free(interfacereflect);
    ccnl_free(interest);
    ccnl_free(interestnext);
    ccnl_free(interestprev);
    ccnl_free(interestlast);
    ccnl_free(interestmin);
    ccnl_free(interestmax);
    ccnl_free(interestretries);
    ccnl_free(interestpublisher);
    ccnl_free(interestprefixlen);
    ccnl_free(content); 
    ccnl_free(contentnext); 
    ccnl_free(contentprev);
    ccnl_free(cprefixlen);
    ccnl_free(contentlast_use); 
    ccnl_free(contentserved_cnt);
    ccnl_free(out);
    ccnl_free(contentobj);
    ccnl_free(stmt);
    ccnl_free(action);
    ccnl_free(debugaction);
    
    for(it = 0; it < num_faces; ++it) {
        ccnl_free(facepeer[it]);
        ccnl_free(facefrag[it]);
    }
    ccnl_free(facepeer);
    ccnl_free(facefrag);
    for(it = 0; it < num_interfaces; ++it)
        ccnl_free(interfaceaddr[it]);
    ccnl_free(interfaceaddr);
    for(it = 0; it < num_interests; ++it)
        ccnl_free(interestprefix[it]);
    ccnl_free(interestprefix);
    for(it = 0; it < num_contents; ++it){
        ccnl_free(ccontents[it]);
        ccnl_free(cprefix[it]);
    }
    ccnl_free(ccontents);
    ccnl_free(cprefix);
    for(it = 0; it < num_fwds; ++it)
        ccnl_free(fwdprefix[it]);
    ccnl_free(fwdprefix);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

int
ccnl_mgmt_newface(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    unsigned char *buf;
    int buflen, num, typ;
    unsigned char *action, *macsrc, *ip4src, *proto, *host, *port,
	*path, *frag, *flags;
    char *cp = "newface cmd failed";
    int rc = -1;
    struct ccnl_face_s *f = NULL;
    //varibales for answer
    int len = 0, len3;
    //    unsigned char contentobj[2000];
    //    unsigned char faceinst[2000];
    unsigned char faceidstr[100];
    unsigned char retstr[200];

    DEBUGMSG(99, "ccnl_mgmt_newface from=%p, ifndx=%d\n",
	     (void*) from, from->ifndx);
    action = macsrc = ip4src = proto = host = port = NULL;
    path = frag = flags = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FACEINSTANCE) goto Bail;

    while (dehead(&buf, &buflen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end
	extractStr(action, CCN_DTAG_ACTION);
	extractStr(macsrc, CCNL_DTAG_MACSRC);
	extractStr(ip4src, CCNL_DTAG_IP4SRC);
	extractStr(path, CCNL_DTAG_UNIXSRC);
	extractStr(proto, CCN_DTAG_IPPROTO);
	extractStr(host, CCN_DTAG_HOST);
	extractStr(port, CCN_DTAG_PORT);
//	extractStr(frag, CCNL_DTAG_FRAG);
	extractStr(flags, CCNL_DTAG_FACEFLAGS);

	if (consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="newface"

#ifdef USE_ETHERNET
    if (macsrc && host && port) {
	sockunion su;
	DEBUGMSG(99, "  adding ETH face macsrc=%s, host=%s, ethtype=%s\n",
		 macsrc, host, port);
	memset(&su, 0, sizeof(su));
	su.eth.sll_family = AF_PACKET;
	su.eth.sll_protocol = htons(strtol((const char*)port, NULL, 0));
	if (sscanf((const char*) host, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		   su.eth.sll_addr,   su.eth.sll_addr+1,
		   su.eth.sll_addr+2, su.eth.sll_addr+3,
		   su.eth.sll_addr+4, su.eth.sll_addr+5) == 6) {
	// if (!strcmp(macsrc, "any")) // honouring macsrc not implemented yet
	    f = ccnl_get_face_or_create(ccnl, -1, &su.sa, sizeof(su.eth));
	}
    } else
#endif
    if (proto && host && port && !strcmp((const char*)proto, "17")) {
	sockunion su;
	DEBUGMSG(99, "  adding IP face ip4src=%s, proto=%s, host=%s, port=%s\n",
		 ip4src, proto, host, port);
	su.sa.sa_family = AF_INET;
	inet_aton((const char*)host, &su.ip4.sin_addr);
	su.ip4.sin_port = htons(strtol((const char*)port, NULL, 0));
	// not implmented yet: honor the requested ip4src parameter
	f = ccnl_get_face_or_create(ccnl, -1, // from->ifndx,
				    &su.sa, sizeof(struct sockaddr_in));
    }
#ifdef USE_UNIXSOCKET
    if (path) {
	sockunion su;
	DEBUGMSG(99, "  adding UNIX face unixsrc=%s\n", path);
	su.sa.sa_family = AF_UNIX;
	strcpy(su.ux.sun_path, (char*) path);
	f = ccnl_get_face_or_create(ccnl, -1, // from->ifndx,
				    &su.sa, sizeof(struct sockaddr_un));
    }
#endif

    if (f) {
	int flagval = flags ?
	    strtol((const char*)flags, NULL, 0) : CCNL_FACE_FLAGS_STATIC;
	//	printf("  flags=%s %d\n", flags, flagval);
	DEBUGMSG(99, "  adding a new face (id=%d) worked!\n", f->faceid);
	f->flags = flagval &
	    (CCNL_FACE_FLAGS_STATIC|CCNL_FACE_FLAGS_REFLECT);

#ifdef USE_FRAG
	if (frag) {
	    int mtu = 1500;
	    if (f->frag) {
		ccnl_frag_destroy(f->frag);
		f->frag = NULL;
	    }
	    if (f->ifndx >= 0 && ccnl->ifs[f->ifndx].mtu > 0)
		mtu = ccnl->ifs[f->ifndx].mtu;
	    f->frag = ccnl_frag_new(strtol((const char*)frag, NULL, 0),
					mtu); 
	}
#endif
	cp = "newface cmd worked";
    } else {
	DEBUGMSG(99, "  newface request for (macsrc=%s ip4src=%s proto=%s host=%s port=%s frag=%s flags=%s) failed or was ignored\n",
		 macsrc, ip4src, proto, host, port, frag, flags);
    }
    rc = 0;

Bail:
    /*ANSWER*/

    len = mkHeader(out_buf, CCN_DTAG_NAME, CCN_TT_DTAG);
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newface");
    out_buf[len++] = 0; // end-of-name

    // prepare FACEINSTANCE
    len3 = mkHeader(faceinst_buf, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    sprintf((char *)retstr,"newface:  %s", cp);
    len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, (char*) retstr);
    if (macsrc)
	len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_MACSRC, CCN_TT_DTAG, (char*) macsrc);
    if (ip4src) {
	len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_IP4SRC, CCN_TT_DTAG, (char*) ip4src);
        len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_IPPROTO, CCN_TT_DTAG, "17");
    }
    if (host)
	len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_HOST, CCN_TT_DTAG, (char*) host);
    if (port)
	len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_PORT, CCN_TT_DTAG, (char*) port);
    /*
    if (frag)
	len3 += mkStrBlob(faceinst+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, frag);
    */
    if (flags)
	len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG, (char *) flags);
    if (f) {
        sprintf((char *)faceidstr,"%i",f->faceid);
        len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char *) faceidstr);
    }
    
    faceinst_buf[len3++] = 0; // end-of-faceinst

    len += mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) faceinst_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (char*)out_buf);

    
    /*END ANWER*/  
            
            
    ccnl_free(action);
    ccnl_free(macsrc);
    ccnl_free(ip4src);
    ccnl_free(proto);
    ccnl_free(host);
    ccnl_free(port);
    ccnl_free(frag);
    ccnl_free(flags);
    ccnl_free(path);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

int
ccnl_mgmt_setfrag(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    unsigned char *buf;
    int buflen, num, typ;
    unsigned char *action, *faceid, *frag, *mtu;
    char *cp = "setfrag cmd failed";
    int rc = -1;
    struct ccnl_face_s *f;
    int len = 0, len3;

    DEBUGMSG(99, "ccnl_mgmt_setfrag from=%p, ifndx=%d\n",
	     (void*) from, from->ifndx);
    action = faceid = frag = mtu = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FACEINSTANCE) goto Bail;

    while (dehead(&buf, &buflen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end
	extractStr(action, CCN_DTAG_ACTION);
	extractStr(faceid, CCN_DTAG_FACEID);
	extractStr(frag, CCNL_DTAG_FRAG);
	extractStr(mtu, CCNL_DTAG_MTU);

	if (consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="newface"

    if (faceid && frag && mtu) {
	int e = -1, mtuval, fi = strtol((const char*)faceid, NULL, 0);

	for (f = ccnl->faces; f && f->faceid != fi; f = f->next);
	if (!f) goto Error;
	mtuval = strtol((const char*)mtu, NULL, 0);

#ifdef USE_FRAG
	if (f->frag) {
	    ccnl_frag_destroy(f->frag);
	    f->frag = 0;
	}
	if (!strcmp((const char*)frag, "none"))
	    e = CCNL_FRAG_NONE;
	else if (!strcmp((const char*)frag, "seqd2012")) {
	    e = CCNL_FRAG_SEQUENCED2012;
	} else if (!strcmp((const char*)frag, "ccnx2013")) {
	    e = CCNL_FRAG_CCNx2013;
	}
	if (e < 0)
	    goto Error;
	f->frag = ccnl_frag_new(e, mtuval);
	cp = "setfrag cmd worked";
#else
	cp = "no fragmentation support" + 0*e; // use e to silence compiler
#endif
    } else {
Error:
	DEBUGMSG(99, "  setfrag request for (faceid=%s frag=%s mtu=%s) failed or was ignored\n",
		 faceid, frag, mtu);
    }
    rc = 0;

Bail:
    ccnl_free(action);

    len += mkHeader(out_buf+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "setfrag");
    out_buf[len++] = 0; // end-of-name

    // prepare FACEINSTANCE
    len3 = mkHeader(faceinst_buf, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, cp);
    len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid);
    len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, (char*) frag);
    len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_MTU, CCN_TT_DTAG, (char*) mtu);
    faceinst_buf[len3++] = 0; // end-of-faceinst

    len += mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) faceinst_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (char*)out_buf);

    ccnl_free(faceid);
    ccnl_free(frag);
    ccnl_free(mtu);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

int
ccnl_mgmt_destroyface(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		      struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    unsigned char *buf;
    int buflen, num, typ;
    unsigned char *action, *faceid;
    char *cp = "destroyface cmd failed";
    int rc = -1;
   
    int len = 0, len3;
//    unsigned char contentobj[2000];
//    unsigned char faceinst[2000];

    DEBUGMSG(99, "ccnl_mgmt_destroyface\n");
    action = faceid = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FACEINSTANCE) goto Bail;

    while (dehead(&buf, &buflen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end
	extractStr(action, CCN_DTAG_ACTION);
	extractStr(faceid, CCN_DTAG_FACEID);

	if (consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="destroyface"

    if (faceid) {
	struct ccnl_face_s *f;
	int fi = strtol((const char*)faceid, NULL, 0);
	for (f = ccnl->faces; f && f->faceid != fi; f = f->next);
	if (!f) {
	    DEBUGMSG(99, "  could not find face=%s\n", faceid);
	    goto Bail;
	}
	ccnl_face_remove(ccnl, f);
	DEBUGMSG(99, "  face %s destroyed\n", faceid);
	cp = "facedestroy cmd worked";
    } else {
	DEBUGMSG(99, "  missing faceid\n");
    }
    rc = 0;

Bail:
    /*ANSWER*/
    if(!faceid) {
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "destroyface", cp);
        return -1;
    }
   
    len += mkHeader(out_buf+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "destroyface");
    out_buf[len++] = 0; // end-of-name

    // prepare FACEINSTANCE
    len3 = mkHeader(faceinst_buf, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, cp);
    len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid);
    faceinst_buf[len3++] = 0; // end-of-faceinst

    len += mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) faceinst_buf, len3);
    
    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (char*)out_buf);

    
    /*END ANWER*/  
    ccnl_free(action);
    ccnl_free(faceid);
    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

int
ccnl_mgmt_newdev(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		 struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    unsigned char *buf;
    int buflen, num, typ;
    unsigned char *action, *devname, *ip4src, *port, *frag, *flags;
    char *cp = "newdevice cmd worked";
    int rc = -1;
    
    //variables for answer
    int len = 0, len3, portnum;
//    unsigned char contentobj[2000];
//    unsigned char faceinst[2000];
    struct ccnl_if_s *i;
    

    DEBUGMSG(99, "ccnl_mgmt_newdev\n");
    action = devname = ip4src = port = frag = flags = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCNL_DTAG_DEVINSTANCE) goto Bail;

    while (dehead(&buf, &buflen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end
	extractStr(action, CCN_DTAG_ACTION);
	extractStr(devname, CCNL_DTAG_DEVNAME);
	extractStr(ip4src, CCNL_DTAG_IP4SRC);
	extractStr(port, CCN_DTAG_PORT);
	extractStr(frag, CCNL_DTAG_FRAG);
	extractStr(flags, CCNL_DTAG_DEVFLAGS);

	if (consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="newdev"

    if (ccnl->ifcount >= CCNL_MAX_INTERFACES) {
      DEBUGMSG(99, "  too many interfaces, no new interface created\n");
      goto Bail;
    }

#if defined(USE_ETHERNET) && (defined(CCNL_UNIX) || defined(CCNL_LINUXKERNEL))
    if (devname && port) {
        cp = "newETHdev cmd worked";
	portnum = port ? strtol((const char*)port, NULL, 0) : CCNL_ETH_TYPE;

	DEBUGMSG(99, "  adding eth device devname=%s, port=%s\n",
		 devname, port);

	// check if it already exists, bail

	// create a new ifs-entry
	i = &ccnl->ifs[ccnl->ifcount];
#ifdef CCNL_LINUXKERNEL
	{
	    struct net_device *nd;
	    int j;
	    nd = ccnl_open_ethdev((char*)devname, &i->addr.eth, portnum);
	    if (!nd) {
		DEBUGMSG(99, "  could not open device %s\n", devname);
		goto Bail;
	    }
	    for (j = 0; j < ccnl->ifcount; j++) {
		if (ccnl->ifs[j].netdev == nd) {
		    dev_put(nd);
		    DEBUGMSG(99, "  device %s already open\n", devname);
		    goto Bail;
		}
	    }
	    i->netdev = nd;
	    i->ccnl_packet.type = htons(portnum);
	    i->ccnl_packet.dev = i->netdev;
	    i->ccnl_packet.func = ccnl_eth_RX;
	    dev_add_pack(&i->ccnl_packet);
	}
#else
	i->sock = ccnl_open_ethdev((char*)devname, &i->addr.eth, portnum);
	if (!i->sock) {
	    DEBUGMSG(99, "  could not open device %s\n", devname);
	    goto Bail;
	}
#endif
//	i->frag = frag ? atoi(frag) : 0;
	i->mtu = 1500;
//	we should analyse and copy flags, here we hardcode some defaults:
	i->reflect = 1;
	i->fwdalli = 1;

	if (ccnl->defaultInterfaceScheduler)
	    i->sched = ccnl->defaultInterfaceScheduler(ccnl, ccnl_interface_CTS);
	ccnl->ifcount++;

	rc = 0;
	goto Bail;
    }
#endif

// #ifdef USE_UDP
    if (ip4src && port) {
        cp = "newUDPdev cmd worked";
	DEBUGMSG(99, "  adding UDP device ip4src=%s, port=%s\n",
		 ip4src, port);

	// check if it already exists, bail

	// create a new ifs-entry
	i = &ccnl->ifs[ccnl->ifcount];
	i->sock = ccnl_open_udpdev(strtol((char*)port, NULL, 0), &i->addr.ip4);
	if (!i->sock) {
	    DEBUGMSG(99, "  could not open UDP device %s/%s\n", ip4src, port);
	    goto Bail;
	}

#ifdef CCNL_LINUXKERNEL
	{
	    int j;
	    for (j = 0; j < ccnl->ifcount; j++) {
		if (!ccnl_addr_cmp(&ccnl->ifs[j].addr, &i->addr)) {
		    sock_release(i->sock);
		    DEBUGMSG(99, "  UDP device %s/%s already open\n",
			     ip4src, port);
		    goto Bail;
		}
	    }
	}

	i->wq = create_workqueue(ccnl_addr2ascii(&i->addr));
	if (!i->wq) {
	    DEBUGMSG(99, "  could not create work queue (UDP device %s/%s)\n", ip4src, port);
	    sock_release(i->sock);
	    goto Bail;
	}
	write_lock_bh(&i->sock->sk->sk_callback_lock);
	i->old_data_ready = i->sock->sk->sk_data_ready;
	i->sock->sk->sk_data_ready = ccnl_udp_data_ready;
//	i->sock->sk->sk_user_data = &theRelay;
	write_unlock_bh(&i->sock->sk->sk_callback_lock);
#endif

//	i->frag = frag ? atoi(frag) : 0;
	i->mtu = CCN_DEFAULT_MTU;
//	we should analyse and copy flags, here we hardcode some defaults:
	i->reflect = 0;
	i->fwdalli = 1;

	if (ccnl->defaultInterfaceScheduler)
	    i->sched = ccnl->defaultInterfaceScheduler(ccnl, ccnl_interface_CTS);
	ccnl->ifcount++;

	//cp = "newdevice cmd worked";
	rc = 0;
	goto Bail;
    }

    DEBUGMSG(99, "  newdevice request for (namedev=%s ip4src=%s port=%s frag=%s) failed or was ignored\n",
	     devname, ip4src, port, frag);
// #endif // USE_UDP

Bail:

    len += mkHeader(out_buf+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newdev");
    out_buf[len++] = 0; // end-of-name
    
    // prepare DEVINSTANCE
    len3 = mkHeader(faceinst_buf, CCNL_DTAG_DEVINSTANCE, CCN_TT_DTAG);
    len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, cp);
    if (devname)
    len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_DEVNAME, CCN_TT_DTAG,
                      (char *) devname);
    
    if (devname && port) {
        if (port)
            len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_PORT, CCN_TT_DTAG, (char*) port);
        if (frag)
            len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, (char*) frag);
        if (flags)
            len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG, (char*) flags);
        faceinst_buf[len3++] = 0; // end-of-faceinst 
    }
    else if (ip4src && port) {
        if (ip4src)
            len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_IP4SRC, CCN_TT_DTAG, (char*) ip4src);
        if (port)
            len3 += mkStrBlob(faceinst_buf+len3, CCN_DTAG_PORT, CCN_TT_DTAG, (char*) port);
        if (frag)
            len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, (char*) frag);
        if (flags)
            len3 += mkStrBlob(faceinst_buf+len3, CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG, (char*) flags);
        faceinst_buf[len3++] = 0; // end-of-faceinst
    }

    len += mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (char*)out_buf);
    
    ccnl_free(devname);
    ccnl_free(port);
    ccnl_free(frag);
    ccnl_free(action);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}


int
ccnl_mgmt_destroydev(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		     struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{

    DEBUGMSG(99, "mgmt_destroydev not implemented yet\n");
    /*ANSWER*/
    ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "mgmt_destroy", "mgmt_destroydev not implemented yet");
    
    /*END ANSWER*/
    return -1;
}


int
ccnl_mgmt_prefixreg(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    unsigned char *buf;
    int buflen, num, typ;
    struct ccnl_prefix_s *p = NULL;
    unsigned char *action, *faceid;
    char *cp = "prefixreg cmd failed";
    int rc = -1;

    int len = 0, len3;
//    unsigned char contentobj[2000];
//    unsigned char fwdentry[2000];

    DEBUGMSG(99, "ccnl_mgmt_prefixreg\n");
    action = faceid = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
  
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FWDINGENTRY) goto Bail;

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p) goto Bail;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
					   sizeof(unsigned char*));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    while (dehead(&buf, &buflen, &num, &typ) == 0) {
	if (num==0 && typ==0)
	    break; // end

	if (typ == CCN_TT_DTAG && num == CCN_DTAG_NAME) {
	    for (;;) {
		if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
		if (num==0 && typ==0)
		    break;
		if (typ == CCN_TT_DTAG && num == CCN_DTAG_COMPONENT &&
		    p->compcnt < CCNL_MAX_NAME_COMP) {
			// if (ccnl_grow_prefix(p)) goto Bail;
		    if (consume(typ, num, &buf, &buflen,
				p->comp + p->compcnt,
				p->complen + p->compcnt) < 0) goto Bail;
		    p->compcnt++;
		} else {
		    if (consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
		}
	    }
	    continue;
	}

	extractStr(action, CCN_DTAG_ACTION);
	extractStr(faceid, CCN_DTAG_FACEID);

	if (consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="prefixreg"
    if (faceid && p->compcnt > 0) {
	struct ccnl_face_s *f;
	struct ccnl_forward_s *fwd, **fwd2;
	int fi = strtol((const char*)faceid, NULL, 0);
	DEBUGMSG(99, "mgmt: adding prefix %s to faceid=%s\n",
		 ccnl_prefix_to_path(p), faceid);

	for (f = ccnl->faces; f && f->faceid != fi; f = f->next);
	if (!f) goto Bail;

//	printf("Face %s found\n", faceid);
	fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
	if (!fwd) goto Bail;
	fwd->prefix = ccnl_prefix_clone(p);
	fwd->face = f;
	fwd2 = &ccnl->fib;
	while (*fwd2)
	    fwd2 = &((*fwd2)->next);
	*fwd2 = fwd;
	cp = "prefixreg cmd worked";
    } else {
	DEBUGMSG(99, "mgmt: ignored prefixreg faceid=%s\n", faceid);
    }

    rc = 0;

Bail:
    /*ANSWER*/
    if(!action || !p || ! faceid) {
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "prefixreg", cp);
        return -1;
    }
    len += mkHeader(out_buf+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, (char*) action);
    out_buf[len++] = 0; // end-of-name

    // prepare FWDENTRY
    len3 = mkHeader(fwdentry_buf, CCNL_DTAG_PREFIX, CCN_TT_DTAG);
    len3 += mkStrBlob(fwdentry_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, cp);
    len3 += mkStrBlob(fwdentry_buf+len3, CCN_DTAG_NAME, CCN_TT_DTAG, ccnl_prefix_to_path(p)); // prefix

    len3 += mkStrBlob(fwdentry_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid);
    fwdentry_buf[len3++] = 0; // end-of-fwdentry

    len += mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) fwdentry_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (char*)out_buf);
    
    /*END ANWER*/  


    ccnl_free(faceid);
    ccnl_free(action);
    free_prefix(p);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

int
ccnl_mgmt_addcacheobject(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{    
    unsigned char *buf;
    unsigned char *data;
    int buflen, datalen, contlen;
    int num, typ;
    
    struct ccnl_prefix_s *prefix_a = 0;
    struct ccnl_content_s *c = 0;
    struct ccnl_buf_s *nonce=0, *ppkd=0, *pkt = 0;
    unsigned char *content;
    
    //unsigned char *sigtype = 0, *sig = 0;
    char *answer = "Failed to add content";
    
    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    
    datalen = buflen - 2;
    data = buf;  

    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    
    //add object to cache here...
    data = buf + 2;

    pkt = ccnl_ccnb_extract(&data, &datalen, 0, 0, 0, 0,
			    &prefix_a, &nonce, &ppkd, &content, &contlen);
    if (!pkt) {
        DEBUGMSG(6, " parsing error\n"); goto Done;
    }
    if (!prefix_a) {
        DEBUGMSG(6, " no prefix error\n"); goto Done;
    }
    c = ccnl_content_new(ccnl, CCNL_SUITE_CCNB, &pkt, &prefix_a, &ppkd,
                        content, contlen);
    if (!c) goto Done;
    ccnl_content_add2cache(ccnl, c);
    c->flags |= CCNL_CONTENT_FLAGS_STATIC;
     
    answer = "Content successfully added";
    Done:
        free_prefix(prefix_a);
        ccnl_free(pkt);
        ccnl_free(nonce);
        ccnl_free(ppkd);

    Bail:
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "addcacheobject", answer);
    return 0;   
}

int
ccnl_mgmt_removecacheobject(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    
    unsigned char *buf;
    unsigned char **components = 0;
    unsigned int num_of_components = -1;
    int buflen, i;
    int num, typ;
    char *answer = "Failed to remove content";
    struct ccnl_content_s *c2;
    
    components = (unsigned char**) ccnl_malloc(sizeof(unsigned char*)*1024);
    for(i = 0; i < 1024; ++i)components[i] = 0;
    
    buf = prefix->comp[3];
    buflen = prefix->complen[3];
 
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
     
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
     
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME) goto Bail;
    
    while (dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
	    break; // end
        ++num_of_components;
        extractStr(components[num_of_components], CCN_DTAG_COMPONENT);
       
        if (consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }
    ++num_of_components;
    
    for (c2 = ccnl->contents; c2; c2 = c2->next)
    {
        if(c2->name->compcnt != num_of_components) continue;
        for(i = 0; i < num_of_components; ++i)
        {
            if(strcmp((char*)c2->name->comp[i], (char*)components[i]))
            {
                break;
            }
        }
        if(i == num_of_components)
        {
            break;
        }
    }
    if(i == num_of_components){
        DEBUGMSG(99, "Content found\n");
        ccnl_content_remove(ccnl, c2);
    }else
    {
       DEBUGMSG(99, "Ignore request since content not found\n");
       goto Bail;
    }
    answer = "Content successfully removed";
    
    Bail:
    //send answer
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "removecacheobject", answer); 
    ccnl_free(components);        
    return 0;
}

#ifdef USE_SIGNATURES
int
ccnl_mgmt_validate_signatue(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
		    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, char *cmd)
{
    
    unsigned char *buf;
    unsigned char *data;
    int buflen, datalen, siglen = 0;
    int num, typ;
    unsigned char *sigtype = 0, *sig = 0;
    
    buf = orig->data;
    buflen = orig->datalen;
    
    //SKIP HEADER FIELDS
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_INTEREST) goto Bail;
    
    if (dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME) goto Bail;
    
    if (dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_SIGNATURE) goto Bail;
    while (dehead(&buf, &buflen, &num, &typ) == 0) {
        
        if (num==0 && typ==0)
	    break; // end
        
        extractStr(sigtype, CCN_DTAG_NAME);
        siglen = buflen;
        extractStr(sig, CCN_DTAG_SIGNATUREBITS);
        if (consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }
    siglen = siglen-(buflen+4);
    
    datalen = buflen - 2;
    data = buf;  
    
    ccnl_crypto_verify(ccnl, data, datalen, (char *)sig, siglen, "ccnl_mgmt_crypto", from->faceid);
    
    return 0;
    
    Bail:
    ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, cmd, 
                "refused: signature could not be validated"); 
    return -1;
}
#endif /*USE_SIGNATURES*/

int ccnl_mgmt_handle(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
	  struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, 
        char *cmd, int verified)
{
    DEBUGMSG(99, "ccnl_mgmt_handle \"%s\"\n", cmd);
    if(!verified){
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, cmd, 
                "refused: error signature not verified");
        return -1;
    }
    
    if (!strcmp(cmd, "newdev"))
	ccnl_mgmt_newdev(ccnl, orig, prefix, from);
    else if (!strcmp(cmd, "setfrag"))
	ccnl_mgmt_setfrag(ccnl, orig, prefix, from);
    else if (!strcmp(cmd, "destroydev"))
	ccnl_mgmt_destroydev(ccnl, orig, prefix, from);
    else if (!strcmp(cmd, "newface"))
	ccnl_mgmt_newface(ccnl, orig, prefix, from);
    else if (!strcmp(cmd, "destroyface"))
	ccnl_mgmt_destroyface(ccnl, orig, prefix, from);
    else if (!strcmp(cmd, "prefixreg"))
	ccnl_mgmt_prefixreg(ccnl, orig, prefix, from);
#ifdef USE_DEBUG
    else if (!strcmp(cmd, "addcacheobject"))
	ccnl_mgmt_addcacheobject(ccnl, orig, prefix, from);
    else if (!strcmp(cmd, "removecacheobject"))
	ccnl_mgmt_removecacheobject(ccnl, orig, prefix, from);
    else if (!strcmp(cmd, "debug")) {
      ccnl_mgmt_debug(ccnl, orig, prefix, from);
    }
#endif
    else {
	DEBUGMSG(99, "unknown mgmt command %s\n", cmd);
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, cmd, "unknown mgmt command");
	return -1;
    }
    return 0;
}

int
ccnl_mgmt(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
	  struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    char cmd[1000];
    if (prefix->complen[2] < sizeof(cmd)) {
        memcpy(cmd, prefix->comp[2], prefix->complen[2]);
        cmd[prefix->complen[2]] = '\0';
    } else
	strcpy(cmd, "cmd-is-too-long-to-display");

    DEBUGMSG(99, "ccnl_mgmt request \"%s\"\n", cmd);

    if (ccnl_is_local_addr(&from->peer)) goto MGMT;

#ifdef USE_SIGNATURES
    return ccnl_mgmt_validate_signatue(ccnl, orig, prefix, from, cmd);
#endif /*USE_SIGNATURES*/
                   
    DEBUGMSG(99, "  rejecting because src=%s is not a local addr\n",
            ccnl_addr2ascii(&from->peer));
    ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, cmd, 
                "refused: origin of mgmt cmd is not local"); 
    return -1;
	
    MGMT:
    ccnl_mgmt_handle(ccnl, orig, prefix, from, cmd, 1);

    return 0;
}

#endif // USE_MGMT
#endif /*CCNL_EXT_MGMT_C*/
// eof

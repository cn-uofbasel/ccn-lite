/*
 * @f ccnl-ext-mgmt.c
 * @b CCN lite extension, management logic (face mgmt and registration)
 *
 * Copyright (C) 2012-15, Christian Tschudin, University of Basel
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


#ifdef USE_MGMT

#include "ccnl-ext-crypto.c"

unsigned char contentobj_buf[2000];
unsigned char faceinst_buf[2000];
unsigned char out_buf[2000];
unsigned char fwdentry_buf[2000];
unsigned char out1[2000], out2[1000], out3[500];

// ----------------------------------------------------------------------

int
get_num_faces(void *p)
{
    int num = 0;
    struct ccnl_relay_s    *top = (struct ccnl_relay_s    *) p;
    struct ccnl_face_s     *fac = (struct ccnl_face_s     *) top->faces;

    while (fac) {
        ++num;
        fac = fac->next;
    }
    return num;
}

int
get_num_fwds(void *p)
{
    int num = 0;
    struct ccnl_relay_s    *top = (struct ccnl_relay_s    *) p;
    struct ccnl_forward_s  *fwd = (struct ccnl_forward_s  *) top->fib;

    while (fwd) {
        ++num;
        fwd = fwd->next;
    }
    return num;
}

int
get_num_interface(void *p)
{
    struct ccnl_relay_s    *top = (struct ccnl_relay_s    *) p;
    return top->ifcount;
}

int
get_num_interests(void *p)
{
    int num = 0;
    struct ccnl_relay_s *top = (struct ccnl_relay_s    *) p;
    struct ccnl_interest_s *itr = (struct ccnl_interest_s *) top->pit;

    while (itr) {
        ++num;
        itr = itr->next;
    }
    return num;
}

int
get_num_contents(void *p)
{
    int num = 0;
    struct ccnl_relay_s *top = (struct ccnl_relay_s    *) p;
    struct ccnl_content_s  *con = (struct ccnl_content_s  *) top->contents;

    while (con) {
        ++num;
        con = con->next;
    }
    return num;
}

// ----------------------------------------------------------------------

int
ccnl_mgmt_send_return_split(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                struct ccnl_prefix_s *prefix, struct ccnl_face_s *from,
                int len, unsigned char *buf)
{

    int it, size = CCNL_MAX_PACKET_SIZE/2;
    int numPackets = len/(size/2) + 1;

    DEBUGMSG(DEBUG, "ccnl_mgmt_send_return_split %d bytes, %d packet(s)\n",
             len, numPackets);

    for(it = 0; it < numPackets; ++it){
        unsigned char *buf2;
        int packetsize = size/2, len4 = 0, len5;
        unsigned char *packet = (unsigned char*) ccnl_malloc(sizeof(char)*packetsize * 2);

        len4 += ccnl_ccnb_mkHeader(packet+len4, CCNL_DTAG_FRAG, CCN_TT_DTAG);
        if(it == numPackets - 1) {
            len4 += ccnl_ccnb_mkStrBlob(packet+len4, CCN_DTAG_ANY, CCN_TT_DTAG, "last");
        }
        len5 = len - it * packetsize;
        if (len5 > packetsize)
            len5 = packetsize;
        len4 += ccnl_ccnb_mkBlob(packet+len4, CCN_DTAG_CONTENTDIGEST,
                                 CCN_TT_DTAG, (char*) buf + it*packetsize,
                                 len5);
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
            buf2 = ccnl_malloc(CCNL_MAX_PACKET_SIZE*sizeof(char));
            len5 = ccnl_ccnb_mkHeader(buf2, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // content
            memcpy(buf2+len5, packet, len4);
            len5 +=len4;
            buf2[len5++] = 0; // end-of-interest


            if(it == 0){
                struct ccnl_buf_s *retbuf;
                DEBUGMSG(TRACE, "  enqueue %d %d bytes\n", len4, len5);
                retbuf = ccnl_buf_new((char *)buf2, len5);
                ccnl_face_enqueue(ccnl, from, retbuf);
            }
            else
            {
                struct ccnl_content_s *c;
                char uri[50];
                int contentpos;
                struct ccnl_pkt_s *pkt;

                DEBUGMSG(INFO, "  .. adding to cache %d %d bytes\n", len4, len5);
                sprintf(uri, "/mgmt/seqnum-%d", it);
                pkt = ccnl_calloc(1, sizeof(*pkt));
                pkt->pfx = ccnl_URItoPrefix(uri, CCNL_SUITE_CCNB, NULL, NULL);
                pkt->buf = ccnl_mkSimpleContent(pkt->pfx, buf2, len5, &contentpos);
                pkt->content = pkt->buf->data + contentpos;
                pkt->contlen = len5;
                c = ccnl_content_new(ccnl, &pkt);
                ccnl_content_serve_pending(ccnl, c);
                ccnl_content_add2cache(ccnl, c);
/*
                //put to cache
                struct ccnl_prefix_s *prefix_a = 0;
                struct ccnl_content_s *c = 0;
                struct ccnl_buf_s *pkt = 0;
                unsigned char *content = 0, *cp = buf2;
                unsigned char *ht = (unsigned char *) ccnl_malloc(sizeof(char)*20);
                int contlen;
                pkt = ccnl_ccnb_extract(&cp, &len5, 0, 0, 0, 0,
                                &prefix_a, NULL, NULL, &content, &contlen);

                if (!pkt) {
                     DEBUGMSG(WARNING, " parsing error\n");
                }
                DEBUGMSG(INFO, " prefix is %s\n", ccnl_prefix_to_path(prefix_a));
                prefix_a->compcnt = 2;
                prefix_a->comp = (unsigned char **) ccnl_malloc(sizeof(unsigned char*)*2);
                prefix_a->comp[0] = (unsigned char *)"mgmt";
                sprintf((char*)ht, "seqnum-%d", it);
                prefix_a->comp[1] = ht;
                prefix_a->complen = (int *) ccnl_malloc(sizeof(int)*2);
                prefix_a->complen[0] = strlen("mgmt");
                prefix_a->complen[1] = strlen((char*)ht);
                c = ccnl_content_new(ccnl, CCNL_SUITE_CCNB, &pkt, &prefix_a,
                                     NULL, content, contlen);
                //if (!c) goto Done;

                ccnl_content_serve_pending(ccnl, c);
                ccnl_content_add2cache(ccnl, c);
                //Done:
                //continue;
*/
            }
            ccnl_free(buf2);
#ifdef USE_SIGNATURES
        }
#endif
        ccnl_free(packet);
    }
    return 1;
}

#define ccnl_prefix_clone(P) ccnl_prefix_dup(P)

/*
struct ccnl_prefix_s*
ccnl_prefix_clone(struct ccnl_prefix_s *p)
{
    int i, len;
    struct ccnl_prefix_s *p2;

    p2 = (struct ccnl_prefix_s*) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p2) return NULL;
    for (i = 0, len = 0; i < p->compcnt; len += p->complen[i++]);
    p2->bytes = (unsigned char*) ccnl_malloc(len);
    p2->comp = (unsigned char**) ccnl_malloc(p->compcnt*sizeof(char *));
    p2->complen = (int*) ccnl_malloc(p->compcnt*sizeof(int));
    if (!p2->comp || !p2->complen || !p2->bytes) goto Bail;
    p2->compcnt = p->compcnt;
    for (i = 0, len = 0; i < p->compcnt; len += p2->complen[i++]) {
        p2->complen[i] = p->complen[i];
        p2->comp[i] = p2->bytes + len;
        memcpy(p2->comp[i], p->comp[i], p2->complen[i]);
    }
    return p2;
Bail:
    free_prefix(p2);
    return NULL;
}
*/

// ----------------------------------------------------------------------
// management protocols

#define extractStr(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
        char *s; unsigned char *valptr; int vallen; \
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, &valptr, &vallen) < 0) goto Bail; \
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

    len = ccnl_ccnb_mkHeader(out1+len, CCN_DTAG_NAME, CCN_TT_DTAG);
    len += ccnl_ccnb_mkStrBlob(out1+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(out1+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += ccnl_ccnb_mkStrBlob(out1+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, component_type);
    out1[len++] = 0;

    // prepare FWDENTRY
    len3 = ccnl_ccnb_mkStrBlob(out3, CCN_DTAG_ACTION, CCN_TT_DTAG, answer);

    len += ccnl_ccnb_mkBlob(out1+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) out3, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char *)out1);
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
            len3 += ccnl_ccnb_mkHeader(stmt+len3, CCNL_DTAG_INTERFACE, CCN_TT_DTAG);

            memset(str, 0, 100);
            sprintf(str, "%d", interfaceifndx[it]);
            len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_IFNDX, CCN_TT_DTAG, str);

            memset(str, 0, 100);
            sprintf(str, "%s", interfaceaddr[it]);
            len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_ADDRESS, CCN_TT_DTAG, str);

            memset(str, 0, 100);
            if(interfacedevtype[it] == 1)
            {
                 sprintf(str, "%p", (void *) interfacedev[it]);
                 len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_ETH, CCN_TT_DTAG, str);
            }
            else if(interfacedevtype[it] == 2)
            {
                 sprintf(str, "%p", (void *) interfacedev[it]);
                 len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_SOCK, CCN_TT_DTAG, str);
            }
            else{
                 sprintf(str, "%p", (void *) interfacedev[it]);
                 len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_SOCK, CCN_TT_DTAG, str);
            }

            memset(str, 0, 100);
            sprintf(str, "%d", interfacereflect[it]);
            len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_REFLECT, CCN_TT_DTAG, str);

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
        len3 += ccnl_ccnb_mkHeader(stmt+len3, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);

        memset(str, 0, 100);
        sprintf(str, "%d", faceid[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str,"%p", (void *)facenext[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_NEXT, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str,"%p", (void *)faceprev[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PREV, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str,"%d", faceifndx[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_IFNDX, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str,"%02x", faceflags[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        if(facetype[it] == AF_INET)
            len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_IP, CCN_TT_DTAG, facepeer[it]);
#ifdef USE_LINKLAYER
        else if(facetype[it] == AF_PACKET)
            len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_ETH, CCN_TT_DTAG, facepeer[it]);
#endif
        else if(facetype[it] == AF_UNIX)
            len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_UNIX, CCN_TT_DTAG, facepeer[it]);
        else{
            sprintf(str,"peer=?");
            len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PEER, CCN_TT_DTAG, str);
        }
        // FIXME: dump frag information if present

        stmt[len3++] = 0; //end of faceinstance;
    }
     return len3;
}

static int
ccnl_mgmt_create_fwds_stmt(int num_fwds, long *fwd, long *fwdnext, long *fwdface, int *fwdfaceid, int *suite,
        int *fwdprefixlen, char **fwdprefix, unsigned char *stmt, int len3)
{
    int it;
    char str[100];
    for(it = 0; it < num_fwds; ++it) //FWDS content
    {
         len3 += ccnl_ccnb_mkHeader(stmt+len3, CCN_DTAG_FWDINGENTRY, CCN_TT_DTAG);

         memset(str, 0, 100);
         sprintf(str, "%p", (void *)fwd[it]);
         len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_FWD, CCN_TT_DTAG, str);

         memset(str, 0, 100);
         sprintf(str, "%p", (void *)fwdnext[it]);
         len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_NEXT, CCN_TT_DTAG, str);

         memset(str, 0, 100);
         sprintf(str, "%p", (void *)fwdface[it]);
         len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_FACE, CCN_TT_DTAG, str);

         memset(str, 0, 100);
         sprintf(str, "%d", fwdfaceid[it]);
         len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, str);

         memset(str, 0, 100);
         sprintf(str, "%d", suite[it]);
         len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_SUITE, CCN_TT_DTAG, str);

         len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PREFIX, CCN_TT_DTAG, fwdprefix[it]);

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
        len3 += ccnl_ccnb_mkHeader(stmt+len3, CCN_DTAG_INTEREST, CCN_TT_DTAG);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) interest[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_INTERESTPTR, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) interestnext[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_NEXT, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) interestprev[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PREV, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", interestlast[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_LAST, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", interestmin[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_MIN, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", interestmax[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_MAX, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", interestretries[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_RETRIES, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) interestpublisher[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PUBLISHER, CCN_TT_DTAG, str);

        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PREFIX, CCN_TT_DTAG, interestprefix[it]);

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
        len3 += ccnl_ccnb_mkHeader(stmt+len3, CCN_DTAG_CONTENT, CCN_TT_DTAG);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) content[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_CONTENTPTR, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) contentnext[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_NEXT, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%p", (void *) contentprev[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PREV, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", contentlast_use[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_LASTUSE, CCN_TT_DTAG, str);

        memset(str, 0, 100);
        sprintf(str, "%d", contentserved_cnt[it]);
        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_SERVEDCTN, CCN_TT_DTAG, str);

        len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PREFIX, CCN_TT_DTAG, cprefix[it]);

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

    int *fwdfaceid, *suite ,*fwdprefixlen;
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
    suite = (int*)ccnl_malloc(num_fwds*sizeof(int));
    fwdprefixlen = (int*)ccnl_malloc(num_fwds*sizeof(int));
    fwdprefix = (char**)ccnl_malloc(num_fwds*sizeof(char*));
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

    DEBUGMSG(TRACE, "ccnl_mgmt_debug from=%s\n", ccnl_addr2ascii(&from->peer));
    action = debugaction = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCNL_DTAG_DEBUGREQUEST) goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(debugaction, CCNL_DTAG_DEBUGACTION);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="debug"

    if (debugaction) {
        cp = "debug cmd worked";
        DEBUGMSG(TRACE, "  debugaction is %s\n", debugaction);
        if (!strcmp((char*) debugaction, "dump")){
            ccnl_dump(0, CCNL_RELAY, ccnl);

            get_faces_dump(0, ccnl, faceid, facenext, faceprev, faceifndx,
                           faceflags, facepeer, facetype, facefrag);
            get_fwd_dump(0, ccnl, fwd, fwdnext, fwdface, fwdfaceid, suite,
                         fwdprefixlen, fwdprefix);
            get_interface_dump(0, ccnl, interfaceifndx, interfaceaddr,
                             interfacedev, interfacedevtype, interfacereflect);
            get_interest_dump(0,ccnl, interest, interestnext, interestprev,
                              interestlast, interestmin, interestmax,
                              interestretries, interestpublisher,
                              interestprefixlen, interestprefix);
            get_content_dump(0, ccnl, content, contentnext, contentprev,
                    contentlast_use, contentserved_cnt, cprefixlen, cprefix);

        }
        else if (!strcmp((char*) debugaction, "halt")){
            ccnl->halt_flag = 1;
        }
        else if (!strcmp((char*) debugaction, "dump+halt")) {
            ccnl_dump(0, CCNL_RELAY, ccnl);

            get_faces_dump(0, ccnl, faceid, facenext, faceprev, faceifndx,
                           faceflags, facepeer, facetype, facefrag);
            get_fwd_dump(0, ccnl, fwd, fwdnext, fwdface, fwdfaceid, suite,
                         fwdprefixlen, fwdprefix);
            get_interface_dump(0, ccnl, interfaceifndx, interfaceaddr,
                             interfacedev, interfacedevtype, interfacereflect);
            get_interest_dump(0,ccnl, interest, interestnext, interestprev,
                              interestlast, interestmin, interestmax,
                              interestretries, interestpublisher,
                              interestprefixlen, interestprefix);
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

    len += ccnl_ccnb_mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);
    len += ccnl_ccnb_mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += ccnl_ccnb_mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "debug");
    out[len++] = 0;

    // prepare debug statement
    len3 = ccnl_ccnb_mkHeader(stmt, CCNL_DTAG_DEBUGREQUEST, CCN_TT_DTAG);
    len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, (char*) debugaction);
    len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_DEBUGACTION, CCN_TT_DTAG, cp);
    stmt[len3++] = 0; //end-of-debugstmt

    if(!strcmp((char*) debugaction, "dump") || !strcmp((char*) debugaction, "dump+halt")) //halt returns no content
    {
        len3 += ccnl_ccnb_mkHeader(stmt+len3, CCNL_DTAG_DEBUGREPLY, CCN_TT_DTAG);
        //len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PREFIX, CCN_TT_DTAG, cinterfaces[it]);

        len3 = ccnl_mgmt_create_interface_stmt(num_interfaces, interfaceifndx, interfacedev,
                interfacedevtype, interfacereflect, interfaceaddr, stmt, len3);

        len3 = ccnl_mgmt_create_faces_stmt(num_faces, faceid, facenext, faceprev, faceifndx,
                        faceflags, facetype, facepeer, facefrag, stmt, len3);

        len3 = ccnl_mgmt_create_fwds_stmt(num_fwds, fwd, fwdnext, fwdface, fwdfaceid, suite,
                fwdprefixlen, fwdprefix, stmt, len3);

        len3 = ccnl_mgmt_create_interest_stmt(num_interests, interest, interestnext, interestprev,
                interestlast, interestmin, interestmax, interestretries,
                interestpublisher, interestprefixlen, interestprefix, stmt, len3);

        len3 = ccnl_mgmt_create_content_stmt(num_contents, content, contentnext, contentprev,
                contentlast_use, contentserved_cnt, ccontents, cprefix, stmt, len3);
    }

    stmt[len3++] = 0; //end of debug reply

    len += ccnl_ccnb_mkBlob(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) stmt, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out);

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
    ccnl_free(suite);
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
    unsigned char *action, *macsrc, *ip4src, *ip6src, *proto, *host, *port,
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

    DEBUGMSG(TRACE, "ccnl_mgmt_newface from=%p, ifndx=%d\n",
             (void*) from, from->ifndx);
    action = macsrc = ip4src = ip6src = proto = host = port = NULL;
    path = frag = flags = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FACEINSTANCE) goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(macsrc, CCNL_DTAG_MACSRC);
        extractStr(ip4src, CCNL_DTAG_IP4SRC);
        extractStr(ip6src, CCNL_DTAG_IP6SRC);
        extractStr(path, CCNL_DTAG_UNIXSRC);
        extractStr(proto, CCN_DTAG_IPPROTO);
        extractStr(host, CCN_DTAG_HOST);
        extractStr(port, CCN_DTAG_PORT);
//      extractStr(frag, CCNL_DTAG_FRAG);
        extractStr(flags, CCNL_DTAG_FACEFLAGS);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="newface"

#ifdef USE_LINKLAYER
    if (macsrc && host && port) {
        sockunion su;
        DEBUGMSG(TRACE, "  adding ETH face macsrc=%s, host=%s, ethtype=%s\n",
                 macsrc, host, port);
        memset(&su, 0, sizeof(su));
        su.linklayer.sll_family = AF_PACKET;
        su.linklayer.sll_protocol = htons(strtol((const char*)port, NULL, 0));
        if (sscanf((const char*) host, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   su.linklayer.sll_addr,   su.linklayer.sll_addr+1,
                   su.linklayer.sll_addr+2, su.linklayer.sll_addr+3,
                   su.linklayer.sll_addr+4, su.linklayer.sll_addr+5) == 6) {
        // if (!strcmp(macsrc, "any")) // honouring macsrc not implemented yet
            f = ccnl_get_face_or_create(ccnl, -1, &su.sa, sizeof(su.linklayer));
        }
    } else
#endif
    if (proto && host && port && !strcmp((const char*)proto, "17")) {
        sockunion su;
#ifdef USE_IPV4
	if (ip4src != NULL) {
        DEBUGMSG(TRACE, "  adding IP face ip4src=%s, proto=%s, host=%s, port=%s\n",
                 ip4src, proto, host, port);
        su.sa.sa_family = AF_INET;
        inet_aton((const char*)host, &su.ip4.sin_addr);
        su.ip4.sin_port = htons(strtol((const char*)port, NULL, 0));
        // not implmented yet: honor the requested ip4src parameter
	f = ccnl_get_face_or_create(ccnl, -1, // from->ifndx,
                                    &su.sa, sizeof(struct sockaddr_in));
	}
#endif
#ifdef USE_IPV6
	if (ip6src != NULL) {
        DEBUGMSG(TRACE, "  adding IP face ip6src=%s, proto=%s, host=%s, port=%s\n",
                 ip6src, proto, host, port);
        su.sa.sa_family = AF_INET6;
        inet_pton(AF_INET6, (const char*)host, &su.ip6.sin6_addr.s6_addr);
        su.ip6.sin6_port = htons(strtol((const char*)port, NULL, 0));
        f = ccnl_get_face_or_create(ccnl, -1, // from->ifndx,
                                    &su.sa, sizeof(struct sockaddr_in6));
	}
#endif
    }
#ifdef USE_UNIXSOCKET
    if (path) {
        sockunion su;
        DEBUGMSG(TRACE, "  adding UNIX face unixsrc=%s\n", path);
        su.sa.sa_family = AF_UNIX;
        strcpy(su.ux.sun_path, (char*) path);
        f = ccnl_get_face_or_create(ccnl, -1, // from->ifndx,
                                    &su.sa, sizeof(struct sockaddr_un));
    }
#endif

    if (f) {
        int flagval = flags ?
            strtol((const char*)flags, NULL, 0) : CCNL_FACE_FLAGS_STATIC;
        //      printf("  flags=%s %d\n", flags, flagval);
        DEBUGMSG(TRACE, "  adding a new face (id=%d) worked!\n", f->faceid);
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
            f->frag = ccnl_frag_new(strtol((const char*)frag, NULL, 0), mtu);
        }
#endif
        cp = "newface cmd worked";
    } else {
#ifdef USE_IPV4
	if (ip4src != NULL) {
        DEBUGMSG(TRACE, "  newface request for (macsrc=%s ip4src=%s proto=%s host=%s port=%s frag=%s flags=%s) failed or was ignored\n",
                 macsrc, ip4src, proto, host, port, frag, flags);
	}
#endif
#ifdef USE_IPV6
	if (ip6src != NULL) {
        DEBUGMSG(TRACE, "  newface request for (macsrc=%s ip6src=%s proto=%s host=%s port=%s frag=%s flags=%s) failed or was ignored\n",
                 macsrc, ip6src, proto, host, port, frag, flags);
	}
#endif
    }
    rc = 0;

Bail:
    /*ANSWER*/

    len = ccnl_ccnb_mkHeader(out_buf, CCN_DTAG_NAME, CCN_TT_DTAG);
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newface");
    out_buf[len++] = 0; // end-of-name

    // prepare FACEINSTANCE
    len3 = ccnl_ccnb_mkHeader(faceinst_buf, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    sprintf((char *)retstr,"newface:  %s", cp);
    len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, (char*) retstr);
    if (macsrc)
        len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_MACSRC, CCN_TT_DTAG, (char*) macsrc);
    if (ip4src) {
        len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_IP4SRC, CCN_TT_DTAG, (char*) ip4src);
        len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_IPPROTO, CCN_TT_DTAG, "17");
    }
    if (ip6src) {
        len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_IP6SRC, CCN_TT_DTAG, (char*) ip6src);
        len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_IPPROTO, CCN_TT_DTAG, "17");
    }
    if (host)
        len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_HOST, CCN_TT_DTAG, (char*) host);
    if (port)
        len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_PORT, CCN_TT_DTAG, (char*) port);
    /*
    if (frag)
        len3 += ccnl_ccnb_mkStrBlob(faceinst+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, frag);
    */
    if (flags)
        len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG, (char *) flags);
    if (f) {
        sprintf((char *)faceidstr,"%i",f->faceid);
        len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char *) faceidstr);
    }

    faceinst_buf[len3++] = 0; // end-of-faceinst

    len += ccnl_ccnb_mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf);


    /*END ANWER*/


    ccnl_free(action);
    ccnl_free(macsrc);
    ccnl_free(ip4src);
    ccnl_free(ip6src);
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

    DEBUGMSG(TRACE, "ccnl_mgmt_setfrag from=%p, ifndx=%d\n",
             (void*) from, from->ifndx);
    action = faceid = frag = mtu = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FACEINSTANCE) goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(faceid, CCN_DTAG_FACEID);
        extractStr(frag, CCNL_DTAG_FRAG);
        extractStr(mtu, CCNL_DTAG_MTU);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="newface"

    if (faceid && frag && mtu) {
#ifdef USE_FRAG
        int e = -1;
#endif
        int fi = strtol((const char*)faceid, NULL, 0);

        for (f = ccnl->faces; f && f->faceid != fi; f = f->next);
        if (!f)
            goto Error;

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
        } else if (!strcmp((const char*)frag, "seqd2015")) {
            e = CCNL_FRAG_SEQUENCED2015;
        }
        if (e < 0)
            goto Error;
        f->frag = ccnl_frag_new(e, strtol((const char*)mtu, NULL, 0));
        cp = "setfrag cmd worked";
#else
        cp = "no fragmentation support";
#endif
    } else {
Error:
        DEBUGMSG(TRACE, "  setfrag request for (faceid=%s frag=%s mtu=%s) failed or was ignored\n",
                 faceid, frag, mtu);
    }
    rc = 0;

Bail:
    ccnl_free(action);

    len += ccnl_ccnb_mkHeader(out_buf+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "setfrag");
    out_buf[len++] = 0; // end-of-name

    // prepare FACEINSTANCE
    len3 = ccnl_ccnb_mkHeader(faceinst_buf, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, cp);
    len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid);
    len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, (char*) frag);
    len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_MTU, CCN_TT_DTAG, (char*) mtu);
    faceinst_buf[len3++] = 0; // end-of-faceinst

    len += ccnl_ccnb_mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf);

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

    DEBUGMSG(DEBUG, "ccnl_mgmt_destroyface\n");
    action = faceid = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FACEINSTANCE) goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(faceid, CCN_DTAG_FACEID);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="destroyface"

    if (faceid) {
        struct ccnl_face_s *f;
        int fi = strtol((const char*)faceid, NULL, 0);
        for (f = ccnl->faces; f && f->faceid != fi; f = f->next);
        if (!f) {
            DEBUGMSG(TRACE, "  could not find face=%s\n", faceid);
            goto Bail;
        }
        ccnl_face_remove(ccnl, f);
        DEBUGMSG(TRACE, "  face %s destroyed\n", faceid);
        cp = "facedestroy cmd worked";
    } else {
        DEBUGMSG(TRACE, "  missing faceid\n");
    }
    rc = 0;

Bail:
    /*ANSWER*/
    if(!faceid) {
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "destroyface", cp);
        return -1;
    }

    len += ccnl_ccnb_mkHeader(out_buf+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "destroyface");
    out_buf[len++] = 0; // end-of-name

    // prepare FACEINSTANCE
    len3 = ccnl_ccnb_mkHeader(faceinst_buf, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG);
    len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, cp);
    len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid);
    faceinst_buf[len3++] = 0; // end-of-faceinst

    len += ccnl_ccnb_mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf);


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
    unsigned char *action, *devname, *ip4src, *ip6src, *port, *frag, *flags;
    char *cp = "newdevice cmd worked";
    int rc = -1;

    //variables for answer
    int len = 0, len3;
//    unsigned char contentobj[2000];
//    unsigned char faceinst[2000];
    struct ccnl_if_s *i;


    DEBUGMSG(TRACE, "ccnl_mgmt_newdev\n");
    action = devname = ip4src = ip6src = port = frag = flags = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCNL_DTAG_DEVINSTANCE) goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(devname, CCNL_DTAG_DEVNAME);
        extractStr(ip4src, CCNL_DTAG_IP4SRC);
        extractStr(ip6src, CCNL_DTAG_IP6SRC);
        extractStr(port, CCN_DTAG_PORT);
        extractStr(frag, CCNL_DTAG_FRAG);
        extractStr(flags, CCNL_DTAG_DEVFLAGS);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="newdev"

    if (ccnl->ifcount >= CCNL_MAX_INTERFACES) {
      DEBUGMSG(TRACE, "  too many interfaces, no new interface created\n");
      goto Bail;
    }

#if defined(USE_LINKLAYER) && (defined(CCNL_UNIX) || defined(CCNL_LINUXKERNEL))
    if (devname && port) {
        int portnum;

        cp = "newETHdev cmd worked";
        portnum = port ? strtol((const char*)port, NULL, 0) : CCNL_ETH_TYPE;

        DEBUGMSG(TRACE, "  adding eth device devname=%s, port=%s\n",
                 devname, port);

        // check if it already exists, bail

        // create a new ifs-entry
        i = &ccnl->ifs[ccnl->ifcount];
#ifdef CCNL_LINUXKERNEL
        {
            struct net_device *nd;
            int j;
            nd = ccnl_open_ethdev((char*)devname, &i->addr.linklayer, portnum);
            if (!nd) {
                DEBUGMSG(TRACE, "  could not open device %s\n", devname);
                goto Bail;
            }
            for (j = 0; j < ccnl->ifcount; j++) {
                if (ccnl->ifs[j].netdev == nd) {
                    dev_put(nd);
                    DEBUGMSG(TRACE, "  device %s already open\n", devname);
                    goto Bail;
                }
            }
            i->netdev = nd;
            i->ccnl_packet.type = htons(portnum);
            i->ccnl_packet.dev = i->netdev;
            i->ccnl_packet.func = ccnl_eth_RX;
            dev_add_pack(&i->ccnl_packet);
        }
#elif defined(USE_LINKLAYER)
        i->sock = ccnl_open_ethdev((char*)devname, &i->addr.linklayer, portnum);
        if (!i->sock) {
            DEBUGMSG(TRACE, "  could not open device %s\n", devname);
            goto Bail;
        }
#endif
//      i->frag = frag ? atoi(frag) : 0;
        i->mtu = 1500;
//      we should analyse and copy flags, here we hardcode some defaults:
        i->reflect = 1;
        i->fwdalli = 1;

        if (ccnl->defaultInterfaceScheduler)
            i->sched = ccnl->defaultInterfaceScheduler(ccnl, ccnl_interface_CTS);
        ccnl->ifcount++;

        rc = 0;
        goto Bail;
    }
#endif

    if ((ip4src || ip6src) && port) {
#ifdef USE_IPV4
	if (ip4src) {
        cp = "newUDPdev cmd worked";
        DEBUGMSG(TRACE, "  adding UDP device ip4src=%s, port=%s\n",
                 ip4src, port);

        // check if it already exists, bail

        // create a new ifs-entry
        i = &ccnl->ifs[ccnl->ifcount];
        i->sock = ccnl_open_udpdev(strtol((char*)port, NULL, 0), &i->addr.ip4);
        if (!i->sock) {
            DEBUGMSG(TRACE, "  could not open UDP device %s/%s\n", ip4src, port);
            goto Bail;
        }
	}
#endif
#ifdef USE_IPV6
	if (ip6src) {
        cp = "newUDPdev cmd worked";
        DEBUGMSG(TRACE, "  adding UDP device ip6src=%s, port=%s\n",
                 ip6src, port);

        // check if it already exists, bail

        // create a new ifs-entry
        i = &ccnl->ifs[ccnl->ifcount];
        i->sock = ccnl_open_udp6dev(strtol((char*)port, NULL, 0), &i->addr.ip6);
        if (!i->sock) {
            DEBUGMSG(TRACE, "  could not open UDP device %s/%s\n", ip6src, port);
            goto Bail;
        }
	}
#endif

#ifdef CCNL_LINUXKERNEL
        {
            int j;
            for (j = 0; j < ccnl->ifcount; j++) {
                if (!ccnl_addr_cmp(&ccnl->ifs[j].addr, &i->addr)) {
                    sock_release(i->sock);
#ifdef USE_IPV4
                    DEBUGMSG(TRACE, "  UDP device %s/%s already open\n",
                             ip4src, port);
#elif defined(USE_IPV6)
                    DEBUGMSG(TRACE, "  UDP device %s/%s already open\n",
                             ip6src, port);
#endif
                    goto Bail;
                }
            }
        }

        i->wq = create_workqueue(ccnl_addr2ascii(&i->addr));
        if (!i->wq) {
#ifdef USE_IPV4
            DEBUGMSG(TRACE, "  could not create work queue (UDP device %s/%s)\n", ip4src, port);
#elif defined(USE_IPV6)
            DEBUGMSG(TRACE, "  could not create work queue (UDP device %s/%s)\n", ip6src, port);
#endif
            sock_release(i->sock);
            goto Bail;
        }
        write_lock_bh(&i->sock->sk->sk_callback_lock);
        i->old_data_ready = i->sock->sk->sk_data_ready;
        i->sock->sk->sk_data_ready = ccnl_udp_data_ready;
//      i->sock->sk->sk_user_data = &theRelay;
        write_unlock_bh(&i->sock->sk->sk_callback_lock);
#endif

//      i->frag = frag ? atoi(frag) : 0;
        i->mtu = CCN_DEFAULT_MTU;
//      we should analyse and copy flags, here we hardcode some defaults:
        i->reflect = 0;
        i->fwdalli = 1;

        if (ccnl->defaultInterfaceScheduler)
            i->sched = ccnl->defaultInterfaceScheduler(ccnl, ccnl_interface_CTS);
        ccnl->ifcount++;

        //cp = "newdevice cmd worked";
        rc = 0;
        goto Bail;
    }

#ifdef USE_IPV4
    if (ip4src) {
    DEBUGMSG(TRACE, "  newdevice request for (namedev=%s ip4src=%s port=%s frag=%s) failed or was ignored\n",
             devname, ip4src, port, frag);
    }
#endif
#ifdef USE_IPV6
    if (ip6src) {
    DEBUGMSG(TRACE, "  newdevice request for (namedev=%s ip6src=%s port=%s frag=%s) failed or was ignored\n",
             devname, ip6src, port, frag);
    }
#endif
// #endif // USE_UDP

Bail:

    len += ccnl_ccnb_mkHeader(out_buf+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newdev");
    out_buf[len++] = 0; // end-of-name

    // prepare DEVINSTANCE
    len3 = ccnl_ccnb_mkHeader(faceinst_buf, CCNL_DTAG_DEVINSTANCE, CCN_TT_DTAG);
    len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, cp);
    if (devname)
    len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_DEVNAME, CCN_TT_DTAG,
                      (char *) devname);

    if (devname && port) {
        if (port)
            len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_PORT, CCN_TT_DTAG, (char*) port);
        if (frag)
            len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, (char*) frag);
        if (flags)
            len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG, (char*) flags);
        faceinst_buf[len3++] = 0; // end-of-faceinst
    }
    else if ((ip4src && port) || (ip6src && port)) {
        if (ip4src)
            len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_IP4SRC, CCN_TT_DTAG, (char*) ip4src);
        if (ip6src)
            len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_IP6SRC, CCN_TT_DTAG, (char*) ip6src);
        if (port)
            len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCN_DTAG_PORT, CCN_TT_DTAG, (char*) port);
        if (frag)
            len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_FRAG, CCN_TT_DTAG, (char*) frag);
        if (flags)
            len3 += ccnl_ccnb_mkStrBlob(faceinst_buf+len3, CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG, (char*) flags);
        faceinst_buf[len3++] = 0; // end-of-faceinst
    }

    len += ccnl_ccnb_mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf);

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

    DEBUGMSG(TRACE, "mgmt_destroydev not implemented yet\n");
    /*ANSWER*/
    ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "mgmt_destroy", "mgmt_destroydev not implemented yet");

    /*END ANSWER*/
    return -1;
}

#ifdef USE_ECHO

int
ccnl_mgmt_echo(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
               struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    unsigned char *buf;
    int buflen, num, typ;
    struct ccnl_prefix_s *p = NULL;
    unsigned char *action, *suite=0, h[10];
    char *cp = "echoserver cmd failed";
    int rc = -1;

    int len = 0, len3;

    DEBUGMSG(TRACE, "ccnl_mgmt_echo\n");
    action = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FWDINGENTRY) goto Bail;

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p) goto Bail;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                                           sizeof(unsigned char*));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end

        if (typ == CCN_TT_DTAG && num == CCN_DTAG_NAME) {
            for (;;) {
                if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
                if (num==0 && typ==0)
                    break;
                if (typ == CCN_TT_DTAG && num == CCN_DTAG_COMPONENT &&
                    p->compcnt < CCNL_MAX_NAME_COMP) {
                        // if (ccnl_grow_prefix(p)) goto Bail;
                    if (ccnl_ccnb_consume(typ, num, &buf, &buflen,
                                p->comp + p->compcnt,
                                p->complen + p->compcnt) < 0) goto Bail;
                    p->compcnt++;
                } else {
                    if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
                }
            }
            continue;
        }

        extractStr(action, CCN_DTAG_ACTION);
        extractStr(suite, CCNL_DTAG_SUITE);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="prefixreg"
    if (suite && *suite >= 0 && *suite < CCNL_SUITE_LAST && p->compcnt > 0) {
        p->suite = *suite;
        DEBUGMSG(TRACE, "mgmt: activating echo server for %s, suite=%s\n",
                 ccnl_prefix_to_path(p), ccnl_suite2str(*suite));
        ccnl_echo_add(ccnl, ccnl_prefix_clone(p));
        cp = "echoserver cmd worked";
    } else {
        DEBUGMSG(TRACE, "mgmt: ignored echoserver\n");
    }

    rc = 0;

Bail:
    /*ANSWER*/
    if(!action || !p) {
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "echoserver", cp);
        return -1;
    }
    len += ccnl_ccnb_mkHeader(out_buf+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, (char*) action);
    out_buf[len++] = 0; // end-of-name

    // prepare FWDENTRY
    len3 = ccnl_ccnb_mkHeader(fwdentry_buf, CCNL_DTAG_PREFIX, CCN_TT_DTAG);
    len3 += ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, cp);
    len3 += ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, CCN_DTAG_NAME, CCN_TT_DTAG, ccnl_prefix_to_path(p)); // prefix

    //    len3 += ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid);
    memset(h,0,sizeof(h));
    sprintf((char*)h, "%d", (int)suite[0]);
    len3 += ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, CCNL_DTAG_SUITE, CCN_TT_DTAG, (char*) h);
    fwdentry_buf[len3++] = 0; // end-of-fwdentry

    len += ccnl_ccnb_mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) fwdentry_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf);

    /*END ANWER*/

    ccnl_free(suite);
    ccnl_free(action);
    free_prefix(p);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

#endif // USE_ECHO

int
ccnl_mgmt_prefixreg(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    unsigned char *buf;
    int buflen, num, typ;
    struct ccnl_prefix_s *p = NULL;
    unsigned char *action, *faceid, *suite=0, h[10];
    char *cp = "prefixreg cmd failed";
    int rc = -1;

    int len = 0, len3;
//    unsigned char contentobj[2000];
//    unsigned char fwdentry[2000];

    DEBUGMSG(TRACE, "ccnl_mgmt_prefixreg\n");
    action = faceid = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FWDINGENTRY) goto Bail;

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p) goto Bail;
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP *
                                           sizeof(unsigned char*));
    p->complen = (int*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(int));
    if (!p->comp || !p->complen) goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end

        if (typ == CCN_TT_DTAG && num == CCN_DTAG_NAME) {
            for (;;) {
                if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
                if (num==0 && typ==0)
                    break;
                if (typ == CCN_TT_DTAG && num == CCN_DTAG_COMPONENT &&
                    p->compcnt < CCNL_MAX_NAME_COMP) {
                        // if (ccnl_grow_prefix(p)) goto Bail;
                    if (ccnl_ccnb_consume(typ, num, &buf, &buflen,
                                p->comp + p->compcnt,
                                p->complen + p->compcnt) < 0) goto Bail;
                    p->compcnt++;
                } else {
                    if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
                }
            }
            continue;
        }

        extractStr(action, CCN_DTAG_ACTION);
        extractStr(faceid, CCN_DTAG_FACEID);
        extractStr(suite, CCNL_DTAG_SUITE);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }

    // should (re)verify that action=="prefixreg"
    if (faceid && p->compcnt > 0) {
        struct ccnl_face_s *f;
        struct ccnl_forward_s *fwd, **fwd2;
        int fi = strtol((const char*)faceid, NULL, 0);

        p->suite = suite[0];

        DEBUGMSG(TRACE, "mgmt: adding prefix %s to faceid=%s, suite=%s\n",
                 ccnl_prefix_to_path(p), faceid, ccnl_suite2str(suite[0]));

        for (f = ccnl->faces; f && f->faceid != fi; f = f->next);
        if (!f) goto Bail;

//      printf("Face %s found\n", faceid);
        fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
        if (!fwd) goto Bail;
        fwd->prefix = ccnl_prefix_clone(p);
        fwd->face = f;
        if (suite)
            fwd->suite = suite[0];

        fwd2 = &ccnl->fib;
        while (*fwd2)
            fwd2 = &((*fwd2)->next);
        *fwd2 = fwd;
        cp = "prefixreg cmd worked";
    } else {
        DEBUGMSG(TRACE, "mgmt: ignored prefixreg faceid=%s\n", faceid);
    }

    rc = 0;

Bail:
    /*ANSWER*/
    if(!action || !p || ! faceid) {
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "prefixreg", cp);
        return -1;
    }
    len += ccnl_ccnb_mkHeader(out_buf+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += ccnl_ccnb_mkStrBlob(out_buf+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, (char*) action);
    out_buf[len++] = 0; // end-of-name

    // prepare FWDENTRY
    len3 = ccnl_ccnb_mkHeader(fwdentry_buf, CCNL_DTAG_PREFIX, CCN_TT_DTAG);
    len3 += ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, CCN_DTAG_ACTION, CCN_TT_DTAG, cp);
    len3 += ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, CCN_DTAG_NAME, CCN_TT_DTAG, ccnl_prefix_to_path(p)); // prefix

    len3 += ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid);
    memset(h,0,sizeof(h));
    sprintf((char*)h, "%d", (int)suite[0]);
    len3 += ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, CCNL_DTAG_SUITE, CCN_TT_DTAG, (char*) h);
    fwdentry_buf[len3++] = 0; // end-of-fwdentry

    len += ccnl_ccnb_mkBlob(out_buf+len, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) fwdentry_buf, len3);

    ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf);

    /*END ANWER*/


    ccnl_free(suite);
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
    unsigned char *components = 0, *h = 0, *h2 = 0, *h3 = 0;
    int buflen;
    unsigned int chunknum = 0, chunkflag = 0;
    int num, typ, num_of_components = -1, suite = 2;
    struct ccnl_prefix_s *prefix_new;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0)
        goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ)
        goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0){
        if (num==0 && typ==0)
            break; // end
        extractStr(h, CCNL_DTAG_SUITE);
        extractStr(h2, CCNL_DTAG_CHUNKNUM);
        extractStr(h3, CCNL_DTAG_CHUNKFLAG);
        if (h) {
            suite = strtol((const char*)h, NULL, 0);
            ccnl_free(h);
            h=0;
        }
        if(h2){
           chunknum = strtol((const char*)h2, NULL, 0);
           ccnl_free(h2);
           h2=0;
        }
        if(h3){
           chunkflag = strtol((const char*)h3, NULL, 0);
           ccnl_free(h3);
           h3=0;
           break;
        }
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0)
            goto Bail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME)
        goto Bail;

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0)
        goto Bail;
    if (typ != CCN_TT_BLOB)
        goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        extractStr(components, CCN_DTAG_COMPONENT);
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0)
            goto Bail;
    }
    ++num_of_components;
    prefix_new = ccnl_URItoPrefix((char *)components, CCNL_SUITE_CCNB, NULL, chunkflag ? &chunknum : NULL);

    ccnl_free(components);
    components = NULL;
    prefix_new->suite = suite;

    DEBUGMSG(TRACE, "  mgmt: adding object %s to cache (suite=%s)\n",
             ccnl_prefix_to_path(ccnl_prefix_dup(prefix_new)), ccnl_suite2str(suite));

    //Reply MSG
    if (h)
        ccnl_free(h);
    h = ccnl_malloc(300);

    sprintf((char *)h, "received add to cache request, inizializing callback for %s", ccnl_prefix_to_path(prefix_new));
    ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from,
                             "addcacheobject", (char *)h);
    if (h)
        ccnl_free(h);

    //Reply MSG END
    {
        struct ccnl_pkt_s *pkt;
        struct ccnl_interest_s *interest;
        struct ccnl_buf_s *buffer;

        pkt = ccnl_calloc(1, sizeof(*pkt));
        pkt->suite = prefix_new->suite;
        switch(pkt->suite) {
        case CCNL_SUITE_CCNB:
            pkt->s.ccnb.maxsuffix = CCNL_MAX_NAME_COMP;
            break;
        case CCNL_SUITE_NDNTLV:
            pkt->s.ndntlv.maxsuffix = CCNL_MAX_NAME_COMP;
            break;
        default:
            break;
        }

        pkt->pfx = prefix_new;
        pkt->buf = ccnl_mkSimpleInterest(prefix_new, NULL);
        pkt->val.final_block_id = -1;
        buffer = buf_dup(pkt->buf);

        interest = ccnl_interest_new(ccnl, from, &pkt);

        if (!interest)
            return 0;

        //Send interest to from!
        ccnl_face_enqueue(ccnl, from, buffer);
    }
//    free_prefix(prefix_new);

Bail:
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

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) goto Bail;

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_BLOB) goto Bail;

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME) goto Bail;

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num==0 && typ==0)
            break; // end
        ++num_of_components;
        extractStr(components[num_of_components], CCN_DTAG_COMPONENT);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
    }
    ++num_of_components;

    for (c2 = ccnl->contents; c2; c2 = c2->next)
    {
        if(c2->pkt->pfx->compcnt != num_of_components) continue;
        for(i = 0; i < num_of_components; ++i)
        {
            if(strcmp((char*)c2->pkt->pfx->comp[i], (char*)components[i]))
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
        DEBUGMSG(TRACE, "Content found\n");
        ccnl_content_remove(ccnl, c2);
    }else
    {
       DEBUGMSG(TRACE, "Ignore request since content not found\n");
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
ccnl_mgmt_validate_signature(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
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
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_INTEREST) goto Bail;

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) < 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME) goto Bail;

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) != 0) goto Bail;
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_SIGNATURE) goto Bail;
    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {

        if (num==0 && typ==0)
            break; // end

        extractStr(sigtype, CCN_DTAG_NAME);
        siglen = buflen;
        extractStr(sig, CCN_DTAG_SIGNATUREBITS);
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0) < 0) goto Bail;
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
    DEBUGMSG(TRACE, "ccnl_mgmt_handle \"%s\"\n", cmd);
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
#ifdef USE_ECHO
    else if (!strcmp(cmd, "echoserver"))
        ccnl_mgmt_echo(ccnl, orig, prefix, from);
#endif
    else if (!strcmp(cmd, "newface"))
        ccnl_mgmt_newface(ccnl, orig, prefix, from);
    else if (!strcmp(cmd, "destroyface"))
        ccnl_mgmt_destroyface(ccnl, orig, prefix, from);
    else if (!strcmp(cmd, "prefixreg"))
        ccnl_mgmt_prefixreg(ccnl, orig, prefix, from);
//  TODO: Add ccnl_mgmt_prefixunreg(ccnl, orig, prefix, from)
//  else if (!strcmp(cmd, "prefixunreg"))
//      ccnl_mgmt_prefixunreg(ccnl, orig, prefix, from);
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
        DEBUGMSG(TRACE, "unknown mgmt command %s\n", cmd);
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

    DEBUGMSG(TRACE, "ccnl_mgmt request \"%s\"\n", cmd);

    if (ccnl_is_local_addr(&from->peer)) goto MGMT;

#ifdef USE_SIGNATURES
    return ccnl_mgmt_validate_signature(ccnl, orig, prefix, from, cmd);
#endif /*USE_SIGNATURES*/

    DEBUGMSG(TRACE, "  rejecting because src=%s is not a local addr\n",
            ccnl_addr2ascii(&from->peer));
    ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, cmd,
                "refused: origin of mgmt cmd is not local");
    return -1;

    MGMT:
    ccnl_mgmt_handle(ccnl, orig, prefix, from, cmd, 1);

    return 0;
}

#endif // USE_MGMT

// eof

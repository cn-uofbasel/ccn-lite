/*
 * @f ccnl-ext-mgmt.c
 * @b CCN lite extension, management logic (face mgmt and registration)
 *
 * Copyright (C) 2012-18, Christian Tschudin, University of Basel
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
 */

#ifndef CCNL_LINUXKERNEL
#include "ccnl-mgmt.h"
#include "ccnl-core.h"
#include "ccnl-pkt-ccnb.h"
#include "ccnl-pkt-builder.h"
#include "ccnl-dump.h"
#include "ccnl-crypto.h"
#include "ccnl-forward.h"
#include "ccnl-pkt-switch.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <limits.h>
#else
#include "../include/ccnl-mgmt.h"
#include "../include/ccnl-core.h"
#include "../../ccnl-pkt/include/ccnl-pkt-ccnb.h"
#include "../../ccnl-pkt/include/ccnl-pkt-builder.h"
#include "../include/ccnl-dump.h"
#include "../include/ccnl-crypto.h"
#include "../include/ccnl-forward.h"
#include "../../ccnl-pkt/include/ccnl-pkt-switch.h"
#endif


#ifdef USE_MGMT

#ifndef CCNL_LINUXKERNEL
#include "ccnl-unix.h"
#endif

#define CONTENTOBJ_BUF_SIZE 2000
#define FACEINST_BUF_SIZE 2000
#define OUT_BUF_SIZE 2000
#define FWDENTRY_BUF_SIZE 2000
#define OUT1_SIZE 2000
#define OUT2_SIZE 1000
#define OUT3_SIZE 500

unsigned char contentobj_buf[CONTENTOBJ_BUF_SIZE];
unsigned char faceinst_buf[FACEINST_BUF_SIZE];
unsigned char out_buf[OUT_BUF_SIZE];
unsigned char fwdentry_buf[FWDENTRY_BUF_SIZE];
unsigned char out1[OUT1_SIZE], out2[OUT2_SIZE], out3[OUT3_SIZE];

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


int8_t
ccnl_mgmt_parse_eth_address(uint8_t *sll_addr, const char *str) {
    char *endptr;
    unsigned long octet_l;
    size_t i;
    for (i = 0; i < 6; ++i) {
        errno = 0;
        octet_l = strtoul(str + 3*i, &endptr, 16);
        if (errno || octet_l > UINT8_MAX || (i != 5 && *endptr != ':')) {
            DEBUGMSG(ERROR, "Could not parse ethernet address: %s\n", str);
            return -1;
        }
        sll_addr[i] = (uint8_t) octet_l;
    }
    sll_addr[6] = 0;
    sll_addr[7] = 0;
    return 0;
}


// ----------------------------------------------------------------------

int8_t
ccnl_mgmt_send_return_split(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                struct ccnl_prefix_s *prefix, struct ccnl_face_s *from,
                size_t len, uint8_t *buf)
{

    size_t it, size = CCNL_MAX_PACKET_SIZE / 2;
    size_t numPackets = len/(size/2) + 1;
    uint8_t *packet = NULL, *buf2 = NULL, *packetend = NULL, *buf2end = NULL;
    struct ccnl_pkt_s *pkt = NULL;
    struct ccnl_content_s *c = NULL;
    (void) orig;
    (void) prefix;
    DEBUGMSG(DEBUG, "ccnl_mgmt_send_return_split %zu bytes, %zu packet(s)\n",
             len, numPackets);

    for (it = 0; it < numPackets; ++it) {
        size_t packetsize = size/2, len4 = 0, len5 = 0;
        packet = (uint8_t*) ccnl_malloc(sizeof(uint8_t) * packetsize * 2);
        if (!packet) {
            goto Bail;
        }
        packetend = packet + sizeof(uint8_t) * packetsize * 2;

        if (ccnl_ccnb_mkHeader(packet+len4, packetend, CCNL_DTAG_FRAG, CCN_TT_DTAG, &len4)) {
            goto Bail;
        }
        if (it == numPackets - 1) {
            if (ccnl_ccnb_mkStrBlob(packet+len4, packetend, CCN_DTAG_ANY, CCN_TT_DTAG, "last", &len4)) {
                goto Bail;
            }
        }
        len5 = len - it * packetsize;
        if (len5 > packetsize) {
            len5 = packetsize;
        }
        if (ccnl_ccnb_mkBlob(packet+len4, packetend, CCN_DTAG_CONTENTDIGEST,
                             CCN_TT_DTAG, (char*) buf + it*packetsize,
                             len5, &len4)) {
            goto Bail;
        }
        if (packet + len4 + 1 >= packetend) {
            goto Bail;
        }
        packet[len4++] = 0;

//#ifdef USE_SIGNATURES
        //        if(it == 0) id = from->faceid;

#ifdef USE_SIGNATURES
        if (!ccnl_is_local_addr(&from->peer)) {
            //                ccnl_crypto_sign(ccnl, packet, len4, "ccnl_mgmt_crypto", id);
            ccnl_crypto_sign(ccnl, packet, len4, "ccnl_mgmt_crypto",
                             it ? -it : from->faceid);
        } else {
#endif
            //send back the first part,
            //store the other parts in cache, after checking the pit
            buf2 = (uint8_t*) ccnl_malloc(CCNL_MAX_PACKET_SIZE*sizeof(char));
            if (!buf2) {
                goto Bail;
            }
            buf2end = buf2 + CCNL_MAX_PACKET_SIZE*sizeof(char);
            // Reset len5 and reuse it for the packet buffer buf2
            len5 = 0;
            if (ccnl_ccnb_mkHeader(buf2, buf2end, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG, &len5)) {   // content
                goto Bail;
            }
            if (buf2 + len5 + len4 + 1 >= buf2end) {
                goto Bail;
            }
            memcpy(buf2+len5, packet, len4);
            len5 += len4;
            buf2[len5++] = 0; // end-of-interest

            if (it == 0) {
                struct ccnl_buf_s *retbuf;
                DEBUGMSG(TRACE, "  enqueue %zu %zu bytes\n", len4, len5);
                retbuf = ccnl_buf_new((char *)buf2, len5);
                if (!retbuf) {
                    goto Bail;
                }
                ccnl_face_enqueue(ccnl, from, retbuf);
            } else {
                char uri[50];
                size_t contentpos;

                DEBUGMSG(INFO, "  .. adding to cache %zu %zu bytes\n", len4, len5);
                snprintf(uri, sizeof(uri), "/mgmt/seqnum-%zu", it);
                pkt = ccnl_calloc(1, sizeof(*pkt));
                if (!pkt) {
                    goto Bail;
                }
                pkt->pfx = ccnl_URItoPrefix(uri, CCNL_SUITE_CCNB, NULL);
                if (!pkt->pfx) {
                    goto Bail;
                }
                pkt->buf = ccnl_mkSimpleContent(pkt->pfx, buf2, len5, &contentpos, NULL);
                if (!pkt->buf) {
                    goto Bail;
                }
                pkt->content = pkt->buf->data + contentpos;
                pkt->contlen = len5;
                c = ccnl_content_new(&pkt);
                if (!c) {
                    goto Bail;
                }
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
    return 0;
Bail:
    if (packet) {
        ccnl_free(packet);
    }
    if (buf2) {
        ccnl_free(buf2);
    }
    if (pkt) {
        ccnl_pkt_free(pkt);
    }
    if (c) {
        ccnl_content_free(c);
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
    ccnl_prefix_free(p2);
    return NULL;
}
*/

// ----------------------------------------------------------------------
// management protocols

#define extractStr(VAR,DTAG) \
    if (typ == CCN_TT_DTAG && num == DTAG) { \
        char *s; uint8_t *valptr; size_t vallen; \
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, &valptr, &vallen)) { goto Bail; } \
        s = ccnl_malloc(vallen+1); if (!s) { goto Bail; } \
        memcpy(s, valptr, vallen); s[vallen] = '\0'; \
        ccnl_free(VAR); \
        VAR = (uint8_t*) s; \
        continue; \
    } do {} while(0)


int8_t
ccnl_mgmt_return_ccn_msg(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                         struct ccnl_prefix_s *prefix, struct ccnl_face_s *from,
                         char *component_type, char* answer)
{
    size_t len = 0, len3 = 0;

    if (ccnl_ccnb_mkHeader(out1+len, out1 + OUT1_SIZE, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {
        return -1;
    }

    if (ccnl_ccnb_mkStrBlob(out1+len, out1 + OUT1_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len, out1 + OUT1_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len)) {
        return -1;
    }
    if (ccnl_ccnb_mkStrBlob(out1+len, out1 + OUT1_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, component_type, &len)) {
        return -1;
    }
    if (len + 1 >= OUT1_SIZE) {
        return -1;
    }
    out1[len++] = 0;

    // prepare FWDENTRY
    if (ccnl_ccnb_mkStrBlob(out3, out3 + OUT3_SIZE, CCN_DTAG_ACTION, CCN_TT_DTAG, answer, &len3)) {
        return -1;
    }

    if (ccnl_ccnb_mkBlob(out1+len, out1 + OUT1_SIZE, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                         (char*) out3, len3, &len)) {
        return -1;
    }

    if (ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char *) out1)) {
        return -1;
    }
    return 0;
}


static int8_t
ccnl_mgmt_create_interface_stmt(size_t num_interfaces, int *interfaceifndx, long *interfacedev,
        int *interfacedevtype, int *interfacereflect, char **interfaceaddr, uint8_t *stmt, const uint8_t *stmtend,
        size_t *len3)
{
    size_t it;
    int ret;
    char str[100];
    for (it = 0; it < num_interfaces; ++it) {  // interface content
        if (ccnl_ccnb_mkHeader(stmt+*len3, stmtend, CCNL_DTAG_INTERFACE, CCN_TT_DTAG, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str), "%d", interfaceifndx[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_IFNDX, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        ret = snprintf(str, sizeof(str), "%s", interfaceaddr[it]);
        if (ret < 0 || (unsigned) ret >= sizeof(str)) {
            return -1;
        }
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_ADDRESS, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        if (interfacedevtype[it] == 1) {
            snprintf(str, sizeof(str), "%p", (void *) interfacedev[it]);
            if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_ETH, CCN_TT_DTAG, str, len3)) {
                return -1;
            }
        } else if(interfacedevtype[it] == 2) {
            snprintf(str, sizeof(str), "%p", (void *) interfacedev[it]);
            if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_SOCK, CCN_TT_DTAG, str, len3)) {
                return -1;
            }
        } else {
            snprintf(str, sizeof(str), "%p", (void *) interfacedev[it]);
            if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_SOCK, CCN_TT_DTAG, str, len3)) {
                return -1;
            }
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str), "%d", interfacereflect[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_REFLECT, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        if (stmt + *len3 + 1 >= stmtend) {
            return -1;
        }
        stmt[(*len3)++] = 0; //end of fwd;
    }
    return 0;
}

static int8_t
ccnl_mgmt_create_faces_stmt(size_t num_faces, int *faceid, long *facenext,
                      long *faceprev, int *faceifndx, int *faceflags,
                      int *facetype, char **facepeer, char **facefrag,
                      unsigned char *stmt, const uint8_t *stmtend, size_t *len3)
{
    size_t it;
    char str[100];
    (void) facefrag;
    for (it = 0; it < num_faces; ++it) {  //FACES CONTENT
        if (ccnl_ccnb_mkHeader(stmt+*len3, stmtend, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str), "%d", faceid[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCN_DTAG_FACEID, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str), "%p", (void *) facenext[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_NEXT, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str), "%p", (void *)faceprev[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_PREV, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str), "%d", faceifndx[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_IFNDX, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str), "%02x", faceflags[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        if(facetype[it] == AF_INET) {
            if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_IP, CCN_TT_DTAG, facepeer[it], len3)) {
                return -1;
            }
#ifdef USE_LINKLAYER
#if !(defined(__FreeBSD__) || defined(__APPLE__))
        } else if(facetype[it] == AF_PACKET) {
            if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_ETH, CCN_TT_DTAG, facepeer[it], len3)) {
                return -1;
            }
#endif
#endif
        } else if(facetype[it] == AF_UNIX) {
            if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_UNIX, CCN_TT_DTAG, facepeer[it], len3)) {
                return -1;
            }
        } else {
            snprintf(str, sizeof(str), "peer=?");
            if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_PEER, CCN_TT_DTAG, str, len3)) {
                return -1;
            }
        }
        // FIXME: dump frag information if present

        if (stmt + *len3 >= stmtend) {
            return -1;
        }

        stmt[(*len3)++] = 0; //end of faceinstance;
    }
     return 0;
}

static int8_t
ccnl_mgmt_create_fwds_stmt(size_t num_fwds, long *fwd, long *fwdnext, long *fwdface, int *fwdfaceid, int *suite,
        int *fwdprefixlen, char **fwdprefix, uint8_t *stmt, const uint8_t *stmtend, size_t *len3)
{
    size_t it;
    char str[100];
    (void) fwdprefixlen;
    for (it = 0; it < num_fwds; ++it) {  //FWDS content
         if (ccnl_ccnb_mkHeader(stmt+*len3, stmtend, CCN_DTAG_FWDINGENTRY, CCN_TT_DTAG, len3)) {
             return -1;
         }

         memset(str, 0, sizeof(str));
         snprintf(str, sizeof(str),  "%p", (void *)fwd[it]);
         if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_FWD, CCN_TT_DTAG, str, len3)) {
             return -1;
         }

         memset(str, 0, sizeof(str));
         snprintf(str, sizeof(str),  "%p", (void *)fwdnext[it]);
         if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_NEXT, CCN_TT_DTAG, str, len3)) {
             return -1;
         }

         memset(str, 0, sizeof(str));
         snprintf(str, sizeof(str),  "%p", (void *)fwdface[it]);
         if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_FACE, CCN_TT_DTAG, str, len3)) {
             return -1;
         }

         memset(str, 0, sizeof(str));
         snprintf(str, sizeof(str),  "%d", fwdfaceid[it]);
         if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCN_DTAG_FACEID, CCN_TT_DTAG, str, len3)) {
             return -1;
         }

         memset(str, 0, sizeof(str));
         snprintf(str, sizeof(str),  "%d", suite[it]);
         if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_SUITE, CCN_TT_DTAG, str, len3)) {
             return -1;
         }

         if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_PREFIX, CCN_TT_DTAG, fwdprefix[it], len3)) {
             return -1;
         }

         if (stmt + *len3 + 1 >= stmtend) {
             return -1;
         }
         stmt[(*len3)++] = 0; //end of fwd;

    }
    return 0;
}

static int8_t
ccnl_mgmt_create_interest_stmt(size_t num_interests, long *interest, long *interestnext, long *interestprev,
        int *interestlast, int *interestmin, int *interestmax, int *interestretries,
        long *interestpublisher, int* interestprefixlen, char **interestprefix,
        uint8_t *stmt, const uint8_t *stmtend, size_t *len3)
{
    size_t it;
    char str[100];
    (void) interestprefixlen;
    for (it = 0; it < num_interests; ++it) {  // interest content
        if (ccnl_ccnb_mkHeader(stmt+*len3, stmtend, CCN_DTAG_INTEREST, CCN_TT_DTAG, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%p", (void *) interest[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_INTERESTPTR, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%p", (void *) interestnext[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_NEXT, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%p", (void *) interestprev[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_PREV, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%d", interestlast[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_LAST, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%d", interestmin[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_MIN, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%d", interestmax[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_MAX, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%d", interestretries[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_RETRIES, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%p", (void *) interestpublisher[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_PUBLISHER, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_PREFIX, CCN_TT_DTAG, interestprefix[it], len3)) {
            return -1;
        }

        if (stmt + *len3 + 1 >= stmtend) {
            return -1;
        }
        stmt[(*len3)++] = 0; //end of interest;
    }
    return 0;
}

static int8_t
ccnl_mgmt_create_content_stmt(size_t num_contents, long *content, long *contentnext,
        long *contentprev, int *contentlast_use, int *contentserved_cnt,
        char **ccontents, char **cprefix, uint8_t *stmt, const uint8_t *stmtend, size_t *len3)
{
    size_t it;
    char str[100];
    (void) ccontents;
    for (it = 0; it < num_contents; ++it) {  // content content
        if (ccnl_ccnb_mkHeader(stmt+*len3, stmtend, CCN_DTAG_CONTENT, CCN_TT_DTAG, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%p", (void *) content[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_CONTENTPTR, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%p", (void *) contentnext[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_NEXT, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%p", (void *) contentprev[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_PREV, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%d", contentlast_use[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_LASTUSE, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str),  "%d", contentserved_cnt[it]);
        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_SERVEDCTN, CCN_TT_DTAG, str, len3)) {
            return -1;
        }

        if (ccnl_ccnb_mkStrBlob(stmt+*len3, stmtend, CCNL_DTAG_PREFIX, CCN_TT_DTAG, cprefix[it], len3)) {
            return -1;
        }

        if (stmt + *len3 + 1 >= stmtend) {
            return -1;
        }
        stmt[(*len3)++] = 0; //end of content;
    }
    return 0;
}

int8_t
ccnl_mgmt_debug(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    unsigned char *buf = NULL, *action = NULL, *debugaction = NULL;
    size_t it;

    int *faceid = NULL, *faceifndx = NULL, *faceflags = NULL, *facetype = NULL; //store face-info
    long *facenext = NULL, *faceprev = NULL;
    char **facepeer = NULL, **facefrag = NULL;

    int *fwdfaceid = NULL, *suite = NULL, *fwdprefixlen = NULL;
    long *fwd = NULL, *fwdnext = NULL, *fwdface = NULL;
    char **fwdprefix = NULL;

    int *interfaceifndx = NULL, *interfacedevtype = NULL, *interfacereflect = NULL;
    long *interfacedev = NULL;
    char **interfaceaddr = NULL;

    int *interestlast = NULL, *interestmin = NULL, *interestmax = NULL, *interestretries = NULL,
        *interestprefixlen = NULL;
    long *interest = NULL, *interestnext = NULL, *interestprev = NULL, *interestpublisher = NULL;
    char **interestprefix = NULL;

    int *contentlast_use = NULL, *contentserved_cnt = NULL, *cprefixlen = NULL;
    long *content = NULL, *contentnext = NULL, *contentprev = NULL;
    char **ccontents = NULL, **cprefix = NULL;

    size_t num_faces = 0, num_fwds = 0, num_interfaces = 0, num_interests = 0, num_contents = 0, buflen = 0;
    uint64_t num;
    uint8_t typ;
    char *cp = "debug cmd failed";
    int8_t rc = -1;

    //variables for answer
    size_t stmt_length, object_length, contentobject_length;
    uint8_t *out = NULL, *contentobj = NULL, *stmt = NULL;
    size_t len = 0, len3 = 0;

    //Alloc memory storage for face answer
    num_faces = (size_t) get_num_faces(ccnl);
    facepeer = (char**) ccnl_calloc(num_faces, sizeof(char*));
    if (!facepeer) {
        goto Bail;
    }
    facefrag = (char**) ccnl_calloc(num_faces, sizeof(char*));
    if (!facefrag) {
        goto Bail;
    }
    for (it = 0; it < num_faces; ++it) {
        facepeer[it] = (char*) ccnl_malloc(50*sizeof(char));
        if (!facepeer[it]) {
            goto Bail;
        }
        facefrag[it] = (char*) ccnl_malloc(50*sizeof(char));
        if (!facefrag[it]) {
            goto Bail;
        }
    }
    faceid = (int*) ccnl_malloc(num_faces*sizeof(int));
    if (!faceid) {
        goto Bail;
    }
    facenext = (long*) ccnl_malloc(num_faces*sizeof(long));
    if (!facenext) {
        goto Bail;
    }
    faceprev = (long*) ccnl_malloc(num_faces*sizeof(long));
    if (!faceprev) {
        goto Bail;
    }
    faceifndx = (int*) ccnl_malloc(num_faces*sizeof(int));
    if (!faceifndx) {
        goto Bail;
    }
    faceflags = (int*) ccnl_malloc(num_faces*sizeof(int));
    if (!faceflags) {
        goto Bail;
    }
    facetype = (int*) ccnl_malloc(num_faces*sizeof(int));
    if (!facetype) {
        goto Bail;
    }

    //Alloc memory storage for fwd answer
    num_fwds = (size_t) get_num_fwds(ccnl);
    fwd = (long*) ccnl_malloc(num_fwds*sizeof(long));
    if (!fwd) {
        goto Bail;
    }
    fwdnext = (long*) ccnl_malloc(num_fwds*sizeof(long));
    if (!fwdnext) {
        goto Bail;
    }
    fwdface = (long*) ccnl_malloc(num_fwds*sizeof(long));
    if (!fwdface) {
        goto Bail;
    }
    fwdfaceid = (int*) ccnl_malloc(num_fwds*sizeof(int));
    if (!fwdfaceid) {
        goto Bail;
    }
    suite = (int*) ccnl_malloc(num_fwds*sizeof(int));
    if (!suite) {
        goto Bail;
    }
    fwdprefixlen = (int*) ccnl_malloc(num_fwds*sizeof(int));
    if (!fwdprefixlen) {
        goto Bail;
    }
    fwdprefix = (char**) ccnl_calloc(num_fwds, sizeof(char*));
    if (!fwdprefix) {
        goto Bail;
    }
    for(it = 0; it < num_fwds; ++it) {
        fwdprefix[it] = (char*) ccnl_calloc(256, sizeof(char));
        if (!fwdprefix[it]) {
            goto Bail;
        }
    }

    //Alloc memory storage for interface answer
    num_interfaces = (size_t) get_num_interface(ccnl);
    interfaceaddr = (char**) ccnl_calloc(num_interfaces, sizeof(char*));
    if (!interfaceaddr) {
        goto Bail;
    }
    for (it = 0; it <num_interfaces; ++it) {
        interfaceaddr[it] = (char *) ccnl_malloc(130 * sizeof(char));
        if (!interfaceaddr[it]) {
            goto Bail;
        }
    }
    interfaceifndx = (int*) ccnl_malloc(num_interfaces*sizeof(int));
    if (!interfaceifndx) {
        goto Bail;
    }
    interfacedev = (long*) ccnl_malloc(num_interfaces*sizeof(long));
    if (!interfacedev) {
        goto Bail;
    }
    interfacedevtype = (int*) ccnl_malloc(num_interfaces*sizeof(int));
    if (!interfacedevtype) {
        goto Bail;
    }
    interfacereflect = (int*) ccnl_malloc(num_interfaces*sizeof(int));
    if (!interfacereflect) {
        goto Bail;
    }

    //Alloc memory storage for interest answer
    num_interests = (size_t) get_num_interests(ccnl);
    interest = (long*) ccnl_malloc(num_interests*sizeof(long));
    if (!interest) {
        goto Bail;
    }
    interestnext = (long*) ccnl_malloc(num_interests*sizeof(long));
    if (!interestnext) {
        goto Bail;
    }
    interestprev = (long*) ccnl_malloc(num_interests*sizeof(long));
    if (!interestprev) {
        goto Bail;
    }
    interestlast = (int*) ccnl_malloc(num_interests*sizeof(int));
    if (!interestlast) {
        goto Bail;
    }
    interestmin = (int*) ccnl_malloc(num_interests*sizeof(int));
    if (!interestmin) {
        goto Bail;
    }
    interestmax = (int*) ccnl_malloc(num_interests*sizeof(int));
    if (!interestmax) {
        goto Bail;
    }
    interestretries = (int*) ccnl_malloc(num_interests*sizeof(int));
    if (!interestretries) {
        goto Bail;
    }
    interestprefixlen = (int*) ccnl_malloc(num_interests*sizeof(int));
    if (!interestprefixlen) {
        goto Bail;
    }
    interestpublisher = (long*) ccnl_malloc(num_interests*sizeof(long));
    if (!interestpublisher) {
        goto Bail;
    }
    interestprefix = (char**) ccnl_calloc(num_interests, sizeof(char*));
    if (!interestprefix) {
        goto Bail;
    }
    for (it = 0; it < num_interests; ++it) {
        interestprefix[it] = (char *) ccnl_malloc(256 * sizeof(char));
        if (!interestprefix[it]) {
            goto Bail;
        }
    }

    //Alloc memory storage for content answer
    num_contents = (size_t) get_num_contents(ccnl);
    content = (long*)ccnl_malloc(num_contents*sizeof(long));
    if (!content) {
        goto Bail;
    }
    contentnext = (long*)ccnl_malloc(num_contents*sizeof(long));
    if (!contentnext) {
        goto Bail;
    }
    contentprev = (long*)ccnl_malloc(num_contents*sizeof(long));
    if (!contentprev) {
        goto Bail;
    }
    contentlast_use = (int*)ccnl_malloc(num_contents*sizeof(int));
    if (!contentlast_use) {
        goto Bail;
    }
    contentserved_cnt = (int*)ccnl_malloc(num_contents*sizeof(int));
    if (!contentserved_cnt) {
        goto Bail;
    }
    cprefixlen = (int*)ccnl_malloc(num_contents*sizeof(int));
    if (!cprefixlen) {
        goto Bail;
    }
    ccontents = (char**)ccnl_calloc(num_contents, sizeof(char*));
    if (!ccontents) {
        goto Bail;
    }
    cprefix = (char**) ccnl_calloc(num_contents, sizeof(char*));
    if (!cprefix) {
        goto Bail;
    }
    for (it = 0; it < num_contents; ++it) {
        ccontents[it] = (char*) ccnl_malloc(50*sizeof(char));
        if (!ccontents[it]) {
            goto Bail;
        }
        cprefix[it] = (char*) ccnl_malloc(256*sizeof(char));
        if (!cprefix[it]) {
            goto Bail;
        }
    }

    //End Alloc

    DEBUGMSG(TRACE, "ccnl_mgmt_debug from=%s\n", ccnl_addr2ascii(&from->peer));
    action = debugaction = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_BLOB) {
        goto SoftBail;
    }
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCNL_DTAG_DEBUGREQUEST) {
        goto SoftBail;
    }

    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        if (num == 0 && typ == 0) {
            break; // end
        }
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(debugaction, CCNL_DTAG_DEBUGACTION);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto SoftBail;
        }
    }

    // should (re)verify that action=="debug"

    if (debugaction) {
        cp = "debug cmd worked";
        DEBUGMSG(TRACE, "  debugaction is %s\n", debugaction);
        if (!strcmp((char*) debugaction, "dump")) {
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
        } else if (!strcmp((char*) debugaction, "halt")){
            ccnl->halt_flag = 1;
        } else if (!strcmp((char*) debugaction, "dump+halt")) {
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
        } else {
            cp = "unknown debug action, ignored";
        }
    } else {
        cp = "no debug action given, ignored";
    }

SoftBail:
    /*ANSWER*/
    if (!debugaction) {
        debugaction = (unsigned char *) "Error for debug cmd";
    }
    stmt_length = 200 * num_faces + 200 * num_interfaces + 200 * num_fwds //alloc stroage for answer dynamically.
            + 200 * num_interests + 200 * num_contents;
    contentobject_length = stmt_length + 1000;
    object_length = contentobject_length + 1000;

    out = ccnl_malloc(object_length);
    if (!out) {
        goto Bail;
    }
    contentobj = ccnl_malloc(contentobject_length);
    if (!contentobj) {
        goto Bail;
    }
    stmt = ccnl_malloc(stmt_length);
    if (!stmt) {
        goto Bail;
    }

    if (ccnl_ccnb_mkHeader(out+len, out + object_length, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out+len, out + object_length, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out+len, out + object_length, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out+len, out + object_length, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "debug", &len)) {
        goto Bail;
    }
    if (len + 1 >= object_length) {
        goto Bail;
    }
    out[len++] = 0;

    // prepare debug statement
    if (ccnl_ccnb_mkHeader(stmt, stmt + stmt_length, CCNL_DTAG_DEBUGREQUEST, CCN_TT_DTAG, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(stmt+len3, stmt + stmt_length, CCN_DTAG_ACTION, CCN_TT_DTAG,
            (char*) debugaction, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(stmt+len3, stmt + stmt_length, CCNL_DTAG_DEBUGACTION, CCN_TT_DTAG, cp, &len3)) {
        goto Bail;
    }
    if (len3 + 1 >= stmt_length) {
        goto Bail;
    }
    stmt[len3++] = 0; //end-of-debugstmt

    if (!strcmp((char*) debugaction, "dump") || !strcmp((char*) debugaction, "dump+halt")) {  //halt returns no content
        if (ccnl_ccnb_mkHeader(stmt+len3, stmt+stmt_length, CCNL_DTAG_DEBUGREPLY, CCN_TT_DTAG, &len3)) {
            goto Bail;
        }
        //len3 += ccnl_ccnb_mkStrBlob(stmt+len3, CCNL_DTAG_PREFIX, CCN_TT_DTAG, cinterfaces[it]);

        if (ccnl_mgmt_create_interface_stmt(num_interfaces, interfaceifndx, interfacedev,
                interfacedevtype, interfacereflect, interfaceaddr, stmt, stmt+stmt_length, &len3)) {
            goto Bail;
        }

        if (ccnl_mgmt_create_faces_stmt(num_faces, faceid, facenext, faceprev, faceifndx,
                        faceflags, facetype, facepeer, facefrag, stmt, stmt+stmt_length, &len3)) {
            goto Bail;
        }

        if (ccnl_mgmt_create_fwds_stmt(num_fwds, fwd, fwdnext, fwdface, fwdfaceid, suite,
                fwdprefixlen, fwdprefix, stmt, stmt+stmt_length, &len3)) {
            goto Bail;
        }

        if (ccnl_mgmt_create_interest_stmt(num_interests, interest, interestnext, interestprev,
                interestlast, interestmin, interestmax, interestretries,
                interestpublisher, interestprefixlen, interestprefix, stmt, stmt+stmt_length, &len3)) {
            goto Bail;
        }

        if (ccnl_mgmt_create_content_stmt(num_contents, content, contentnext, contentprev,
                contentlast_use, contentserved_cnt, ccontents, cprefix, stmt, stmt+stmt_length, &len3)) {
            goto Bail;
        }
    }

    if (len3 + 1 >= stmt_length) {
        goto Bail;
    }
    stmt[len3++] = 0; //end of debug reply

    if (ccnl_ccnb_mkBlob(out+len, out+object_length, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                         (char*) stmt, len3, &len)) {
        goto Bail;
    }

    if (ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, out)) {
        goto Bail;
    }

    /*END ANWER*/

    rc = 0;

Bail:
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
    if (facepeer) {
        for (it = 0; it < num_faces; ++it) {
            ccnl_free(facepeer[it]);
        }
    }
    ccnl_free(facepeer);
    if (facefrag) {
        for (it = 0; it < num_faces; ++it) {
            ccnl_free(facefrag[it]);
        }
    }
    ccnl_free(facefrag);
    if (interfaceaddr) {
        for (it = 0; it < num_interfaces; ++it) {
            ccnl_free(interfaceaddr[it]);
        }
    }
    ccnl_free(interfaceaddr);
    if (interestprefix) {
        for (it = 0; it < num_interests; ++it) {
            ccnl_free(interestprefix[it]);
        }
    }
    ccnl_free(interestprefix);
    if (ccontents) {
        for (it = 0; it < num_contents; ++it) {
            ccnl_free(ccontents[it]);
        }
    }
    ccnl_free(ccontents);
    if (cprefix) {
        for (it = 0; it < num_contents; ++it) {
            ccnl_free(cprefix[it]);
        }
    }
    ccnl_free(cprefix);
    if (fwdprefix) {
        for (it = 0; it < num_fwds; ++it) {
            ccnl_free(fwdprefix[it]);
        }
    }
    ccnl_free(fwdprefix);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

int8_t
ccnl_mgmt_newface(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    uint8_t *buf;
    size_t buflen;
    uint64_t num;
    uint8_t typ;
    uint8_t *action, *macsrc, *ip4src, *ip6src, *proto, *host, *port, *wpanaddr,
        *wpanpanid, *path, *frag, *flags;
    char *cp = "newface cmd failed";
    int ret;
    int8_t rc = -1;
    struct ccnl_face_s *f = NULL;
    //varibales for answer
    size_t len = 0, len3 = 0;
    //    unsigned char contentobj[2000];
    //    unsigned char faceinst[2000];
    unsigned char faceidstr[100];
    unsigned char retstr[200];

    DEBUGMSG(TRACE, "ccnl_mgmt_newface from=%p, ifndx=%d\n",
             (void*) from, from->ifndx);
    action = macsrc = ip4src = ip6src = proto = host = port = NULL;
    path = frag = flags = wpanaddr = wpanpanid = NULL;
    

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_BLOB) {
        goto SoftBail;
    }
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FACEINSTANCE) {
        goto SoftBail;
    }

    while (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ) == 0) {
        if (num == 0 && typ == 0) {
            break; // end
        }
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(macsrc, CCNL_DTAG_MACSRC);
        extractStr(ip4src, CCNL_DTAG_IP4SRC);
        extractStr(ip6src, CCNL_DTAG_IP6SRC);
        extractStr(path, CCNL_DTAG_UNIXSRC);
        extractStr(proto, CCN_DTAG_IPPROTO);
        extractStr(host, CCN_DTAG_HOST);
        extractStr(port, CCN_DTAG_PORT);
        extractStr(wpanaddr, CCNL_DTAG_WPANADR);
        extractStr(wpanpanid, CCNL_DTAG_WPANPANID);
        extractStr(flags, CCNL_DTAG_FACEFLAGS);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto SoftBail;
        }
    }

    // should (re)verify that action=="newface"

#ifdef USE_LINKLAYER
#if !(defined(__FreeBSD__) || defined(__APPLE__))
    if (macsrc && host && port) {
        sockunion su;
        unsigned long lport;
        DEBUGMSG(TRACE, "  adding ETH face macsrc=%s, host=%s, ethtype=%s\n",
                 macsrc, host, port);
        memset(&su, 0, sizeof(su));
        errno = 0;
        lport = strtoul((const char*) port, NULL, 0);
        if (errno != 0 || lport > UINT16_MAX) {
            goto SoftBail;
        }
        su.linklayer.sll_family = AF_PACKET;
        su.linklayer.sll_protocol = htons((uint16_t) lport);
        if (ccnl_mgmt_parse_eth_address(su.linklayer.sll_addr, (const char*) host)) {
            goto SoftBail;
        }
        // if (!strcmp(macsrc, "any")) // honouring macsrc not implemented yet
        f = ccnl_get_face_or_create(ccnl, -1, &su.sa, sizeof(su.linklayer));
    } else
#endif
#endif
    if ( (proto && host && port && !strcmp("17", (const char*) proto)) ||
                    (wpanaddr && wpanpanid) ) {
        sockunion su;
        unsigned long lport;
        errno = 0;
        lport = strtoul((const char*) port, NULL, 0);
        if (errno != 0 || lport > UINT16_MAX) {
            goto SoftBail;
        }
#ifdef USE_IPV4
        if (ip4src != NULL) {
            DEBUGMSG(TRACE, "  adding IP face ip4src=%s, proto=%s, host=%s, port=%s\n",
                    ip4src, proto, host, port);
            su.sa.sa_family = AF_INET;
    #if defined(__linux__) && !defined(CCNL_LINUXKERNEL)
            inet_pton(AF_INET, (const char*)host, &su.ip4.sin_addr);
    #else
            inet_aton((const char*)host, &su.ip4.sin_addr);
    #endif
            su.ip4.sin_port = htons((uint16_t) lport);
            // not implmented yet: honor the requested ip4src parameter
            f = ccnl_get_face_or_create(ccnl, -1, // from->ifndx,
                                        &su.sa, sizeof(struct sockaddr_in));
        }
#endif
#ifdef USE_IPV6
#ifndef CCNL_LINUXKERNEL
        if (ip6src != NULL) {
            DEBUGMSG(TRACE, "  adding IP face ip6src=%s, proto=%s, host=%s, port=%s\n",
                    ip6src, proto, host, port);
            su.sa.sa_family = AF_INET6;
            inet_pton(AF_INET6, (const char*)host, &su.ip6.sin6_addr.s6_addr);
            su.ip6.sin6_port = htons((uint16_t) lport);
            f = ccnl_get_face_or_create(ccnl, -1, // from->ifndx,
                                        &su.sa, sizeof(struct sockaddr_in6));
        }
#endif //CCNL_LINUXKERNEL
#endif
#ifdef USE_WPAN
        if (wpanaddr && wpanpanid) {
            /* initialize address with 0xFF for broadcast */
            DEBUGMSG(TRACE, "  adding WPAN face ADDR=%s PANID=%s\n", wpanaddr, wpanpanid);
            su.sa.sa_family = AF_IEEE802154;
            su.wpan.addr.addr_type = IEEE802154_ADDR_SHORT;
            su.wpan.addr.pan_id = strtol((const char*)wpanpanid, NULL, 0);
            su.wpan.addr.addr.short_addr = strtol((const char*)wpanaddr, NULL, 0);
            f = ccnl_get_face_or_create(ccnl, -1, &su.sa, sizeof(su.wpan));
        }
#endif
    }
#ifdef USE_UNIXSOCKET
    if (path) {
        sockunion su;
        DEBUGMSG(TRACE, "  adding UNIX face unixsrc=%s\n", path);
        su.sa.sa_family = AF_UNIX;
        strncpy(su.ux.sun_path, (char*) path, sizeof(su.ux.sun_path));
        if (su.ux.sun_path[sizeof(su.ux.sun_path) - 1] != 0) {
            goto SoftBail;
        }
        f = ccnl_get_face_or_create(ccnl, -1, // from->ifndx,
                                    &su.sa, sizeof(struct sockaddr_un));
    }
#endif


    if (f) {
        long lflags;
        errno = 0;
        lflags = strtol((const char*) flags, NULL, 0);
        if (errno != 0 || lflags > INT16_MAX || lflags < INT16_MIN) {
            goto SoftBail;
        }
        int flagval = flags ? (int) lflags : CCNL_FACE_FLAGS_STATIC;
        //      printf("  flags=%s %d\n", flags, flagval);
        DEBUGMSG(TRACE, "  adding a new face (id=%d) worked!\n", f->faceid);
        f->flags = flagval &
            (CCNL_FACE_FLAGS_STATIC|CCNL_FACE_FLAGS_REFLECT);

#ifdef USE_FRAG
        if (frag) {
            int mtu = 1500;
            long lfrag;
            if (f->frag) {
                ccnl_frag_destroy(f->frag);
                f->frag = NULL;
            }
            if (f->ifndx >= 0 && ccnl->ifs[f->ifndx].mtu > 0) {
                mtu = ccnl->ifs[f->ifndx].mtu;
            }
            errno = 0;
            lfrag = strtol((const char*) frag, NULL, 0);
            if (errno != 0 || lflags > INT16_MAX || lflags < INT16_MIN) {
                goto SoftBail;
            }
            f->frag = ccnl_frag_new((int) lfrag, mtu);
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

SoftBail:
    /*ANSWER*/

    if (ccnl_ccnb_mkHeader(out_buf, out_buf + OUT_BUF_SIZE, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newface", &len)) {
        goto Bail;
    }
    if (len + 1 >= OUT_BUF_SIZE) {
        goto Bail;
    }
    out_buf[len++] = 0; // end-of-name

    // prepare FACEINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst_buf, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG, &len3)) {
        goto Bail;
    }
    ret = snprintf((char *) retstr, sizeof(retstr), "newface:  %s", cp);
    if (ret < 0 || (unsigned) ret >= sizeof(retstr)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_ACTION, CCN_TT_DTAG, (char*) retstr, &len3)) {
        goto Bail;
    }
    if (macsrc) {
        if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_MACSRC, CCN_TT_DTAG, (char *) macsrc, &len3)) {
            goto Bail;
        }
    }
    if (ip4src) {
        if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_IP4SRC, CCN_TT_DTAG, (char*) ip4src, &len3)) {
            goto Bail;
        }
        if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_IPPROTO, CCN_TT_DTAG, "17", &len3)) {
            goto Bail;
        }
    }
    if (ip6src) {
        if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_IP6SRC, CCN_TT_DTAG, (char*) ip6src, &len3)) {
            goto Bail;
        }
        if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_IPPROTO, CCN_TT_DTAG, "17", &len3)) {
            goto Bail;
        }
    }
    if (host) {
        if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_HOST, CCN_TT_DTAG, (char *) host, &len3)) {
            goto Bail;
        }
    }
    if (port) {
        if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_PORT, CCN_TT_DTAG, (char *) port, &len3)) {
            goto Bail;
        }
    }
    /*
    if (frag) {
        if (ccnl_ccnb_mkStrBlob(faceinst+len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_FRAG, CCN_TT_DTAG, frag, &len3)) {
            goto Bail;
        }
    }
    */
    if (flags) {
        if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_FACEFLAGS, CCN_TT_DTAG, (char *) flags, &len3)) {
            goto Bail;
        }
    }
    if (f) {
        snprintf((char *)faceidstr, sizeof(faceidstr), "%i",f->faceid);
        if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_FACEID, CCN_TT_DTAG, (char *) faceidstr, &len3)) {
            goto Bail;
        }
    }

    faceinst_buf[len3++] = 0; // end-of-faceinst

    if (ccnl_ccnb_mkBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst_buf, len3, &len)) {
        goto Bail;
    }

    if (ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf)) {
        goto Bail;
    }

    /*END ANWER*/

    rc = 0;

Bail:
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

int8_t
ccnl_mgmt_setfrag(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    uint8_t *buf;
    size_t buflen;
    uint64_t num;
    uint8_t typ;
    uint8_t *action, *faceid, *frag, *mtu;
    char *cp = "setfrag cmd failed";
    int8_t rc = -1;
    struct ccnl_face_s *f;
    size_t len = 0, len3 = 0;

    DEBUGMSG(TRACE, "ccnl_mgmt_setfrag from=%p, ifndx=%d\n",
             (void*) from, from->ifndx);
    action = faceid = frag = mtu = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_BLOB) {
        goto SoftBail;
    }
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FACEINSTANCE) {
        goto SoftBail;
    }

    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        if (num==0 && typ==0) {
            break; // end
        }
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(faceid, CCN_DTAG_FACEID);
        extractStr(frag, CCNL_DTAG_FRAG);
        extractStr(mtu, CCNL_DTAG_MTU);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto SoftBail;
        }
    }

    // should (re)verify that action=="newface"

    if (faceid && frag && mtu) {
#ifdef USE_FRAG
        int e = -1;
#endif
        long fi = strtol((const char*)faceid, NULL, 0);
        long lmtu = 0;
        (void) lmtu;

        for (f = ccnl->faces; f && f->faceid != fi; f = f->next);
        if (!f) {
            goto Error;
        }

#ifdef USE_FRAG
        if (f->frag) {
            ccnl_frag_destroy(f->frag);
            f->frag = 0;
        }
        if (!strcmp((const char*)frag, "none")) {
            e = CCNL_FRAG_NONE;
        } else if (!strcmp((const char*)frag, "seqd2012")) {
            e = CCNL_FRAG_SEQUENCED2012;
        } else if (!strcmp((const char*)frag, "ccnx2013")) {
            e = CCNL_FRAG_CCNx2013;
        } else if (!strcmp((const char*)frag, "seqd2015")) {
            e = CCNL_FRAG_SEQUENCED2015;
        }
        if (e < 0) {
            goto Error;
        }
        errno = 0;
        lmtu = strtol((const char*) mtu, NULL, 0);
        if (errno != 0 || lmtu < 0 || lmtu >= UINT16_MAX) {
            goto SoftBail;
        }
        f->frag = ccnl_frag_new(e, (int) lmtu);
        cp = "setfrag cmd worked";
#else
        cp = "no fragmentation support";
#endif
    } else {
Error:
        DEBUGMSG(TRACE, "  setfrag request for (faceid=%s frag=%s mtu=%s) failed or was ignored\n",
                 faceid, frag, mtu);
    }

SoftBail:

    if (ccnl_ccnb_mkHeader(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "setfrag", &len)) {
        goto Bail;
    }
    if (len + 1 >= OUT_BUF_SIZE) {
        goto Bail;
    }
    out_buf[len++] = 0; // end-of-name

    // prepare FACEINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst_buf, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_ACTION, CCN_TT_DTAG, cp, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_FRAG, CCN_TT_DTAG, (char*) frag, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_MTU, CCN_TT_DTAG, (char*) mtu, &len3)) {
        goto Bail;
    }
    if (len3 + 1 >= FACEINST_BUF_SIZE) {
        goto Bail;
    }
    faceinst_buf[len3++] = 0; // end-of-faceinst

    if (ccnl_ccnb_mkBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst_buf, len3, &len)) {
        goto Bail;
    }

    if (ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf)) {
        goto Bail;
    }

    rc = 0;

Bail:

    ccnl_free(action);
    ccnl_free(faceid);
    ccnl_free(frag);
    ccnl_free(mtu);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

int8_t
ccnl_mgmt_destroyface(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                      struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    uint8_t *buf;
    size_t buflen;
    uint64_t num;
    uint8_t typ;
    uint8_t *action, *faceid;
    char *cp = "destroyface cmd failed";
    int8_t rc = -1;

    size_t len = 0, len3 = 0;
//    unsigned char contentobj[2000];
//    unsigned char faceinst[2000];

    DEBUGMSG(DEBUG, "ccnl_mgmt_destroyface\n");
    action = faceid = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_BLOB) {
        goto SoftBail;
    }
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FACEINSTANCE) {
        goto SoftBail;
    }

    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        if (num == 0 && typ == 0) {
            break; // end
        }
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(faceid, CCN_DTAG_FACEID);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto SoftBail;
        }
    }

    // should (re)verify that action=="destroyface"

    if (faceid) {
        struct ccnl_face_s *f;
        long lfi;
        int fi;
        errno = 0;
        lfi = strtol((const char*)faceid, NULL, 0);
        if (errno != 0 || lfi < 0 || lfi > INT16_MAX) {
            goto SoftBail;
        }
        fi = (int) lfi;
        for (f = ccnl->faces; f && f->faceid != fi; f = f->next);
        if (!f) {
            DEBUGMSG(TRACE, "  could not find face=%s\n", faceid);
            goto SoftBail;
        }
        ccnl_face_remove(ccnl, f);
        DEBUGMSG(TRACE, "  face %s destroyed\n", faceid);
        cp = "facedestroy cmd worked";
    } else {
        DEBUGMSG(TRACE, "  missing faceid\n");
    }

SoftBail:
    /*ANSWER*/
    if (!faceid) {
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "destroyface", cp);
        goto Bail;
    }

    if (ccnl_ccnb_mkHeader(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "destroyface", &len)) {
        goto Bail;
    }
    if (len + 1 >= OUT_BUF_SIZE) {
        goto Bail;
    }
    out_buf[len++] = 0; // end-of-name

    // prepare FACEINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst_buf, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_FACEINSTANCE, CCN_TT_DTAG, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_ACTION, CCN_TT_DTAG, cp, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid, &len3)) {
        goto Bail;
    }
    if (len3 + 1 >= FACEINST_BUF_SIZE) {
        goto Bail;
    }
    faceinst_buf[len3++] = 0; // end-of-faceinst

    if (ccnl_ccnb_mkBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                         (char*) faceinst_buf, len3, &len)) {
        goto Bail;
    }

    if (ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf)) {
        goto Bail;
    }

    rc = 0;

Bail:

    /*END ANWER*/
    ccnl_free(action);
    ccnl_free(faceid);
    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

int8_t
ccnl_mgmt_newdev(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                 struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    uint8_t *buf;
    size_t buflen;
    uint64_t num;
    uint8_t typ;
    uint8_t *action, *devname, *ip4src, *ip6src, *port, *frag, *flags;
    char *cp = "newdevice cmd worked";
    int8_t rc = -1;

    //variables for answer
    size_t len = 0, len3 = 0;
//    unsigned char contentobj[2000];
//    unsigned char faceinst[2000];
    struct ccnl_if_s *i = NULL;


    DEBUGMSG(TRACE, "ccnl_mgmt_newdev\n");
    action = devname = ip4src = ip6src = port = frag = flags = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_BLOB) {
        goto SoftBail;
    }
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCNL_DTAG_DEVINSTANCE) {
        goto SoftBail;
    }

    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        if (num == 0 && typ == 0) {
            break; // end
        }
        extractStr(action, CCN_DTAG_ACTION);
        extractStr(devname, CCNL_DTAG_DEVNAME);
        extractStr(ip4src, CCNL_DTAG_IP4SRC);
        extractStr(ip6src, CCNL_DTAG_IP6SRC);
        extractStr(port, CCN_DTAG_PORT);
        extractStr(frag, CCNL_DTAG_FRAG);
        extractStr(flags, CCNL_DTAG_DEVFLAGS);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto SoftBail;
        }
    }

    // should (re)verify that action=="newdev"

    if (ccnl->ifcount >= CCNL_MAX_INTERFACES) {
      DEBUGMSG(TRACE, "  too many interfaces, no new interface created\n");
      goto SoftBail;
    }

#if defined(USE_LINKLAYER) && (defined(CCNL_UNIX) || defined(CCNL_LINUXKERNEL))
    if (devname && port) {
        int portnum = CCNL_ETH_TYPE;
        unsigned long lport;

        cp = "newETHdev cmd worked";
        if (port) {
            errno = 0;
            lport = strtoul((const char *) port, NULL, 0);
            if (errno != 0 || lport > UINT16_MAX) {
                goto SoftBail;
            }
            portnum = (int) lport;
        }

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
                goto SoftBail;
            }
            for (j = 0; j < ccnl->ifcount; j++) {
                if (ccnl->ifs[j].netdev == nd) {
                    dev_put(nd);
                    DEBUGMSG(TRACE, "  device %s already open\n", devname);
                    goto SoftBail;
                }
            }
            i->netdev = nd;
            i->ccnl_packet.type = htons(portnum);
            i->ccnl_packet.dev = i->netdev;
            i->ccnl_packet.func = ccnl_eth_RX;
            dev_add_pack(&i->ccnl_packet);
        }
#elif defined(USE_LINKLAYER)
#if !(defined(__FreeBSD__) || defined(__APPLE__))
        i->sock = ccnl_open_ethdev((char*)devname, &i->addr.linklayer, portnum);
        if (!i->sock) {
            DEBUGMSG(TRACE, "  could not open device %s\n", devname);
            goto SoftBail;
        }
#endif
#endif
//      i->frag = frag ? atoi(frag) : 0;
        i->mtu = 1500;
//      we should analyse and copy flags, here we hardcode some defaults:
        i->reflect = 1;
        i->fwdalli = 1;

        if (ccnl->defaultInterfaceScheduler) {
            i->sched = ccnl->defaultInterfaceScheduler(ccnl, ccnl_interface_CTS);
        }
        ccnl->ifcount++;

        goto SoftBail;
    }
#endif

    if ((ip4src || ip6src) && port) {
#ifdef USE_IPV4
        if (ip4src) {
            unsigned long lport;
            cp = "newUDPdev cmd worked";
            DEBUGMSG(TRACE, "  adding UDP device ip4src=%s, port=%s\n",
                     ip4src, port);

            errno = 0;
            lport = strtoul((char*)port, NULL, 0);
            if (errno || lport > UINT16_MAX) {
                goto SoftBail;
            }
            // check if it already exists, bail

            // create a new ifs-entry
            i = &ccnl->ifs[ccnl->ifcount];
            i->sock = ccnl_open_udpdev((uint16_t) lport, &i->addr.ip4);
            if (!i->sock) {
                DEBUGMSG(TRACE, "  could not open UDP device %s/%s\n", ip4src, port);
                goto SoftBail;
            }
        }
#endif
#ifdef USE_IPV6
#ifndef CCNL_ANDROID
        if (ip6src) {
            unsigned long port_l;
            errno = 0;
            port_l = strtoul((char*)port, NULL, 0);
            if (errno || port_l > UINT16_MAX) {
                DEBUGMSG(TRACE, "  could not parse UDP port: %s\n", port);
                goto SoftBail;
            }
            cp = "newUDPdev cmd worked";
            DEBUGMSG(TRACE, "  adding UDP device ip6src=%s, port=%s\n",
                     ip6src, port);

            // check if it already exists, bail

            // create a new ifs-entry
            i = &ccnl->ifs[ccnl->ifcount];
            i->sock = ccnl_open_udp6dev((uint16_t) port_l, &i->addr.ip6);
            if (!i->sock) {
                DEBUGMSG(TRACE, "  could not open UDP device %s/%s\n", ip6src, port);
                goto SoftBail;
            }
        }
#endif //CCNL_ANDROID
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
                    goto SoftBail;
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
            goto SoftBail;
        }
        write_lock_bh(&i->sock->sk->sk_callback_lock);
        i->old_data_ready = i->sock->sk->sk_data_ready;
        i->sock->sk->sk_data_ready = ccnl_udp_data_ready;
//      i->sock->sk->sk_user_data = &theRelay;
        write_unlock_bh(&i->sock->sk->sk_callback_lock);
#endif

        if (!i) {
            goto SoftBail;
        }
//      i->frag = frag ? atoi(frag) : 0;
        i->mtu = CCN_DEFAULT_MTU;
//      we should analyse and copy flags, here we hardcode some defaults:
        i->reflect = 0;
        i->fwdalli = 1;

        if (ccnl->defaultInterfaceScheduler) {
            i->sched = ccnl->defaultInterfaceScheduler(ccnl, ccnl_interface_CTS);
        }
        ccnl->ifcount++;

        //cp = "newdevice cmd worked";
        goto SoftBail;
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

SoftBail:

    if (ccnl_ccnb_mkHeader(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "newdev", &len)) {
        goto Bail;
    }
    if (len + 1 >= OUT_BUF_SIZE) {
        goto Bail;
    }
    out_buf[len++] = 0; // end-of-name

    // prepare DEVINSTANCE
    if (ccnl_ccnb_mkHeader(faceinst_buf, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_DEVINSTANCE, CCN_TT_DTAG, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(faceinst_buf+len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_ACTION, CCN_TT_DTAG, cp, &len3)) {
        goto Bail;
    }
    if (devname) {
        if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_DEVNAME,
                                    CCN_TT_DTAG, (char *) devname, &len3)) {
            goto Bail;
        }
    }

    if (devname && port) {
        if (port) {
            if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_PORT, CCN_TT_DTAG, (char *) port, &len3)) {
                goto Bail;
            }
        }
        if (frag) {
            if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_FRAG, CCN_TT_DTAG, (char *) frag, &len3)) {
                goto Bail;
            }
        }
        if (flags) {
            if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG, (char *) flags, &len3)) {
                goto Bail;
            }
        }
        if (len3 + 1 >= FACEINST_BUF_SIZE) {
            goto Bail;
        }
        faceinst_buf[len3++] = 0; // end-of-faceinst
    }
    else if ((ip4src && port) || (ip6src && port)) {
        if (ip4src) {
            if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_IP4SRC, CCN_TT_DTAG, (char *) ip4src, &len3)) {
                goto Bail;
            }
        }
        if (ip6src) {
            if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_IP6SRC, CCN_TT_DTAG, (char *) ip6src, &len3)) {
                goto Bail;
            }
        }
        if (port) {
            if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCN_DTAG_PORT, CCN_TT_DTAG, (char *) port, &len3)) {
                goto Bail;
            }
        }
        if (frag) {
            if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_FRAG, CCN_TT_DTAG, (char *) frag, &len3)) {
                goto Bail;
            }
        }
        if (flags) {
            if (ccnl_ccnb_mkStrBlob(faceinst_buf + len3, faceinst_buf + FACEINST_BUF_SIZE, CCNL_DTAG_DEVFLAGS, CCN_TT_DTAG, (char *) flags, &len3)) {
                goto Bail;
            }
        }
        if (len3 + 1 >= FACEINST_BUF_SIZE) {
            goto Bail;
        }
        faceinst_buf[len3++] = 0; // end-of-faceinst
    }

    if (ccnl_ccnb_mkBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) faceinst_buf, len3, &len)) {
        goto Bail;
    }

    if (ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf)) {
        goto Bail;
    }

    rc = 0;

Bail:

    ccnl_free(devname);
    ccnl_free(port);
    ccnl_free(frag);
    ccnl_free(action);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}


int8_t
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

int8_t
ccnl_mgmt_echo(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
               struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    uint8_t *buf;
    size_t buflen;
    uint64_t num;
    uint8_t typ;
    struct ccnl_prefix_s *p = NULL;
    uint8_t *action, *suite = NULL, h[12];
    char *cp = "echoserver cmd failed";
    int8_t rc = -1;
    char s[CCNL_MAX_PREFIX_SIZE];

    size_t len = 0, len3 = 0;

    DEBUGMSG(TRACE, "ccnl_mgmt_echo\n");
    action = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_BLOB) {
        goto SoftBail;
    }
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FWDINGENTRY) {
        goto SoftBail;
    }

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p) {
        goto SoftBail;
    }
    p->comp = (unsigned char**) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(unsigned char*));
    p->complen = (size_t *) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(size_t));
    if (!p->comp || !p->complen) {
        goto SoftBail;
    }

    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        if (num == 0 && typ == 0) {
            break; // end
        }

        if (typ == CCN_TT_DTAG && num == CCN_DTAG_NAME) {
            for (;;) {
                if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
                    goto SoftBail;
                }
                if (num == 0 && typ == 0) {
                    break;
                }
                if (typ == CCN_TT_DTAG && num == CCN_DTAG_COMPONENT &&
                    p->compcnt < CCNL_MAX_NAME_COMP) {
                        // if (ccnl_grow_prefix(p)) {
                        //     goto SoftBail;
                        // }
                    if (ccnl_ccnb_consume(typ, num, &buf, &buflen,
                                p->comp + p->compcnt,
                                p->complen + p->compcnt)) {
                        goto SoftBail;
                    }
                    p->compcnt++;
                } else {
                    if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
                        goto SoftBail;
                    }
                }
            }
            continue;
        }

        extractStr(action, CCN_DTAG_ACTION);
        extractStr(suite, CCNL_DTAG_SUITE);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto SoftBail;
        }
    }

    // should (re)verify that action=="prefixreg"
    if (suite && *suite >= 0 && *suite < CCNL_SUITE_LAST && p->compcnt > 0) {
        p->suite = *suite;
        DEBUGMSG(TRACE, "mgmt: activating echo server for %s, suite=%s\n",
                 ccnl_prefix_to_str(p,s,CCNL_MAX_PREFIX_SIZE), ccnl_suite2str(*suite));
        ccnl_echo_add(ccnl, ccnl_prefix_clone(p));
        cp = "echoserver cmd worked";
    } else {
        DEBUGMSG(TRACE, "mgmt: ignored echoserver\n");
    }

SoftBail:
    /*ANSWER*/
    if (!action || !p) {
        if (ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "echoserver", cp)) {
            goto Bail;
        }
    }
    if (ccnl_ccnb_mkHeader(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) {  // name
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, (char*) action, &len)) {
        goto Bail;
    }
    if (len + 1 >= OUT_BUF_SIZE) {
        goto Bail;
    }
    out_buf[len++] = 0; // end-of-name

    // prepare FWDENTRY
    if (ccnl_ccnb_mkHeader(fwdentry_buf, fwdentry_buf + FWDENTRY_BUF_SIZE, CCNL_DTAG_PREFIX, CCN_TT_DTAG, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, fwdentry_buf + FWDENTRY_BUF_SIZE, CCN_DTAG_ACTION, CCN_TT_DTAG, cp, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, fwdentry_buf + FWDENTRY_BUF_SIZE, CCN_DTAG_NAME, CCN_TT_DTAG, ccnl_prefix_to_str(p,s,CCNL_MAX_PREFIX_SIZE, &len3))) { // prefix
        goto Bail;
    }

    //    len3 += ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid);
    memset(h,0,sizeof(h));
    snprintf((char*)h, sizeof(h), "%d", (int)suite[0]);
    if (ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, fwdentry_buf + FWDENTRY_BUF_SIZE, CCNL_DTAG_SUITE, CCN_TT_DTAG, (char*) h, &len3)) {
        goto Bail;
    }
    if (len3 + 1 >= FWDENTRY_BUF_SIZE) {
        goto Bail;
    }
    fwdentry_buf[len3++] = 0; // end-of-fwdentry

    if (ccnl_ccnb_mkBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) fwdentry_buf, len3, &len)) {
        goto Bail;
    }

    if (ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf)) {
        goto Bail;
    }

    /*END ANWER*/

    rc = -1;

Bail:

    ccnl_free(suite);
    ccnl_free(action);
    ccnl_prefix_free(p);

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

#endif // USE_ECHO

int8_t
ccnl_mgmt_prefixreg(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    uint8_t *buf;
    size_t buflen;
    uint64_t num;
    uint8_t typ;
    struct ccnl_prefix_s *p = NULL;
    uint8_t *action, *faceid, *suite=0, h[12];
    char *cp = "prefixreg cmd failed";
    int8_t rc = -1;
    char s[CCNL_MAX_PREFIX_SIZE];
    struct ccnl_forward_s *fwd = NULL;

    size_t len = 0, len3 = 0;

    DEBUGMSG(TRACE, "ccnl_mgmt_prefixreg\n");
    action = faceid = NULL;

    buf = prefix->comp[3];
    buflen = prefix->complen[3];
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) {
        goto SoftBail;
    }
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_BLOB) {
        goto SoftBail;
    }
    buflen = num;
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_FWDINGENTRY) {
        goto SoftBail;
    }

    p = (struct ccnl_prefix_s *) ccnl_calloc(1, sizeof(struct ccnl_prefix_s));
    if (!p) {
        goto Bail;
    }
    p->comp = (uint8_t**) ccnl_calloc(CCNL_MAX_NAME_COMP, sizeof(uint8_t*));
    p->complen = (size_t*) ccnl_malloc(CCNL_MAX_NAME_COMP * sizeof(size_t));
    if (!p->comp || !p->complen) {
        goto Bail;
    }

    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        if (num == 0 && typ == 0) {
            break; // end
        }

        if (typ == CCN_TT_DTAG && num == CCN_DTAG_NAME) {
            for (;;) {
                if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
                    goto SoftBail;
                }
                if (num == 0 && typ == 0) {
                    break;
                }
                if (typ == CCN_TT_DTAG && num == CCN_DTAG_COMPONENT &&
                    p->compcnt < CCNL_MAX_NAME_COMP) {
                        // if (ccnl_grow_prefix(p)) goto SoftBail;
                    if (ccnl_ccnb_consume(typ, num, &buf, &buflen,
                                p->comp + p->compcnt, p->complen + p->compcnt)) {
                        goto SoftBail;
                    }
                    p->compcnt++;
                } else {
                    if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
                        goto SoftBail;
                    }
                }
            }
            continue;
        }

        extractStr(action, CCN_DTAG_ACTION);
        extractStr(faceid, CCN_DTAG_FACEID);
        extractStr(suite, CCNL_DTAG_SUITE);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto SoftBail;
        }
    }

    // should (re)verify that action=="prefixreg"
    if (faceid && p->compcnt > 0) {
        struct ccnl_face_s *f = NULL;
        struct ccnl_forward_s **fwd2 = NULL;
        long faceid_l;

        errno = 0;
        faceid_l = strtol((const char*)faceid, NULL, 0);
        if (errno || faceid_l < INT_MIN || faceid_l > INT_MAX) {
            DEBUGMSG(WARNING, "mgmt: could not parse faceid: %s\n", faceid);
            goto SoftBail;
        }
        int fi = (int) faceid_l;

        p->suite = suite[0];

        DEBUGMSG(TRACE, "mgmt: adding prefix %s to faceid=%s, suite=%s\n",
                 ccnl_prefix_to_str(p,s,CCNL_MAX_PREFIX_SIZE), faceid, ccnl_suite2str(suite[0]));

        for (f = ccnl->faces; f && f->faceid != fi; f = f->next);
        if (!f) {
            goto SoftBail;
        }

//      printf("Face %s found\n", faceid);
        fwd = (struct ccnl_forward_s *) ccnl_calloc(1, sizeof(*fwd));
        if (!fwd) {
            goto SoftBail;
        }
        fwd->prefix = ccnl_prefix_clone(p);
        fwd->face = f;
        if (suite) {
            fwd->suite = suite[0];
        }

        fwd2 = &ccnl->fib;
        while (*fwd2) {
            fwd2 = &((*fwd2)->next);
        }
        *fwd2 = fwd;
        cp = "prefixreg cmd worked";
    } else {
        DEBUGMSG(TRACE, "mgmt: ignored prefixreg faceid=%s\n", faceid);
    }

SoftBail:
    /*ANSWER*/
    if (!action || !p || ! faceid) {
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "prefixreg", cp);
        goto Bail;
    }
    if (ccnl_ccnb_mkHeader(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_NAME, CCN_TT_DTAG, &len)) { // name
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "", &len)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_COMPONENT, CCN_TT_DTAG, (char*) action, &len)) {
        goto Bail;
    }
    if (len + 1 >= OUT_BUF_SIZE) {
        goto Bail;
    }
    out_buf[len++] = 0; // end-of-name

    // prepare FWDENTRY
    if (ccnl_ccnb_mkHeader(fwdentry_buf, fwdentry_buf + FWDENTRY_BUF_SIZE, CCNL_DTAG_PREFIX, CCN_TT_DTAG, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, fwdentry_buf + FWDENTRY_BUF_SIZE, CCN_DTAG_ACTION, CCN_TT_DTAG, cp, &len3)) {
        goto Bail;
    }
    if (ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, fwdentry_buf + FWDENTRY_BUF_SIZE, CCN_DTAG_NAME, CCN_TT_DTAG, ccnl_prefix_to_str(p,s,CCNL_MAX_PREFIX_SIZE), &len3)) { // prefix
        goto Bail;
    }

    if (ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, fwdentry_buf + FWDENTRY_BUF_SIZE, CCN_DTAG_FACEID, CCN_TT_DTAG, (char*) faceid, &len3)) {
        goto Bail;
    }
    memset(h,0,sizeof(h));
    snprintf((char*)h, sizeof(h), "%d", (int)suite[0]);
    if (ccnl_ccnb_mkStrBlob(fwdentry_buf+len3, fwdentry_buf + FWDENTRY_BUF_SIZE, CCNL_DTAG_SUITE, CCN_TT_DTAG, (char*) h, &len3)) {
        goto Bail;
    }
    fwdentry_buf[len3++] = 0; // end-of-fwdentry

    if (ccnl_ccnb_mkBlob(out_buf+len, out_buf + OUT_BUF_SIZE, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
                   (char*) fwdentry_buf, len3, &len)) {
        goto Bail;
    }

    if (ccnl_mgmt_send_return_split(ccnl, orig, prefix, from, len, (unsigned char*)out_buf)) {
        goto Bail;
    }

    /*END ANWER*/

    rc = 0;

Bail:

    ccnl_free(suite);
    ccnl_free(faceid);
    ccnl_free(action);
    ccnl_prefix_free(p);
    if (rc) {
        ccnl_free(fwd);
    }

    //ccnl_mgmt_return_msg(ccnl, orig, from, cp);
    return rc;
}

int8_t
ccnl_mgmt_addcacheobject(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    uint8_t *buf;
    uint8_t *components = 0, *h = 0, *h2 = 0, *h3 = 0;
    size_t buflen;
    uint32_t chunknum = 0, chunkflag = 0;
    uint64_t num;
    uint8_t typ;
    char suite = 2;
    int ret;
    struct ccnl_prefix_s *prefix_new;
    char s[CCNL_MAX_PREFIX_SIZE];

    buf = prefix->comp[3];
    buflen = prefix->complen[3];

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto Bail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        goto Bail;
    }

    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)){
        if (num == 0 && typ == 0) {
            break; // end
        }
        extractStr(h, CCNL_DTAG_SUITE);
        extractStr(h2, CCNL_DTAG_CHUNKNUM);
        extractStr(h3, CCNL_DTAG_CHUNKFLAG);
        if (h) {
            long suite_l;
            errno = 0;
            suite_l = strtol((const char*)h, NULL, 0);
            if (errno || suite_l < CHAR_MIN || suite_l > CHAR_MAX) {
                DEBUGMSG(WARNING, "mgmt: cannot parse suite: %s\n", h);
                goto Bail;
            }
            suite = (char) suite_l;
            ccnl_free(h);
            h=0;
        }
        if (h2) {
            unsigned long chunknum_l;
            errno = 0;
            chunknum_l = strtoul((const char*) h2, NULL, 0);
            if (errno || chunknum_l > UINT32_MAX) {
                DEBUGMSG(WARNING, "mgmt: cannot parse chunknum: %s\n", h2);
                goto Bail;
            }
            chunknum = (uint32_t) chunknum_l;
            ccnl_free(h2);
            h2=0;
        }
        if (h3) {
            unsigned long chunkflag_l;
            errno = 0;
            chunkflag_l = strtoul((const char*) h3, NULL, 0);
            if (errno || chunkflag_l > UINT32_MAX) {
                DEBUGMSG(WARNING, "mgmt: cannot parse chunkflag: %s\n", h2);
                goto Bail;
            }
            chunkflag = (uint32_t) chunkflag_l;
            ccnl_free(h3);
            h3=0;
            break;
        }
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto Bail;
        }
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME) {
        goto Bail;
    }

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto Bail;
    }
    if (typ != CCN_TT_BLOB) {
        goto Bail;
    }

    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        if (num == 0 && typ == 0) {
            break; // end
        }
        extractStr(components, CCN_DTAG_COMPONENT);
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto Bail;
        }
    }

    printf("components: %s\n", components);

    prefix_new = ccnl_URItoPrefix((char *)components, CCNL_SUITE_CCNB, chunkflag ? &chunknum : NULL);

    ccnl_free(components);
    components = NULL;
    prefix_new->suite = suite;

    DEBUGMSG(TRACE, "  mgmt: adding object %s to cache (suite=%s)\n",
             ccnl_prefix_to_str(ccnl_prefix_dup(prefix_new),s,CCNL_MAX_PREFIX_SIZE),
             ccnl_suite2str(suite));

    //Reply MSG
    if (h) {
        ccnl_free(h);
    }
    h = ccnl_malloc(300);
    if (!h) {
        goto Bail;
    }

    ret = snprintf((char *)h, 300, "received add to cache request, inizializing callback for %s",
            ccnl_prefix_to_str(prefix_new,s,CCNL_MAX_PREFIX_SIZE));
    if (ret < 0 || (unsigned) ret >= 300) {
        goto Bail;
    }
    if (ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "addcacheobject", (char *)h)) {
        ccnl_free(h);
        goto Bail;
    }
    ccnl_free(h);

    //Reply MSG END
    {
        struct ccnl_pkt_s *pkt = NULL;
        struct ccnl_interest_s *interest = NULL;
        struct ccnl_buf_s *buffer = NULL;

        pkt = ccnl_calloc(1, sizeof(*pkt));
        if (!pkt) {
            goto Bail;
        }
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
        if (!pkt->buf) {
            goto Bail;
        }
        pkt->val.final_block_id = -1;
        buffer = buf_dup(pkt->buf);
        if (!buffer) {
            goto Bail;
        }

        interest = ccnl_interest_new(ccnl, from, &pkt);
        if (!interest) {
            goto Bail;
        }

        //Send interest to from!
        ccnl_face_enqueue(ccnl, from, buffer);
    }
//    ccnl_prefix_free(prefix_new);

    return 0;

Bail:
    return -1;
}

int8_t
ccnl_mgmt_removecacheobject(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{

    uint8_t *buf;
    uint8_t **components = 0;
    size_t num_of_components = 0;
    size_t buflen, i;
    uint64_t num;
    uint8_t typ;
    int8_t rc = -1;
    char *answer = "Failed to remove content";
    struct ccnl_content_s *c2;

    components = (uint8_t**) ccnl_malloc(sizeof(uint8_t*)*1024);
    if (!components) {
        goto Bail;
    }
    for (i = 0; i < 1024; ++i) {
        components[i] = 0;
    }

    buf = prefix->comp[3];
    buflen = prefix->complen[3];

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENTOBJ) {
        goto SoftBail;
    }

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }

    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) {
        goto SoftBail;
    }

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_BLOB) {
        goto SoftBail;
    }

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) {
        goto SoftBail;
    }

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto SoftBail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME) {
        goto SoftBail;
    }

    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        if (num == 0 && typ == 0) {
            break; // end
        }
        ++num_of_components;
        extractStr(components[num_of_components - 1], CCN_DTAG_COMPONENT);

        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto SoftBail;
        }
    }

    for (c2 = ccnl->contents; c2; c2 = c2->next) {
        if (c2->pkt->pfx->compcnt != num_of_components) {
            continue;
        }
        for (i = 0; i < num_of_components; ++i) {
            if (strcmp((char*)c2->pkt->pfx->comp[i], (char*)components[i])) {
                break;
            }
        }
        if (i == num_of_components) {
            break;
        }
    }
    if (i == num_of_components){
        DEBUGMSG(TRACE, "Content found\n");
        ccnl_content_remove(ccnl, c2);
    } else {
       DEBUGMSG(TRACE, "Ignore request since content not found\n");
       goto SoftBail;
    }
    answer = "Content successfully removed";

SoftBail:
    //send answer
    if (ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, "removecacheobject", answer)) {
        goto Bail;
    }

    rc = 0;

Bail:
    ccnl_free(components);
    return rc;
}

#ifdef USE_SIGNATURES
int8_t
ccnl_mgmt_validate_signature(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                    struct ccnl_prefix_s *prefix, struct ccnl_face_s *from, char *cmd)
{

    uint8_t *buf;
    uint8_t *data;
    size_t buflen, datalen, siglen = 0;
    uint64_t num;
    uint8_t typ;
    uint8_t *sigtype = 0, *sig = 0;

    buf = orig->data;
    if (orig->datalen < 0) {
        return -1;
    }
    buflen = (size_t) orig->datalen;

    //SKIP HEADER FIELDS
    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto Bail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_INTEREST) {
        goto Bail;
    }

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto Bail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_NAME) {
        goto Bail;
    }

    if (ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {
        goto Bail;
    }
    if (typ != CCN_TT_DTAG || num != CCN_DTAG_SIGNATURE) {
        goto Bail;
    }
    while (!ccnl_ccnb_dehead(&buf, &buflen, &num, &typ)) {

        if (num == 0 && typ == 0) {
            break; // end
        }

        extractStr(sigtype, CCN_DTAG_NAME);
        siglen = buflen;
        extractStr(sig, CCN_DTAG_SIGNATUREBITS);
        if (ccnl_ccnb_consume(typ, num, &buf, &buflen, 0, 0)) {
            goto Bail;
        }
    }
    siglen = siglen - (buflen+4);

    datalen = buflen - 2;
    data = buf;

    if (ccnl_crypto_verify(ccnl, data, datalen, (char *)sig, siglen, "ccnl_mgmt_crypto", from->faceid)) {
        goto Bail;
    }

    return 0;

Bail:
    ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, cmd,
                "refused: signature could not be validated");
    return -1;
}
#endif /*USE_SIGNATURES*/

int8_t
ccnl_mgmt_handle(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
                 struct ccnl_prefix_s *prefix, struct ccnl_face_s *from,
                 char *cmd, int8_t verified) {
    DEBUGMSG(TRACE, "ccnl_mgmt_handle \"%s\"\n", cmd);
    if (!verified) {
        ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, cmd,
                                 "refused: error signature not verified");
        return -1;
    }

    if (!strcmp(cmd, "newdev")) {
        return ccnl_mgmt_newdev(ccnl, orig, prefix, from);
    } else if (!strcmp(cmd, "setfrag")) {
        return ccnl_mgmt_setfrag(ccnl, orig, prefix, from);
    } else if (!strcmp(cmd, "destroydev")) {
        return ccnl_mgmt_destroydev(ccnl, orig, prefix, from);
#ifdef USE_ECHO
    } else if (!strcmp(cmd, "echoserver")) {
        return ccnl_mgmt_echo(ccnl, orig, prefix, from);
#endif
    } else if (!strcmp(cmd, "newface")) {
        return ccnl_mgmt_newface(ccnl, orig, prefix, from);
    } else if (!strcmp(cmd, "destroyface")) {
        return ccnl_mgmt_destroyface(ccnl, orig, prefix, from);
    } else if (!strcmp(cmd, "prefixreg")) {
        return ccnl_mgmt_prefixreg(ccnl, orig, prefix, from);
//  TODO: Add ccnl_mgmt_prefixunreg(ccnl, orig, prefix, from)
//  } else if (!strcmp(cmd, "prefixunreg")) {
//      return ccnl_mgmt_prefixunreg(ccnl, orig, prefix, from);
#ifdef USE_DEBUG
    } else if (!strcmp(cmd, "addcacheobject")) {
        return ccnl_mgmt_addcacheobject(ccnl, orig, prefix, from);
    } else if (!strcmp(cmd, "removecacheobject")) {
        return ccnl_mgmt_removecacheobject(ccnl, orig, prefix, from);
    } else if (!strcmp(cmd, "debug")) {
        return ccnl_mgmt_debug(ccnl, orig, prefix, from);
#endif
    }

    DEBUGMSG(TRACE, "unknown mgmt command %s\n", cmd);
    ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, cmd, "unknown mgmt command");
    return -1;
}

int8_t
ccnl_mgmt(struct ccnl_relay_s *ccnl, struct ccnl_buf_s *orig,
          struct ccnl_prefix_s *prefix, struct ccnl_face_s *from)
{
    char cmd[1000];
    if (prefix->complen[2] < sizeof(cmd)) {
        memcpy(cmd, prefix->comp[2], prefix->complen[2]);
        cmd[prefix->complen[2]] = '\0';
    } else {
        strcpy(cmd, "cmd-is-too-long-to-display");
    }

    DEBUGMSG(TRACE, "ccnl_mgmt request \"%s\"\n", cmd);

    if (ccnl_is_local_addr(&from->peer)) {
        goto MGMT;
    }

#ifdef USE_SIGNATURES
    return ccnl_mgmt_validate_signature(ccnl, orig, prefix, from, cmd);
#endif /*USE_SIGNATURES*/

    DEBUGMSG(TRACE, "  rejecting because src=%s is not a local addr\n",
            ccnl_addr2ascii(&from->peer));
    if (ccnl_mgmt_return_ccn_msg(ccnl, orig, prefix, from, cmd,
                "refused: origin of mgmt cmd is not local")) {
        return -1;
    }
    return -1;

MGMT:
    return ccnl_mgmt_handle(ccnl, orig, prefix, from, cmd, 1);
}

#endif // USE_MGMT

// eof

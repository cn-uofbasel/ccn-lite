/**
 * @f ccnl-cs-ll.c
 * @b Content store implementation using linked lists
 *
 * Copyright (C) 2019 Safety IO
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
#include <stdio.h>
#include <string.h>

#include "utlist.h"
#include "ccnl-cs.h"
#include "ccnl-pkt.h"
#include "ccnl-cs-helper.h"
#include "ccnl-fwd.h"
#include "ccnl-dispatch.h"

/**
 * Linked list of \ref ccnl_cs_content_t elements
 */
static ccnl_cs_content_t *ll = NULL;

/**
 * The struct holds a list of \ref dispatchFct and 
 * \ref cMatchFct function pointers per packet types. 
 *
 * TODO: refactor for lose coupling
extern struct ccnl_suite_s ccnl_core_suites[CCNL_SUITE_LAST];
 */

/**
 * @brief Wraps a @p name into a @p content object
 *
 * The functions embeds a @p name into @p content object, so it can be used for
 * the \ref _content_cmp_name function. The entries in the content store store 
 * the prefix/name of a content object in a packet representation. The packet
 * representation holds a fully encoded TLV for the prefix (in case of NDN) and
 * thus won't match with a prefix/name which was just encoded with a call to 
 * \ref ccnl_URItoPrefix
 *
 * @param[in]  name The name to be embedded into a @p content object
 * @param[out] content The newly created content object which holds the given @p name
 *
 */
static void _embed_ccnl_cs_name(ccnl_cs_name_t *name, ccnl_cs_content_t **content);

/**
 * Compares two \ref ccnl_cs_content_t elements
 *
 * @param[in] a The first item 
 * @param[in] b The second item
 *
 * @return 0 if the first item \ref a is equal to the second item \ref b
 * @return -1 if the first item \ref a should sort before the second item \ref b
 * @return 1 if the first item \ref a should sort after the second item \ref b
 */
//static int _content_cmp(const ccnl_cs_content_t *a, const ccnl_cs_content_t *b);

static int _content_cmp_name(const ccnl_cs_content_t *a, const ccnl_cs_content_t *b);

/**
 * \ref ccnl_cs_add
 */
static ccnl_cs_status_t ccnl_cs_ll_add(const ccnl_cs_name_t *name, const ccnl_cs_content_t *content);
/**
 * \ref ccnl_cs_lookup
 */
static ccnl_cs_status_t ccnl_cs_ll_lookup(const ccnl_cs_name_t *name, ccnl_cs_content_t **content);
/**
 * \ref ccnl_cs_remove
 */
static ccnl_cs_status_t ccnl_cs_ll_remove(const ccnl_cs_name_t *name);
/**
 * \ref ccnl_cs_clear
 */
static ccnl_cs_status_t ccnl_cs_ll_clear(void);
/**
 * \ref
 */
static ccnl_cs_status_t ccnl_cs_ll_dump(void);
/**
 * \ref
 */
static ccnl_cs_status_t ccnl_cs_ll_age(void);
/**
 * \ref
 */
static ccnl_cs_status_t ccnl_cs_ll_remove_oldest_entry(void);


static int ccnl_cs_ll_match_interest(const struct ccnl_pkt_s *packet, ccnl_cs_content_t *content);

/**
 * \ref ccnl_cs_exists
 */
static bool ccnl_cs_ll_exists(ccnl_cs_name_t *name);

/**
 * Compares two content store entries.
 *
 * The function checks if an element \ref a in the content store matches an element \ref b. Particulary,
 * the function first checks if the packet types match (since the CS can handle different packet types
 * at the same time) and then passes the items to a \ref cMatchFct function. This function is unique per 
 * packet type. See also \ref ccnl_suite_s. This function is only used for \ref ccnl_fwd_handleInterest
 *
 * @param[in] a The first item 
 * @param[in] b The second item
 *
 * @return ?
 */
static int ccnl_cs_ll_cmp_match_interest(const ccnl_cs_content_t *a, const ccnl_cs_content_t *b);

void ccnl_cs_init_ll(ccnl_cs_ops_t *ccnl_cs_ops_ll) {
    /* set the function pointers */
    ccnl_cs_init(ccnl_cs_ops_ll, ccnl_cs_ll_add, ccnl_cs_ll_lookup, ccnl_cs_ll_remove, ccnl_cs_ll_clear, 
        ccnl_cs_ll_dump, ccnl_cs_ll_age, NULL, ccnl_cs_ll_remove_oldest_entry, ccnl_cs_ll_match_interest);
}

static int _content_cmp_name(const ccnl_cs_content_t *a, const ccnl_cs_content_t *b) {
    /** do the number of components match */
    if (a->pkt->pfx->compcnt == b->pkt->pfx->compcnt) {
        /** does the length of the names match */
        if (a->pkt->pfx->namelen== b->pkt->pfx->namelen) {
            /** compare the names */
            return memcmp(a->pkt->pfx->nameptr, b->pkt->pfx->nameptr, a->pkt->pfx->namelen);
        }
    }

    /** TODO: think about the return value */
    return -1;
}

static ccnl_cs_status_t ccnl_cs_ll_add(const ccnl_cs_name_t *name, const ccnl_cs_content_t *content) {
    ccnl_cs_status_t result = CS_OPERATION_UNSUCCESSFUL;

    if (!ccnl_cs_ll_exists((ccnl_cs_name_t*)name)) {
        DL_APPEND(ll, (ccnl_cs_content_t*)content);
        result = CS_OPERATION_WAS_SUCCESSFUL;
    }

    return result;
}

static void _embed_ccnl_cs_name(ccnl_cs_name_t *name, ccnl_cs_content_t **content) {
    /** create some exemplary payload (which is not used) */
    size_t payload_len = sizeof(uint8_t);
    uint8_t *payload = malloc(payload_len);
    /** create a new content object with the given name */
    ccnl_cs_build_content(content, name, NULL, payload, payload_len); 
    /** free the payload */
    free(payload);
}

static ccnl_cs_status_t ccnl_cs_ll_lookup(const ccnl_cs_name_t *name, ccnl_cs_content_t **content) {
    ccnl_cs_status_t result = CS_OPERATION_UNSUCCESSFUL;

    ccnl_cs_content_t *temporary = NULL;
    _embed_ccnl_cs_name((ccnl_cs_name_t*)name, &temporary);

    if (temporary) {
        /** search if there is an actual element already in the list which matches the name */
        DL_SEARCH(ll, *content, temporary, _content_cmp_name); 

        /** check if an element was found */
        if (content) {
            result = CS_OPERATION_WAS_SUCCESSFUL;
        }

        ccnl_content_free(temporary);
    }

    return result;
}

static ccnl_cs_status_t ccnl_cs_ll_remove(const ccnl_cs_name_t *name) {
    ccnl_cs_status_t result = CS_NAME_COULD_NOT_BE_FOUND;

    /** build a temporary content element which encapsulates the name */
    ccnl_cs_content_t *temporary = NULL;
    _embed_ccnl_cs_name((ccnl_cs_name_t*)name, &temporary);

    if (temporary) {
        /** temporary element which is used during search */
        ccnl_cs_content_t *content = NULL;

        /** search if there is an actual element already in the list which matches the name */
        DL_SEARCH(ll, content, temporary, _content_cmp_name); 

        /** check if an element was found */
        if (content) {
            /** delete the content associated with that name */
            DL_DELETE(ll, content);
            result = CS_OPERATION_WAS_SUCCESSFUL;
        }

        ccnl_content_free(temporary);
    }

    return result;
}

static ccnl_cs_status_t ccnl_cs_ll_clear(void) {
    ccnl_cs_content_t *element, *temp;

    DL_FOREACH_SAFE(ll, element, temp) {
        DL_DELETE(ll, element);
        ccnl_content_free(element);
    }

    return CS_OPERATION_WAS_SUCCESSFUL;
}

static ccnl_cs_status_t ccnl_cs_ll_dump(void) {
    ccnl_cs_content_t *element;

    DL_FOREACH(ll, element) {
        printf("TODO");
    }

    return 0;
}

static ccnl_cs_status_t ccnl_cs_ll_age(void) {
    return 0;
}

static ccnl_cs_status_t ccnl_cs_ll_remove_oldest_entry(void) {
    /*
    if (ccnl->max_cache_entries > 0 &&
        ccnl->contentcnt >= ccnl->max_cache_entries) { // remove oldest content
        struct ccnl_content_s *c2, *oldest = NULL;
        uint32_t age = 0;
        for (c2 = ccnl->contents; c2; c2 = c2->next) {
             if (!(c2->flags & CCNL_CONTENT_FLAGS_STATIC)) {
                 if ((age == 0) || c2->last_used < age) {
                     age = c2->last_used;
                     oldest = c2;
                 }
             }
         }
         if (oldest) {
             DEBUGMSG_CORE(DEBUG, " remove old entry from cache\n");
             ccnl_content_remove(ccnl, oldest);
         }
    }
    */
    return -1;
}

static bool ccnl_cs_ll_exists(ccnl_cs_name_t *name) {
    /** build a temporary content element which encapsulates the name */
    ccnl_cs_content_t *temporary = NULL;
    _embed_ccnl_cs_name(name, &temporary);

    /** temporary element which is used during search */
    ccnl_cs_content_t *content = NULL;

    if (temporary) {
        /** search if there is an actual element already in the list which matches the name */
        DL_SEARCH(ll, content, temporary, _content_cmp_name); 
        
        ccnl_content_free(temporary);
    }

    /** check if an element was found */
    return (content != NULL); 
}

static int ccnl_cs_ll_match_interest(const struct ccnl_pkt_s *packet, ccnl_cs_content_t *content) {
    /** temporary element which is used during search */
    ccnl_cs_content_t temporary;
    temporary.pkt = (struct ccnl_pkt_s*)packet;

    /** iterate over all elements in the content store */
    DL_SEARCH(ll, content, &temporary, ccnl_cs_ll_cmp_match_interest); 

    // check
    return (content != NULL);
}


static int ccnl_cs_ll_cmp_match_interest(const ccnl_cs_content_t *a, const ccnl_cs_content_t *b) {
    int result = -1;

    /** prefixes do not match */
    if (a->pkt->pfx->suite != b->pkt->pfx->suite) {
        return result;
    }
    
    /**
     * 
     */
    switch (b->pkt->pfx->suite) {
#ifdef USE_SUITE_NDNTLV 
        case CCNL_SUITE_NDNTLV:
             result = ccnl_core_suites[CCNL_SUITE_NDNTLV].cMatch(b->pkt, (ccnl_cs_content_t*)a);
//             result = ccnl_ndntlv_cMatch(b->pkt, (ccnl_cs_content_t*)a);
             break;
#endif
             /*
#ifdef USE_SUITE_CCNB
        case CCNL_SUITE_CCNB:
             result = ccnl_core_suites[CCNL_SUITE_CCNB].cMatch(b->pkt, (ccnl_cs_content_t*)a);
             break;
#endif
#ifdef USE_SUITE_CCNTLV
        case CCNL_SUITE_CCNTLV:
             result = ccnl_core_suites[CCNL_SUITE_CCNTLV].cMatch(b->pkt, (ccnl_cs_content_t*)a);
             break;
#endif
*/
        default:
             result = -2;
             break;
    }

    return result;
}

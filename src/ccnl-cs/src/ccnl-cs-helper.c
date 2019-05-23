/**
 * @f ccnl-cs-helper.c
 * @b Implementation of helper functions for the content store
 *
 * Copyright (C) 2018 MSA Safety
 * Copyright (C) 2018 Safety IO
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

#include "ccnl-defs.h"
#include "ccnl-pkt.h"
//#include "ccnl-content.h" // TODO: refactor
#include "ccnl-logging.h"
#include "ccnl-os-time.h"
#include "ccnl-cs-helper.h"

#include <string.h>

ccnl_cs_name_t *ccnl_cs_create_name(const char* name, size_t length) {
    (void)name;
    (void)length;
    /** no valid name was passed to the function 
    if ((!name) || (length == 0)) {
        return NULL;
    }
     */

    /** allocate the size of the structure + the size of the string we'll store
    size_t size = sizeof(ccnl_cs_name_t); // + (sizeof(uint8_t) * length);
    ccnl_cs_name_t *result = malloc(size);

    if (result) {
        size = (sizeof(uint8_t) * length);
        result->name = malloc(size);

        if (result->name) {
     */
            /* set members of 'result' struct */
         //   memcpy(result->name, name, length);
            /** length of the name */
         //   result->length = length;

            /** the number of components */
          //  result->componentcount = 42;
        //} else {
          //  free(result);
           // result = NULL;
        //}
    //}

    //return result;
    return NULL;
}


ccnl_cs_content_t *ccnl_cs_create_content(uint8_t *content, size_t size) {
    (void)content;
    (void)size;
    /** no valid data was passed to the function 
    if ((!content) || (size == 0)) {
        return NULL;
    }

    ccnl_cs_content_t *result = malloc(sizeof(ccnl_cs_content_t));

    if (result) {
        result->content = malloc(sizeof(uint8_t) * size);

        if (result->content) {
     */
            /* set members of 'result' struct 
            memcpy(result->content, content, size);
            result->length = size;
             */
        /* could not allocate memory for member 'content' 
        } else {
            free(result);
            result = NULL;
        }
    }

         */
    return NULL;
    //return result;
}

/*
ccnl_cs_name_t *ccnl_cs_prefix_to_name(struct ccnl_prefix_s *prefix) {
    ccnl_cs_name_t *result = NULL;

    if (prefix) {
        result = malloc(sizeof(ccnl_cs_name_t));

    }

    return result;
}
void 
ccnl_prefix_cmp(struct ccnl_prefix_s *pfx, unsigned char *md,
                struct ccnl_prefix_s *nam, int mode);
*/
int ccnl_cs_name_compare(ccnl_cs_name_t *first, ccnl_cs_name_t *second, uint8_t mode) {
    int result = -1;
    (void)first;
    (void)second;
    (void)mode;

    /** parameter(s) are not valid 
    if (!first || !second) {
        return result;
    }
     */

    /* TODO: not so sure about the checks here 
    if (first->component.count > 0) {
        if (!first->component.components) {
            return result;
        }
    }

    if (second->component.count > 0) {
        if (!second->component.components) {
            return result;
        }
    }
     */

    /** unknown mode 
    if (mode > 42) {
        return -2;
    }

    if (mode == CCNL_CS_HELPER_COMPARE_EXACT) {
        */
        /** check if the number of components match 
        if (first->component.count != second->component.count) {
            return result;
        }
         */

        /** check if the components are chunked 
        if (first->chunknum || second->chunknum) {
         */
            /** check if only one of the components is 'chunked' 
            if (!first->chunknum || !second->chunknum) {
                return result;
            }
             */
            /** check if the number of chunks mismatch 
            if (*first->chunknum != *second->chunknum) {
                return result;
            }
        }
    }

    uint32_t i = 0;
    uint32_t clen;
    unsigned char *comp;

    for (i = 0; i < first->component.count && i < second->component.count; ++i) {
        if (i < first->component.count) {
            comp = first->component.components[i];
            clen = first->component.length[i];
        } else {
//            comp = md;
            clen = 32; // SHA256_DIGEST_LEN
        }

// TODO: CHECK
        if (clen != second->component.length[i] || memcmp(comp, second->component.components[i], second->component.length[i])) {
            //result = mode == CCNL_CS_HELPER_COMPARE_EXACT ? -1 : i;
            if (mode != CCNL_CS_HELPER_COMPARE_EXACT) {
                // that's actually an issue
                result = i;
            } 

//            DEBUGMSG(VERBOSE, "component mismatch: %i\n", i);
            result = i;
            return result;
        }
    }

    // FIXME: we must also inspect chunknum here!
    //rc = (mode == CCNL_CS_HELPER_COMPARE_EXACT) ? 0 : i;

             */
    return result;
}

bool ccnl_cs_perform_ageing(ccnl_cs_content_t *c)
{
    bool result = false;
    (void)c;
    /*
    time_t t = CCNL_NOW();

    if ((result = ((c->options.last_used + CCNL_CONTENT_TIMEOUT) <= (uint32_t) t &&
            !(c->options.flags & CCNL_CONTENT_FLAGS_STATIC)))){
        DEBUGMSG_CORE(TRACE, "AGING: CONTENT REMOVE %p\n", (void*) c);
    } else {
#ifdef USE_SUITE_NDNTLV
        if (c->packet->suite == CCNL_SUITE_NDNTLV) {
            // Mark content as stale if its freshness period expired and it is not static
            if ((c->options.last_used + (c->packet->s.ndntlv.freshnessperiod / 1000)) <= (uint32_t) t &&
                    !(c->options.flags & CCNL_CONTENT_FLAGS_STATIC)) {
                c->options.flags |= CCNL_CONTENT_FLAGS_STALE;
            }
        }
#endif
    }
    */
    return result;
}

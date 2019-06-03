/**
 * @f ccnl-cs-uthash.c
 * @b Content store implementation based on uthash
 *
 * Copyright (C) 2018 MSA Safety 
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
#include <stdbool.h>
#include <string.h>

#include "ccnl-pkt.h" 
#include "ccnl-prefix.h" // FIXME 
#include "ccnl-malloc.h"
#include "ccnl-cs-helper.h"
#include "ccnl-cs-uthash.h"

/** The actual hashmap of the uthash-based content store implementation */
static ccnl_cs_uthash_t *hashmap = NULL;


static ccnl_cs_status_t ccnl_cs_uthash_add(const ccnl_cs_name_t *name, const ccnl_cs_content_t *content);
static ccnl_cs_status_t ccnl_cs_uthash_lookup(const ccnl_cs_name_t *name, ccnl_cs_content_t **content);
static ccnl_cs_status_t ccnl_cs_uthash_remove(const ccnl_cs_name_t *name);
static ccnl_cs_status_t ccnl_cs_uthash_clear(void);
static ccnl_cs_status_t ccnl_cs_uthash_dump(void);
static ccnl_cs_status_t ccnl_cs_uthash_age(void);

void ccnl_cs_init_uthash(ccnl_cs_ops_t *ccnl_cs_ops_uthash) {
    /* set the function pointers */
    ccnl_cs_init(ccnl_cs_ops_uthash, ccnl_cs_uthash_add, ccnl_cs_uthash_lookup, ccnl_cs_uthash_remove, ccnl_cs_uthash_clear, 
        ccnl_cs_uthash_dump, ccnl_cs_uthash_age, NULL, NULL, NULL);
}

static ccnl_cs_status_t ccnl_cs_uthash_add(const ccnl_cs_name_t *name, const ccnl_cs_content_t *content) {
    (void)content;
    ccnl_cs_status_t result = CS_OPERATION_UNSUCCESSFUL;
    /* name is not already stored in the content store */
    if (!ccnl_cs_uthash_exists((ccnl_cs_name_t*)name)) {
//        HASH_ADD_KEYPTR(hh, hashmap, (ccnl_cs_name_t*)name, entry->key->complen, (ccnl_cs_content_t*)content);
        result = CS_OPERATION_WAS_SUCCESSFUL;
    }

    return result;
}

static ccnl_cs_status_t ccnl_cs_uthash_lookup(const ccnl_cs_name_t *name, ccnl_cs_content_t **content) {
    (void)name;
    (void)content;
    //ccnl_cs_uthash_t *entry = NULL;
    /* lookup if an entry for the given name exists 
    HASH_FIND(hh, hashmap, name->name, name->length, entry); 

    if (entry) {
        // TODO: should be caught in ccnl_cs_lookup
        if (content) {
            content->content = entry->value->content;
            content->length = entry->value->length;

            return CS_OPERATION_WAS_SUCCESSFUL;
        }
    }
     */

    return CS_NAME_COULD_NOT_BE_FOUND;
}

static ccnl_cs_status_t ccnl_cs_uthash_remove(const ccnl_cs_name_t *name) {
    (void)name;
    //ccnl_cs_uthash_t *entry = NULL;
    /* lookup if an entry for the given name exists */
    //HASH_FIND(hh, hashmap, name->name, name->length, entry); 

    /* check if entry was filled by the lookup  
    if (entry) {
#ifdef CCNL_RIOT
        evtimer_del((evtimer_t *)(&ccnl_evtimer), (evtimer_event_t *)&entry->options.timeout);
#endif
     */
        /* free the packet 
        if (entry->value->packet) {
            ccnl_prefix_free(entry->value->packet->pfx);
            ccnl_free(entry->value->packet->buf);
            ccnl_free(entry->value->packet);
        }
*/
        /* free the entry */
     //   free(entry);

      //  return CS_OPERATION_WAS_SUCCESSFUL;
    //}

    return CS_NAME_COULD_NOT_BE_FOUND;
}

bool ccnl_cs_uthash_exists(ccnl_cs_name_t *name) {
    ccnl_cs_uthash_t *entry = NULL;

    if (name) {
       // HASH_FIND(hh, hashmap, name, name->complen, entry); 
    }

    /* if no entry could be found the, the temporary element 'entry' points to NULL */
    return (entry != NULL);
}

static int ccnl_cs_uthash_dump(void) {
    /*
    ccnl_cs_uthash_t *entry;

    for (entry = hashmap; entry != NULL; entry = entry->hh.next) {
        printf("name: %s length: %ld\n", entry->key->name, entry->key->length);
        printf("content: \n");
        printf("\t");

        for(uint32_t i = 0; i < entry->value->length; i++) {
            printf("%04x ", entry->value->content[i]);

            if ((i != 0) && (i % 8 == 0)) {
                printf("\n");
                printf("\t");
            }
        }

        printf("\nlength: %d\n", entry->value->length);
        printf("\n");
    }
    */
    return 0;
}

static ccnl_cs_status_t ccnl_cs_uthash_clear(void) {
    ccnl_cs_uthash_t *current_entry, *tmp;
    
    HASH_ITER(hh, hashmap, current_entry, tmp) {
        HASH_DEL(hashmap, current_entry);
        ccnl_content_free(current_entry->value);            
    }

    return 0;
}

static ccnl_cs_status_t ccnl_cs_uthash_age(void)
{
        /*
    ccnl_cs_uthash_t *current_entry, *tmp;
    
    HASH_ITER(hh, hashmap, current_entry, tmp) {
        if (ccnl_cs_perform_ageing(current_entry->value)) {
            HASH_DEL(hashmap, current_entry);
            free(current_entry);            
        }
    }
*/
    return 0;
}


#include "ccnl-cs-hashmap.h"

/**
 * Holds the 
 */
static ccnl_cs_hashmap_t *table = NULL;


int init(const uint32_t size)
{
    /* set return code to 'allocation of table failed' */
    int result = -2;
    /* allocate memory for the actual table */
    table = (ccnl_cs_hashmap_t*) malloc(sizeof(ccnl_cs_hashmap_t));

    if (table) {
        table->entries = (ccnl_cs_hashmap_entry_t**) calloc(size, sizeof(ccnl_cs_hashmap_entry_t));

        if (table->entries) {
             table->size = size;
             result = 0;
        } else {
            /* could not create entries, hence free the table structure */
            free(table);
            result = -1;
        }
    }

    return result;
}

void add(ccnl_cs_name_t *key, ccnl_cs_content_t *value)
{
    int index = hash(key);

    /* check if hash map is full */


    /* set data */
    table->entries[index]->key = key;
    table->entries[index]->value = value;
    table->size++;

}

int size(void)
{
    return table ? table->size : 0;
}

void deinit(void)
{
    free(table->entries);
    free(table);
}

int hash(ccnl_cs_name_t *key)
{
    (void)key;
    return 0;
}

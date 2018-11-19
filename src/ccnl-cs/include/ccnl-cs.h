/**
 * @f ccnl-cs.h
 * @b CCN lite - Content store implementation
 *
 * Copyright (C) 2018 HAW Hamburg
 * Copyright (C) 2018 MSA Safety 
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
 */

#ifndef CCNL_CS
#define CCNL_CS

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @brief Provides status codes for content store operations
 */
typedef enum {
    CS_OPERATION_UNSUCCESSFUL = 0,   /**< operation was not successfull */
    CS_OPERATION_WAS_SUCCESSFUL = 1, /**< operation was successfull */
    CS_OPTIONS_ARE_NULL = -1,        /**< \ref ccnl_cs_ops_t are NULL */
    CS_NAME_IS_INVALID = -2,         /**< name is invalid or NULL */
    CS_CONTENT_IS_INVALID = -3,      /**< content is invalid or NULL */

    CS_NAME_COULD_NOT_BE_FOUND = -4, /**< a name could not be found */
    
    CS_DO_NOT_USE = INT_MAX          /**< set the enum to a fixed width, do not use! */
} ccnl_cs_status_t;

typedef struct {
    unsigned char **components; /**< the components of the prefix without '\0' at the end */
    size_t count;               /**< the number of components */
    size_t *length;             /**< the actual length of the name components */
} ccnl_cs_component_t;

/**
 * @brief An abstract representation of an ICN name 
 */
typedef struct {
    uint8_t *name;         /**< the name itself */
    size_t length;         /**< the length of the name (formerly known as complen) */

    unsigned char **comp; /**< name components of the prefix without '\0' at the end */
    
    size_t componentcount; /**< the number of components of the name (formerly known as compcnt) */
    
    int *componentlength;  /**< length of the name components */

    ccnl_cs_component_t component; /**< */
    int *chunknum;         /**< if defined, number of the chunk else -1 */
} ccnl_cs_name_t;

/**
 * @brief An abstract representation of ICN content
 */
typedef struct {
    uint8_t *content; /**< A byte representation of the content */
    uint32_t length;  /**< The size of the content */
    uint32_t served;  /**< denotes how often the content has been served */
} ccnl_cs_content_t;

/**
 * Type definition for the function pointer to the add function
 */
typedef int (*ccnl_cs_op_add_t)(const ccnl_cs_name_t *name, const ccnl_cs_content_t *content);

/**
 * Type definition for the function pointer to the lookup function
 */
typedef int (*ccnl_cs_op_lookup_t)(const ccnl_cs_name_t *name, ccnl_cs_content_t *content);

/**
 * Type definition for the function pointer to the remove function
 */
typedef int (*ccnl_cs_op_remove_t)(const ccnl_cs_name_t *name);

/**
 * Type definition for the function pointer to the remove function
 */
typedef int (*ccnl_cs_op_clear_t)(void);

/**
 * Type definition for the function pointer to the print function
 */
typedef int (*ccnl_cs_op_print_t)(void);

/**
 * @brief Holds function pointers to concrete implementations of a content store
 */
typedef struct {
    ccnl_cs_op_add_t add;       /**< Function pointer to the add function */
    ccnl_cs_op_lookup_t lookup; /**< Function pointer to the lookup function */
    ccnl_cs_op_remove_t remove; /**< Function pointer to the remove function */
    ccnl_cs_op_clear_t clear;   /**< Function pointer to the clear function */
    ccnl_cs_op_print_t print;   /**< Function pointer to the print function */
} ccnl_cs_ops_t;

/**
 * @brief Sets the given function pointers to \p ops
 *
 * @param[out] ops The data structure to initialize 
 * @param[in] add_fun A function pointer to the add function
 * @param[in] lookup_fun A function pointer to the lookup function
 * @param[in] remove_fun A function pointer to the remove function
 */
void
ccnl_cs_init(ccnl_cs_ops_t *ops,
             ccnl_cs_op_add_t add_fun,
             ccnl_cs_op_lookup_t lookup_fun,
             ccnl_cs_op_remove_t remove_fun,
             ccnl_cs_op_clear_t clear_fun,
             ccnl_cs_op_print_t print_fun
             );

/**
 * @brief Adds an item to the content store
 *
 * @param[in] ops Data structure which holds the function pointer to the add function
 * @param[in] name The name of the content
 * @param[in] content The content to add
 *
 * @return 0 The content was added successfully to the content store
 * @return -1 An invalid \ref ccnl_cs_op_t struct was passed to the function (e.g. \p ops is NULL)
 * @return -2 An invalid \ref ccnl_cs_name_t struct was passed to the function (e.g. \p name is NULL)
 * @return -3 An invalid \ref ccnl_cs_content_t struct was passed to the function (e.g. \p content is NULL)
 */
ccnl_cs_status_t
ccnl_cs_add(ccnl_cs_ops_t *ops,
            const ccnl_cs_name_t *name,
            const ccnl_cs_content_t *content);

/**
 * @brief Searches the content store for the specified item
 *
 * @param[in] ops Data structure which holds the function pointer to the lookup function
 * @param[in] name The name of the content which is about to searched in the content store
 * @param[out] content If the lookup was successfull, the variable contains the result
 *
 * @return 0 The content could not be found in the content store
 * @return 1 The content was found in the content store
 * @return -1 An invalid \ref ccnl_cs_op_t struct was passed to the function (e.g. \p ops is NULL)
 * @return -2 An invalid \ref ccnl_cs_name_t struct was passed to the function (e.g. \p name is NULL)
 * @return -3 An invalid \ref ccnl_cs_content_t struct was passed to the function (e.g. \p content is NULL)
 */
ccnl_cs_status_t
ccnl_cs_lookup(ccnl_cs_ops_t *ops,
               const ccnl_cs_name_t *name,
               ccnl_cs_content_t *content);

/**
 * @brief Removes an item from the content store 
 *
 * @param[in] ops Data structure which holds the function pointer to the remove function
 * @param[in] name The name of the content to be removed
 *
 * @return 0 The content was removed successfully from the content store
 * @return -1 An invalid \ref ccnl_cs_op_t struct was passed to the function (e.g. \p ops is NULL)
 * @return -2 An invalid \ref ccnl_cs_name_t struct was passed to the function (e.g. \p name is NULL)
 */
ccnl_cs_status_t
ccnl_cs_remove(ccnl_cs_ops_t *ops,
               const ccnl_cs_name_t *name);

ccnl_cs_status_t
ccnl_cs_clear(ccnl_cs_ops_t *ops);

ccnl_cs_status_t
ccnl_cs_print(ccnl_cs_ops_t *ops);

#endif //CCNL_CS

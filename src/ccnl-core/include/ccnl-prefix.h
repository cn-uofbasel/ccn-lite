/**
 * @addtogroup CCNL-core
 * @brief core library of CCN-Lite, contains important datastructures
 * @{
 * @file ccnl-prefix.h
 * @brief CCN lite, core CCNx protocol logic
 *
 * @author Christopher Scherb <christopher.scherb@unibas.ch>
 * @author Christian Tschudin <christian.tschudin@unibas.ch>
 *
 * @copyright (C) 2011-18 University of Basel
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef CCNL_PREFIX_H
#define CCNL_PREFIX_H

#include <stddef.h>
#ifndef CCNL_LINUXKERNEL
#include <stdint.h>
#else
#include <linux/types.h>
#endif
#ifndef CCNL_LINUXKERNEL
#include <unistd.h>
#endif

struct ccnl_content_s;

struct ccnl_prefix_s {
    uint8_t **comp; /**< name components of the prefix without '\0' at the end */
    size_t *complen; /**< length of the name components */
    uint32_t compcnt; /**< number of name components */
    char suite; /**< type of the packet format */
    uint8_t *nameptr; /**< binary name (for fast comparison) */
    ssize_t namelen; /**<  valid length of name memory */
    uint8_t *bytes;   /**< memory for name component copies */
    uint32_t *chunknum;   /**< if defined, number of the chunk else -1 */
};

/**
 * @brief Create a new CCNL_Prefix datastructure
 *
 * @param[in] suite       Packet format for which the Prefix should be created
 * @param[in] cnt         Number of components which the Prefix should contain
 *
 * @return The created Prefix
*/
struct ccnl_prefix_s*
ccnl_prefix_new(char suite, uint32_t cnt);

/**
 * @brief Frees CCNL_Prefix datastructure
 *
 * @param[in] prefix       Prefix to be freed
*/
void 
ccnl_prefix_free(struct ccnl_prefix_s *prefix);

/**
 * @brief Duplicates CCNL_Prefix datastructure
 *
 * @param[in] prefix       Prefix to be duplicated
 *
 * @return The duplicated Prefix
*/
struct ccnl_prefix_s* 
ccnl_prefix_dup(struct ccnl_prefix_s *prefix);

/**
 * @brief Add a component to a Prefix
 *
 * @param[in,out] prefix   Prefix to which the component should be added
 * @param[in] cmp          Component which should be added
 * @param[in] cmplen       Length of the Component which should be added
 *
 * @return      0 on success else < 0
*/
int8_t
ccnl_prefix_appendCmp(struct ccnl_prefix_s *prefix, uint8_t *cmp, size_t cmplen);

/**
 * @brief Set a Cunknum to a Prefix
 *
 * @param[in,out] prefix   Prefix to which the component should be added
 * @param[in] cunknum      Chunknum which should be added
 *
 * @return      0 on success else < 0
*/
int
ccnl_prefix_addChunkNum(struct ccnl_prefix_s *prefix, uint32_t chunknum);

/**
 * @brief Compares two Prefix datastructures
 *
 * @param[in] pfx   Prefix 1 to be compared (shorter of Longest Prefix matching)
 * @param[in] md    ?
 * @param[in] nam   Prefix 2 to be compared (longer of Longest Prefix matching)
 * @param[in] mode  Mode for prefix comp (CMP_EXACT, CMP_MATCH, CMP_LONGEST)
 *
 * @return      -1 if no match at all (all modes) or exact match failed
 * @return      0 if full match (mode = CMP_EXACT) or no components match (mode = CMP_MATCH)
 * @return      n>0 for matched components (mode = CMP_MATCH, CMP_LONGEST)
*/
int32_t
ccnl_prefix_cmp(struct ccnl_prefix_s *pfx, unsigned char *md,
                struct ccnl_prefix_s *nam, int mode);

/**
 * @brief checks if a prefixname is a prefix of a content name
 *
 * @param[in] prefix        Prefix to be compared (shorter of Longest Prefix matching)
 * @param[in] minsuffix     ?
 * @param[in] maxsuffix     ? 
 * @param[in] c             Content to test the prefix against
 *
 * @return      -1 if no match at all (all modes) or exact match failed
 * @return      -2 mismatch in expected number of components between prefix and content
 * @return      -3 computation of digest failed 
 * @return      0 if full match
 * @return      n>0 for matched components
*/
int8_t
ccnl_i_prefixof_c(struct ccnl_prefix_s *prefix,
                  uint64_t minsuffix, uint64_t maxsuffix, struct ccnl_content_s *c);

/**
 * @brief checks if a prefixname is a prefix of a content name
 *
 * @param[in,out] comp       component to be unescaped
 *
 * @return  length of the escaped component
*/
size_t
unescape_component(char *comp);

//int
//ccnl_URItoComponents(char **compVector, unsigned int *compLens, char *uri);


/**
 * @brief Take a URI and transform it into a CCNL_Prefix datastructure
 *
 * @param[in] URI       The uri that should be transformed, components separated by '/'
 * @param[in] suite     Packet format of the prefix, that should be created
 * @param[in] chunknum  If prefix is part of a chunked data file, this contains the chunknumber
 *
 * @return The prefix datastruct that was created
*/
struct ccnl_prefix_s *
ccnl_URItoPrefix(char* uri, int suite, uint32_t *chunknum);

/**
 * @brief Transforms a URI to a list of strings 
 *
 * @param[out] compVector       Resulting list of strings, without '\0' at the end      
 * @param[in] compLens          length of the single components in compVector
 * @param[in] uri               The uri that should be transformed, components separated by '/'
 *
 * @return number of components that where added to compVector
*/
uint32_t
ccnl_URItoComponents(char **compVector, size_t *compLens, char *uri);

#ifndef CCNL_LINUXKERNEL
/**
 * @brief Transforms a Prefix into a URI, separated by '/'
 *
 * @param[in]  pr                   The prefix that should be transformed
 * @param[in]  ccntlv_skip          Flag to enable skipping of CCNL_TLV fields
 * @param[in]  escape_components    Flag to enable escaping components
 * @param[in]  call_slash           Flag to enable call_slash
 * @param[out] buf                  buffer to write the URI to
 * @param[in]  buf_len              length of buffer @p buf
 *
 * @return the created URI @p buf
*/
char *
ccnl_prefix_to_str_detailed(struct ccnl_prefix_s *pr, int ccntlv_skip, int escape_components, int call_slash,
                            char *buf, size_t buflen);

/**
 * @brief Transforms a Prefix into a URI, separated by '/'
 *
 * @note A buffer is allocated via `malloc` and passed to
 *       @ref ccnl_prefix_to_str_detailed
 *
 * @param[in] pr                The prefix that should be transformed   
 * @param[in] ccntlv_skip       Flag to enable skipping of CCNL_TLV fields
 * @param[in] escape_components Flag to enable escaping components
 * @param[in] call_slash Flag to enable call_slash
 *
 * @return the created URI
*/
   char* ccnl_prefix_to_path_detailed(struct ccnl_prefix_s *pr, int ccntlv_skip, int escape_components, int call_slash);
#  define ccnl_prefix_to_path(P) ccnl_prefix_to_path_detailed(P, 1, 0, 0)
#  define ccnl_prefix_to_str(P, buf, buflen) ccnl_prefix_to_str_detailed(P, 1, 0, 0, buf, buflen)
#else
/**
 * @brief Transforms a Prefix into a URI, separated bei '/' 
 *
 * @param[in] pr       The prefix that should be transformed      
 *
 * @return the created URI
*/
    char* ccnl_prefix_to_str(struct ccnl_prefix_s *pr, char* buf,size_t buflen);
   char* ccnl_prefix_to_path(struct ccnl_prefix_s *pr);
#endif

/**
 * @brief Transforms a Prefix into a URI, separated bei '/', that contains debug information 
 *
 * @param[in] p       The prefix that should be transformed      
 *
 * @return the created URI containing debug information
*/
char*
ccnl_prefix_debug_info(struct ccnl_prefix_s *p);

#endif //EOF
/** @} */

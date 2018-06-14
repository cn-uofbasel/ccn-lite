/**
 * @addtogroup CCNL-utils
 * @{
 *
 * @file ccnl-crypto.h
 * @brief Crypto functions for CCN-lite utilities
 *
 * Copyright (C) 2013-2018, Christian Tschudin, University of Basel
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
#ifndef CCNL_CRYPTO_H
#define CCNL_CRYPTO_H

#ifdef USE_SIGNATURES
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>
#endif

int sha(void* input, unsigned long length, unsigned char* md);

int sign(char* private_key_path, unsigned char *msg, int msg_len,
     unsigned char *sig, unsigned int *sig_len);

int
verify(char* public_key_path, unsigned char *msg, int msg_len,
       unsigned char *sig, unsigned int sig_len);

int
add_signature(unsigned char *out, char *private_key_path,
              unsigned char *file, unsigned int fsize);

#endif 
/** @} */

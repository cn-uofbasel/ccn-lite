/**
 * @file test_content.c
 * @brief Tests for content functions
 *
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
 */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
 
#include "ccnl-pkt.h"
#include "ccnl-malloc.h"
#include "ccnl-content.h"

void test_ccnl_content_new_invalid()
{
    struct ccnl_content_s *content = ccnl_content_new(NULL);
    assert_null(content);
}

void test_ccnl_content_free_invalid() 
{
    int result = ccnl_content_free(NULL); 
    assert_int_equal(result, -1);
}

void test_ccnl_content_new_valid() 
{
    struct ccnl_pkt_s *packet = ccnl_malloc(sizeof(struct ccnl_pkt_s));
    struct ccnl_content_s *content = ccnl_content_new(&packet);

    assert_non_null(content);
    assert_true(content->last_used != 1);
    assert_int_equal(content->flags, CCNL_CONTENT_FLAGS_NOT_STALE);

    content->pkt = NULL;
    ccnl_pkt_free(packet);

    int result = ccnl_content_free(content);
    assert_int_equal(result, 0);
}

void test_ccnl_content_free_valid() 
{
    struct ccnl_pkt_s *packet = ccnl_malloc(sizeof(struct ccnl_pkt_s));
    struct ccnl_content_s *content = ccnl_content_new(&packet);
    
    assert_non_null(content);

    content->pkt = NULL;
    ccnl_pkt_free(packet);
    
    int result = ccnl_content_free(content); 
    assert_int_equal(result, 0);
}

int main(void)
{
    const UnitTest tests[] = {
        unit_test(test_ccnl_content_new_invalid),
        unit_test(test_ccnl_content_new_valid),
        unit_test(test_ccnl_content_free_invalid),
        unit_test(test_ccnl_content_free_valid),
    };
    
    return run_tests(tests);
}

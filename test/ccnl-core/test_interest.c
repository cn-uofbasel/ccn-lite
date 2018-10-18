/**
 * @file test-interest.c
 * @brief CCN lite - Tests for interest functions
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
 
#include "ccnl-interest.h"


void test_ccnl_interest_append_pending_invalid_parameters()
{
    int result = ccnl_interest_append_pending(NULL, NULL);
    assert_int_equal(result, -1); 

    struct ccnl_interest_s interest;
    result = ccnl_interest_append_pending(&interest, NULL);
    assert_int_equal(result, -2); 
}

void test_ccnl_interest_is_same_invalid_parameters() 
{
    int result = ccnl_interest_isSame(NULL, NULL);
    assert_int_equal(result, -1); 

    struct ccnl_interest_s interest;
    result = ccnl_interest_isSame(&interest, NULL);
    assert_int_equal(result, -2); 
}

void test_ccnl_interest_remove_pending_invalid_parameters()
{
    int result = ccnl_interest_remove_pending(NULL, NULL);
    assert_int_equal(result, -1); 

    struct ccnl_interest_s interest;
    result = ccnl_interest_remove_pending(&interest, NULL);
    assert_int_equal(result, -2); 
}

void test1()
{
  int result = 0;
  assert_int_equal(result, 0);
}
 
int main(void)
{
  const UnitTest tests[] = {
    unit_test(test1),
    unit_test(test_ccnl_interest_is_same_invalid_parameters),
    unit_test(test_ccnl_interest_remove_pending_invalid_parameters),
    unit_test(test_ccnl_interest_append_pending_invalid_parameters),
  };
 
  return run_tests(tests);
}

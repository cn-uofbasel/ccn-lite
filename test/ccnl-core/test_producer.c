/**
 * @file test-producer.c
 * @brief CCN lite - Tests for the local producer functions
 *
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

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
 
#include "ccnl-producer.h"

int _test_local_producer(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt);

int _test_local_producer(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt)
{
    /* we don't care about the parameters */
    (void)relay;
    (void)from;
    (void)pkt;

    /* we don't really care about creating content on demand for test purposes */
    return 1;
}

void test_local_producer_is_set()
{
    /* parameters shouldn't matter in this example */
    int result = local_producer(NULL, NULL, NULL);
    /* function returns 0 if no producer function has been set */
    assert_int_equal(result, 0);

    /* set producer function */
    ccnl_set_local_producer(&_test_local_producer);
    /* call the function again */
    result = local_producer(NULL, NULL, NULL);
    /* should return 1 instead */
    assert_int_equal(result, 1);

    /* unset function */
    ccnl_set_local_producer(NULL);
}

void test_local_producer_is_not_set()
{
    /* again, parameters shouldn't matter in this example */
    int result = local_producer(NULL, NULL, NULL);
    /* function returns 0 if no producer function has been set */
    assert_int_equal(result, 0);
}

int main(void)
{
    const UnitTest tests[] = {
        unit_test(test_local_producer_is_set),
        unit_test(test_local_producer_is_not_set),
    };
    
    return run_tests(tests);
}

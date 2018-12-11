/**
 * @file test-sockunion.c
 * @brief Tests for sockunion related functions
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

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
 
#include "ccnl-sockunion.h"

void test_ll2ascii_invalid()
{
    char *result = ll2ascii(NULL, 1);
    assert_true(result == NULL);
}

void test_ll2ascii_valid()
{
    const char *address = "ffffffffffff";
    char *result = ll2ascii((unsigned char*)address, 6);
    assert_true(result != NULL);

    /**
     * The test will actually fail at the above "assert_true" statement if 'result'
     * is null - however, the static code analyzer of LLVM does not recognize this
     * behavior, hence, the actual check if 'result' is not NULL
     */
    if (result) { 
        // shouldn't it be ff:ff:ff:ff:ff:ff? 
         assert_true(memcmp("66:66:66:66:66:66", result, 17) == 0);
    }
}

void test_half_byte_to_char() 
{
    uint8_t half_byte = 9;
    char result = _half_byte_to_char(half_byte);
    assert_true(result == '9');

    half_byte = 11;
    result = _half_byte_to_char(half_byte);
    assert_true(result == ('a' + 1));
}
    
void test_ccnl_is_local_addr_invalid() 
{
    int result = ccnl_is_local_addr(NULL);
    assert_int_equal(result, 0);
}

void test_ccnl_is_local_addr_valid() 
{
    sockunion socket;
    socket.sa.sa_family = AF_UNIX;
    int result = ccnl_is_local_addr(&socket);
    assert_int_equal(result, -1);

    /** test for socket type ipv4 */
    socket.sa.sa_family = AF_INET;
    socket.ip4.sin_addr.s_addr = htonl(0x7f000001);
    result = ccnl_is_local_addr(&socket);
    assert_int_equal(result, 1);

    /** test for socket type ipv6 with given ipv4 loopback */
    socket.sa.sa_family = AF_INET6;
    unsigned char ipv4_loopback[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1 };
    memcpy((socket.ip6.sin6_addr.s6_addr), ipv4_loopback, 16);
    result = ccnl_is_local_addr(&socket);
    assert_int_equal(result, 1);
    unsigned char ipv6_loopback[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    memcpy(socket.ip6.sin6_addr.s6_addr, ipv6_loopback, 16);
    /** test for socket type ipv6 with ipv6 loopback */
    result = ccnl_is_local_addr(&socket);
    assert_int_equal(result, 1);
}

void test_ccnl_addr2ascii_invalid()
{
    char *result = ccnl_addr2ascii(NULL);
    assert_string_equal(result, "(local)");
}

int main(void)
{
    const UnitTest tests[] = {
        unit_test(test_half_byte_to_char),
        unit_test(test_ll2ascii_invalid),
        unit_test(test_ll2ascii_valid),
        unit_test(test_ccnl_is_local_addr_invalid),
        unit_test(test_ccnl_is_local_addr_valid),
        unit_test(test_ccnl_addr2ascii_invalid),
    };
    
    return run_tests(tests);
}


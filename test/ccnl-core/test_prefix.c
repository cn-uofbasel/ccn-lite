#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
 
#include "ccnl-core.h"
 
void test_prefix_to_path()
{
    char *result = "/path/to/data";
    struct ccnl_prefix_s *p = ccnl_malloc(sizeof(struct ccnl_prefix_s));
    p->compcnt = 3;
    p->comp = ccnl_malloc(sizeof(char*) * p->compcnt);
    p->complen = ccnl_malloc(sizeof(int) * p->compcnt);
    p->comp[0] = (unsigned char*)"path";
    p->complen[0] = 4;
    p->comp[1] = (unsigned char*)"to";
    p->complen[1] = 2;
    p->comp[2] = (unsigned char*)"data";
    p->complen[2] = 4;
    assert_string_equal(result, ccnl_prefix_to_path(p));
    ccnl_free(p->comp);
    ccnl_free(p->complen);
    ccnl_free(p);
}

void test_uri_to_prefix(){
    int uri_to_prefix_suite = 0;
    char *cmpstr = ccnl_malloc(100);
    char *c = ccnl_malloc(100);
    memset(cmpstr, 0, 100);
    memset(c, 0, 100);
    strcpy(cmpstr, "/path/to/data");
    strcpy(c, "/path/to/data");
    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix(c, uri_to_prefix_suite, NULL);
    assert_string_equal(cmpstr, ccnl_prefix_to_path(prefix));
    ccnl_prefix_free(prefix);
}

void test_append_to_prefix(){
    int uri_to_prefix_suite = 0;
    char *cmpstr = ccnl_malloc(100);
    char *c = ccnl_malloc(100);
    strcpy(cmpstr, "/path/to/data/cmp");
    strcpy(c, "/path/to/data");
    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix(c, uri_to_prefix_suite, NULL);
    char *component = ccnl_malloc(100);
    strcpy(component, "cmp");
    ccnl_prefix_appendCmp(prefix, (unsigned char*) component, 3);
    assert_string_equal(cmpstr, ccnl_prefix_to_path(prefix));

    ccnl_prefix_free(prefix);
    ccnl_free(c);
    ccnl_free(cmpstr);
}	

void test_prefix_exact_match()
{
    int prefix_cmp_suite = 0;
    char *c1 = ccnl_malloc(100);
    strcpy(c1, "/path/to/data");
    struct ccnl_prefix_s *p1 = ccnl_URItoPrefix(c1, prefix_cmp_suite, NULL);

    char *c2 = ccnl_malloc(100);
    strcpy(c2, "/path/to/data");
    struct ccnl_prefix_s *p2 = ccnl_URItoPrefix(c2, prefix_cmp_suite, NULL);
    int res = ccnl_prefix_cmp(p1, 0, p2, CMP_EXACT);

    ccnl_prefix_free(p1);
    ccnl_prefix_free(p2);

    assert_int_equal(0, res);
}

void test_prefix_no_exact_match()
{
    int prefix_cmp_suite = 0;
    char *c1 = ccnl_malloc(100);
    strcpy(c1, "/path/to/data");
    struct ccnl_prefix_s *p1 = ccnl_URItoPrefix(c1, prefix_cmp_suite, NULL);

    char *c2 = ccnl_malloc(100);
    strcpy(c2, "/path/to/data1");
    struct ccnl_prefix_s *p2 = ccnl_URItoPrefix(c2, prefix_cmp_suite, NULL);
    int res = ccnl_prefix_cmp(p1, 0, p2, CMP_EXACT);

    ccnl_prefix_free(p1);
    ccnl_prefix_free(p2);

    assert_int_equal(-1, res);
}

void test_prefix_longest_match()
{
    int prefix_cmp_suite = 0;
    char *c1 = ccnl_malloc(100);
    strcpy(c1, "/path/to/data");
    struct ccnl_prefix_s *p1 = ccnl_URItoPrefix(c1, prefix_cmp_suite, NULL);

    char *c2 = ccnl_malloc(100);
    strcpy(c2, "/path/to/data/files");
    struct ccnl_prefix_s *p2 = ccnl_URItoPrefix(c2, prefix_cmp_suite, NULL);
    int res = ccnl_prefix_cmp(p1, 0, p2, CMP_LONGEST);

    ccnl_prefix_free(p1);
    ccnl_prefix_free(p2);

    assert_int_equal(3, res);
}

void test_prefix_no_longest_match()
{
    int prefix_cmp_suite = 0;
    char *c1 = ccnl_malloc(100);
    strcpy(c1, "/path2/to/data");
    struct ccnl_prefix_s *p1 = ccnl_URItoPrefix(c1, prefix_cmp_suite, NULL);

    char *c2 = ccnl_malloc(100);
    strcpy(c2, "/path/to/data/files");
    struct ccnl_prefix_s *p2 = ccnl_URItoPrefix(c2, prefix_cmp_suite, NULL);
    int res = ccnl_prefix_cmp(p1, 0, p2, CMP_LONGEST);
    ccnl_prefix_free(p1);
    ccnl_prefix_free(p2);

    assert_int_equal(0, res);
}

int main(void)
{
  const UnitTest tests[] = {
    unit_test(test_prefix_to_path),
    unit_test(test_uri_to_prefix),
    unit_test(test_append_to_prefix),
    unit_test(test_prefix_exact_match),
    unit_test(test_prefix_no_exact_match),
    unit_test(test_prefix_longest_match),
    unit_test(test_prefix_no_longest_match),
  };
 
  return run_tests(tests);
}

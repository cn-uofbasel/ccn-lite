#include "ccnl-unit.h"

#include "ccnl-core.h"

int prefix_cmp_suite;

struct ccnl_prefix_s *p1 = NULL;
char *str = NULL;
int uri_to_prefix_suite = 0;


int ccnl_test_prepare_uri_to_prefix(void **cmpstr, void **prefix){

    *cmpstr = ccnl_malloc(100);
    char *c = ccnl_malloc(100);
    strcpy(*cmpstr, "/path/to/data");
    strcpy(c, "/path/to/data");
    *prefix = ccnl_URItoPrefix(c, uri_to_prefix_suite, NULL, NULL);


    return 1;
}

int ccnl_test_run_uri_to_prefix(void *cmpstr, void *prefix){

    struct ccnl_prefix_s *p = prefix;

    int res = C_ASSERT_EQUAL_STRING(ccnl_prefix_to_path(p), cmpstr);
    if(p->suite != uri_to_prefix_suite) return 0;
    return res;
}

int ccnl_test_cleanup_uri_to_prefix(void *cmpstr, void *prefix){

    struct ccnl_prefix_s *p = prefix;
    ccnl_prefix_free(p);
    ccnl_free(cmpstr);
    return 1;
}

int ccnl_test_prepare_AppendCmpToPrefix(void **cmpstr, void **prefix){

    *cmpstr = ccnl_malloc(100);
    char *c = ccnl_malloc(100);
    strcpy(*cmpstr, "/path/to/data/cmp");
    strcpy(c, "/path/to/data");
    *prefix = ccnl_URItoPrefix(c, uri_to_prefix_suite, NULL, NULL);


    return 1;
}

int ccnl_test_run_AppendCmpToPrefix(void *cmpstr, void *prefix) {
    struct ccnl_prefix_s *p = prefix;
    char *component = ccnl_malloc(100);

    strcpy(component, "cmp");
    ccnl_prefix_appendCmp(p, (unsigned char*) component, 3);

    int res = C_ASSERT_EQUAL_STRING(ccnl_prefix_to_path(p), cmpstr);
    if(p->suite != uri_to_prefix_suite) return 0;

    return res;
}

//Run Tests
int main(){
    int res = 0;
    int testnum = 0;

    res = RUN_TEST(testnum, "testing add component to prefix", ccnl_test_prepare_AppendCmpToPrefix, ccnl_test_run_AppendCmpToPrefix, ccnl_test_cleanup_uri_to_prefix, str, p1);
    if(!res) return -1;

    return 0;

}

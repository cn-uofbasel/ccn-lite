#include "ccnl-unit.h"

#include "ccnl-core.h"

int prefix_cmp_suite;

struct ccnl_prefix_s *p1 = NULL;
char *str = NULL;
int uri_to_prefix_suite = 0;


int ccnl_test_prepare_prefix_to_path_1(void **prefix, void **out){
    *prefix = ccnl_malloc(sizeof(struct ccnl_prefix_s));
    struct ccnl_prefix_s *p = *prefix;
    p->compcnt = 3;
    p->comp = ccnl_malloc(sizeof(char*) * p->compcnt);
    p->complen = ccnl_malloc(sizeof(int) * p->compcnt);
    p->comp[0] = (unsigned char*)"path";
    p->complen[0] = 4;
    p->comp[1] = (unsigned char*)"to";
    p->complen[1] = 2;
    p->comp[2] = (unsigned char*)"data";
    p->complen[2] = 4;

    p->nfnflags = 0;

    *out = "/path/to/data";
    return 1;
}

int ccnl_test_prepare_prefix_to_path_2(void **prefix, void **out){
    *prefix = ccnl_malloc(sizeof(struct ccnl_prefix_s));
    struct ccnl_prefix_s *p = *prefix;
    p->compcnt = 4;
    p->comp = ccnl_malloc(sizeof(char*) * p->compcnt);
    p->complen = ccnl_malloc(sizeof(int) * p->compcnt);
    p->comp[0] = (unsigned char*)"path";
    p->complen[0] = 4;
    p->comp[1] = (unsigned char*)"to";
    p->complen[1] = 2;
    p->comp[2] = (unsigned char*)"data";
    p->complen[2] = 4;
    p->comp[3] = (unsigned char*)"(@x call 2 /fun/func x)";
    p->complen[3] = strlen((const char *)p->comp[3]);

    p->nfnflags = 0;
    p->nfnflags |= 0x01;

    *out = (unsigned char*)"nfn[/path/to/data/(@x call 2 /fun/func x)]";
    return 1;
}

int ccnl_test_run_prefix_to_path(void *prefix, void *out){
    struct ccnl_prefix_s *p = prefix;
    char *cmp = out;

    int res = C_ASSERT_EQUAL_STRING(ccnl_prefix_to_path(p), cmp);

    return res;

}

int ccnl_test_cleanup_prefix_to_path(void *prefix, void *out){

    (void)out;
    struct ccnl_prefix_s *p = prefix;
    ccnl_prefix_free(p);
    return 1;
}

//Run Tests
int main(){
    int res = 0;
    int testnum = 0;

    res = RUN_TEST(testnum, "testing prefix to path function", ccnl_test_prepare_prefix_to_path_1, ccnl_test_run_prefix_to_path, ccnl_test_cleanup_prefix_to_path, p1, str);
    if(!res) return -1;

    ++testnum;
    res = RUN_TEST(testnum, "testing prefix to path function for nfn", ccnl_test_prepare_prefix_to_path_2, ccnl_test_run_prefix_to_path, ccnl_test_cleanup_prefix_to_path, p1, str);
    if(!res) return -1;

    return 0;

}

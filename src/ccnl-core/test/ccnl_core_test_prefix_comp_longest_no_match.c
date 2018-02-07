
#include "ccnl-unit.h"

#include "ccnl-core.h"

int prefix_cmp_suite;

//longest mismatch
//---------------------------------------------------------------------------------------------------
int ccnl_test_prepare_prefix_cmp_longest_mismatch(void **prefix1, void **prefix2){

    char *c1 = ccnl_malloc(100);
    strcpy(c1, "/path2/to/data/files");
    *prefix1 = ccnl_URItoPrefix(c1, prefix_cmp_suite, NULL, NULL);

    char *c2 = ccnl_malloc(100);
    strcpy(c2, "/path/to/data");
    *prefix2 = ccnl_URItoPrefix(c2, prefix_cmp_suite, NULL, NULL);

    return 1;
}

int ccnl_test_run_prefix_cmp_longest_mismatch(void *prefix1, void *prefix2){

    struct ccnl_prefix_s *p1 = prefix1;
    struct ccnl_prefix_s *p2 = prefix2;

    int res = ccnl_prefix_cmp(p1, 0, p2, CMP_LONGEST);

    return res == 0;
}

int ccnl_test_cleanup_prefix_cmp(void *prefix1, void *prefix2){

    struct ccnl_prefix_s *p1 = prefix1;
    struct ccnl_prefix_s *p2 = prefix2;

    ccnl_prefix_free(p1);
    ccnl_prefix_free(p2);
    return 1;
}

//Run Tests
int main(){
    int testnum = 0;
    int res = 0;
    //Prefix CMP Tests for all suites
    char *testdescription = ccnl_malloc(512);
    struct ccnl_prefix_s *p1 = NULL, *p2 = NULL;
    for(prefix_cmp_suite = 0; prefix_cmp_suite < 3; ++prefix_cmp_suite) {

        //Test: PREFIX CMP LONGEST MISSMATCH
        ++testnum;
        sprintf(testdescription,
                "testing prefix cmp with a match of 0 components for longest prefix matching with suite %s",
                ccnl_suite2str(prefix_cmp_suite));
        res = RUN_TEST(testnum, testdescription, ccnl_test_prepare_prefix_cmp_longest_mismatch,
                       ccnl_test_run_prefix_cmp_longest_mismatch, ccnl_test_cleanup_prefix_cmp, p1, p2);


        if (!res) {
            return -1;
        }
    }
    return 0;
}

#define CCNL_UNIT_TEST


#include "test.h"

#include "unit.h"

#include "ccnl_unit_prefix_to_path.c"
#include "ccnl_unit_prefix_comp.c"


int main(int argc, char **argv){

	fprintf(stderr, "CCN-lite Unit Tests\n");

	int testnum = 0;
	//TEST 1: PREFIX TO PATH
	++testnum;
	struct ccn_prefix_s *p_t1 = 0;
	char *c_t1 = 0;
	RUN_TEST(testnum, "testing prefix to path function", ccnl_test_prepare_prefix_to_path_1, ccnl_test_run_prefix_to_path, ccnl_test_cleanup_prefix_to_path, p_t1, c_t1);

	//Test 2: PREFIX TO PATH NFN
	++testnum;
	struct ccn_prefix_s *p_t2 = 0;
	char *c_t2 = 0;
	RUN_TEST(testnum, "testing prefix to path function for nfn", ccnl_test_prepare_prefix_to_path_2, ccnl_test_run_prefix_to_path, ccnl_test_cleanup_prefix_to_path, p_t2, c_t2);

	char *testdescription = ccnl_malloc(512);
	for(prefix_cmp_suite = 0; prefix_cmp_suite < 3; ++prefix_cmp_suite){
		
		//Test 3: PREFIX CMP EXACT MATCH
		++testnum;
		sprintf(testdescription, "testing prefix cmp exact match with a match with suite %s", ccnl_suite2str(prefix_cmp_suite));
		struct ccn_prefix_s *p1_t3 = 0;
		struct ccn_prefix_s *p2_t3 = 0;
		RUN_TEST(testnum, testdescription, ccnl_test_prepare_prefix_cmp_exact_match, ccnl_test_run_prefix_cmp_exact_match, ccnl_test_cleanup_prefix_cmp_exact_match, p1_t3, p2_t3);

		//Test 4: PREFIX CMP EXACT MISMATCH
		++testnum;
		sprintf(testdescription, "testing prefix cmp exact match with a mismatch with suite %s", ccnl_suite2str(prefix_cmp_suite));
		struct ccn_prefix_s *p1_t4 = 0;
		struct ccn_prefix_s *p2_t4 = 0; 
		RUN_TEST(testnum, testdescription , ccnl_test_prepare_prefix_cmp_exact_mismatch, ccnl_test_run_prefix_cmp_exact_mismatch, ccnl_test_cleanup_prefix_cmp_exact_mismatch, p1_t4, p2_t4);

		//Test 5: PREFIX CMP LONGEST MATCH
		++testnum;
		sprintf(testdescription, "testing prefix cmp with a match of 3 components for longest prefix matching with suite %s", ccnl_suite2str(prefix_cmp_suite));
		struct ccn_prefix_s *p1_t5 = 0;
		struct ccn_prefix_s *p2_t5 = 0; 
		RUN_TEST(testnum, testdescription, ccnl_test_prepare_prefix_cmp_longest_match, ccnl_test_run_prefix_cmp_longest_match, ccnl_test_cleanup_prefix_cmp_longest_match, p1_t5, p2_t5);

		//Test 6: PREFIX CMP LONGEST MISSMATCH
		++testnum;
		sprintf(testdescription, "testing prefix cmp with a match of 0 components for longest prefix matching with suite %s", ccnl_suite2str(prefix_cmp_suite));
		struct ccn_prefix_s *p1_t6 = 0;
		struct ccn_prefix_s *p2_t6 = 0; 
		RUN_TEST(testnum, testdescription, ccnl_test_prepare_prefix_cmp_longest_mismatch, ccnl_test_run_prefix_cmp_longest_mismatch, ccnl_test_cleanup_prefix_cmp_longest_mismatch, p1_t6, p2_t6);
	}
}


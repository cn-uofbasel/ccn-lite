#define CCNL_UNIT_TEST


#include "test.h"

#include "unit.h"

#include "ccnl_unit_prefix_to_path.c"
#include "ccnl_unit_uri_2_prefix.c"
#include "ccnl_unit_prefix_comp.c"
#include "ccnl_unit_stack_type_const.c"

int main(int argc, char **argv){

	struct ccnl_prefix_s *p1 = NULL, *p2 = NULL;
	char *str = NULL;
	struct const_s *con1 = NULL, *con2 = NULL;

	fprintf(stderr, "CCN-lite Unit Tests\n");

	int testnum = 0;
	//TEST: PREFIX TO PATH
	++testnum;
	RUN_TEST(testnum, "testing prefix to path function", ccnl_test_prepare_prefix_to_path_1, ccnl_test_run_prefix_to_path, ccnl_test_cleanup_prefix_to_path, p1, str);

	//Test: PREFIX TO PATH NFN
	++testnum;
	RUN_TEST(testnum, "testing prefix to path function for nfn", ccnl_test_prepare_prefix_to_path_2, ccnl_test_run_prefix_to_path, ccnl_test_cleanup_prefix_to_path, p1, str);

	//Test: uri_to_prefix
	++testnum;
	RUN_TEST(testnum, "testing uri to prefix", ccnl_test_prepare_uri_to_prefix, ccnl_test_run_uri_to_prefix, ccnl_test_cleanup_uri_to_prefix, str, p1);

	//Test: uri_to_prefix NFN
	++testnum;
	RUN_TEST(testnum, "testing uri to prefix for nfn", ccnl_test_prepare_uri_to_prefix_nfn, ccnl_test_run_uri_to_prefix, ccnl_test_cleanup_uri_to_prefix, str, p1);

	//Test: uri_to_prefix chunk
	++testnum;
	RUN_TEST(testnum, "testing uri to prefix with chunk", ccnl_test_prepare_uri_to_prefix_chunk, ccnl_test_run_uri_to_prefix, ccnl_test_cleanup_uri_to_prefix, str, p1);

	++testnum;
	RUN_TEST(testnum, "testing add component to prefix", ccnl_test_prepare_AppendCmpToPrefix, ccnl_test_run_AppendCmpToPrefix, ccnl_test_cleanup_uri_to_prefix, str, p1);

	//Prefix CMP Tests for all suites
	char *testdescription = ccnl_malloc(512);
	for(prefix_cmp_suite = 0; prefix_cmp_suite < 3; ++prefix_cmp_suite){
		
		//Test: PREFIX CMP EXACT MATCH
		++testnum;
		sprintf(testdescription, "testing prefix cmp exact match with a match with suite %s", ccnl_suite2str(prefix_cmp_suite));
		RUN_TEST(testnum, testdescription, ccnl_test_prepare_prefix_cmp_exact_match, ccnl_test_run_prefix_cmp_exact_match, ccnl_test_cleanup_prefix_cmp, p1, p2);

		//Test: PREFIX CMP EXACT MISMATCH
		++testnum;
		sprintf(testdescription, "testing prefix cmp exact match with a mismatch with suite %s", ccnl_suite2str(prefix_cmp_suite));
		RUN_TEST(testnum, testdescription , ccnl_test_prepare_prefix_cmp_exact_mismatch, ccnl_test_run_prefix_cmp_exact_mismatch, ccnl_test_cleanup_prefix_cmp, p1, p2);

		//Test: PREFIX CMP LONGEST MATCH
		++testnum;
		sprintf(testdescription, "testing prefix cmp with a match of 3 components for longest prefix matching with suite %s", ccnl_suite2str(prefix_cmp_suite));
		RUN_TEST(testnum, testdescription, ccnl_test_prepare_prefix_cmp_longest_match, ccnl_test_run_prefix_cmp_longest_match, ccnl_test_cleanup_prefix_cmp, p1, p2);

		//Test: PREFIX CMP LONGEST MISSMATCH
		++testnum;
		sprintf(testdescription, "testing prefix cmp with a match of 0 components for longest prefix matching with suite %s", ccnl_suite2str(prefix_cmp_suite));
		RUN_TEST(testnum, testdescription, ccnl_test_prepare_prefix_cmp_longest_mismatch, ccnl_test_run_prefix_cmp_longest_mismatch, ccnl_test_cleanup_prefix_cmp, p1, p2);

		//Test: PREFIX CMP LONGEST MATCH
		++testnum;
		sprintf(testdescription, "testing prefix cmp with a match components for match with suite %s", ccnl_suite2str(prefix_cmp_suite));
		RUN_TEST(testnum, testdescription, ccnl_test_prepare_prefix_cmp_match, ccnl_test_run_prefix_cmp_match, ccnl_test_cleanup_prefix_cmp, p1, p2);
	}
	ccnl_free(testdescription);

	//Test: prefix type const: str2const
	++testnum;
	RUN_TEST(testnum, "Testing stack type str2const", ccnl_test_prepare_stack_type_const_str2const, ccnl_test_run_stack_type_const_str2const, ccnl_test_cleanup_stack_type_const_str2const, con1, con2);
	
	
	//Test: prefix type const: const2str
	++testnum;
	RUN_TEST(testnum, "Testing stack type const2str", ccnl_test_prepare_stack_type_const_const2str, ccnl_test_run_stack_type_const_const2str, ccnl_test_cleanup_stack_type_const_const2str, str, con2);

	return 0;
}


#include "test.h"
#include "../../src/ccnl-headers.h"


int prefix_cmp_suite;

//exact match
//---------------------------------------------------------------------------------------------------
int ccnl_test_prepare_prefix_cmp_exact_match(void **prefix1, void **prefix2){

	char *c1 = ccnl_malloc(100);
	strcpy(c1, "/path/to/data");
	*prefix1 = ccnl_URItoPrefix(c1, prefix_cmp_suite, NULL, NULL);
	
	char *c2 = ccnl_malloc(100);
	strcpy(c2, "/path/to/data");
	*prefix2 = ccnl_URItoPrefix(c2, prefix_cmp_suite, NULL, NULL);

	return 1;
}

int ccnl_test_run_prefix_cmp_exact_match(void *prefix1, void *prefix2){

	struct ccnl_prefix_s *p1 = prefix1;
	struct ccnl_prefix_s *p2 = prefix2;

	int res = ccnl_prefix_cmp(p1, 0, p2, CMP_EXACT);
	//NULL if match
	return !res;
}

int ccnl_test_cleanup_prefix_cmp(void *prefix1, void *prefix2){

	struct ccnl_prefix_s *p1 = prefix1;
	struct ccnl_prefix_s *p2 = prefix2;

	free_prefix(p1);
	free_prefix(p2);
	return 1;
	return 1;
}


//exact mismatch
//---------------------------------------------------------------------------------------------------
int ccnl_test_prepare_prefix_cmp_exact_mismatch(void **prefix1, void **prefix2){

	char *c1 = ccnl_malloc(100);
	strcpy(c1, "/path/to/data");
	*prefix1 = ccnl_URItoPrefix(c1, prefix_cmp_suite, NULL, NULL);
	
	char *c2 = ccnl_malloc(100);
	strcpy(c2, "/path/to/data1");
	*prefix2 = ccnl_URItoPrefix(c2, prefix_cmp_suite, NULL, NULL);

	return 1;
}

int ccnl_test_run_prefix_cmp_exact_mismatch(void *prefix1, void *prefix2){

	struct ccnl_prefix_s *p1 = prefix1;
	struct ccnl_prefix_s *p2 = prefix2;

	int res = ccnl_prefix_cmp(p1, 0, p2, CMP_EXACT);
	//NULL if match
	return res;
}

//longest match
//---------------------------------------------------------------------------------------------------
int ccnl_test_prepare_prefix_cmp_longest_match(void **prefix1, void **prefix2){

	char *c1 = ccnl_malloc(100);
	strcpy(c1, "/path/to/data");
	*prefix1 = ccnl_URItoPrefix(c1, prefix_cmp_suite, NULL, NULL);
	
	char *c2 = ccnl_malloc(100);
	strcpy(c2, "/path/to/data/files");
	*prefix2 = ccnl_URItoPrefix(c2, prefix_cmp_suite, NULL, NULL);

	return 1;
}

int ccnl_test_run_prefix_cmp_longest_match(void *prefix1, void *prefix2){

	struct ccnl_prefix_s *p1 = prefix1;
	struct ccnl_prefix_s *p2 = prefix2;

	int res = ccnl_prefix_cmp(p1, 0, p2, CMP_LONGEST);

	return res == 3;
}

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

//match
//---------------------------------------------------------------------------------------------------
int ccnl_test_prepare_prefix_cmp_match(void **prefix1, void **prefix2){

	char *c1 = ccnl_malloc(100);
	strcpy(c1, "/path2/to/data");
	*prefix1 = ccnl_URItoPrefix(c1, prefix_cmp_suite, NULL, NULL);
	
	char *c2 = ccnl_malloc(100);
	strcpy(c2, "/path/to/data/files");
	*prefix2 = ccnl_URItoPrefix(c2, prefix_cmp_suite, NULL, NULL);

	return 1;
}

int ccnl_test_run_prefix_cmp_match(void *prefix1, void *prefix2){

	struct ccnl_prefix_s *p1 = prefix1;
	struct ccnl_prefix_s *p2 = prefix2;

	int res = ccnl_prefix_cmp(p1, 0, p2, CMP_MATCH);

	return res == 0;
}
//#undef CCNL_EXT_DEBUG
//#undef USE_DEBUG_MALLOC

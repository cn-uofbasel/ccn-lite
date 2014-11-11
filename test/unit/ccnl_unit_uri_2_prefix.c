#include "test.h"
#include "../../ccnl-headers.h"


int uri_to_prefix_suite = 0;

//---------------------------------------------------------------------------------------------------
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
	//NULL if match
	if(p->suite != uri_to_prefix_suite) return 0;
	return res;
}

int ccnl_test_cleanup_uri_to_prefix(void *prefix1, void *prefix2){

	return 1;
}

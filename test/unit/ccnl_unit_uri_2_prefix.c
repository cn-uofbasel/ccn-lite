#include "test.h"
#include "../../src/ccnl-headers.h"


int uri_to_prefix_suite = 0;

//---------------------------------------------------------------------------------------------------
//Without NFN
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
	free_prefix(p);
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


//---------------------------------------------------------------------------------------------------
//With NFN
int ccnl_test_prepare_uri_to_prefix_nfn(void **cmpstr, void **prefix){

	*cmpstr = ccnl_malloc(100);
	char *c1 = ccnl_malloc(100);
	char *c2 = ccnl_malloc(100);
	strcpy(*cmpstr, "nfn[/path/to/data/(@x call 2 /func/fun1 x)]");
	strcpy(c1, "/path/to/data");
	strcpy(c2, "(@x call 2 /func/fun1 x)");
	*prefix = ccnl_URItoPrefix(c1, uri_to_prefix_suite, c2, NULL);

	return 1;
}

//---------------------------------------------------------------------------------------------------
//With NFN
int ccnl_test_prepare_uri_to_prefix_chunk(void **cmpstr, void **prefix){

	*cmpstr = ccnl_malloc(100);
	char *c1 = ccnl_malloc(100);
	char *c2 = ccnl_malloc(100);
	unsigned int *chunk = ccnl_malloc(sizeof(int));
	*chunk = 1;
	strcpy(*cmpstr, "/path/to/data");
	strcpy(c1, "/path/to/data");
	strcpy(c2, "(@x call 2 /func/fun1 x)");
	*prefix = ccnl_URItoPrefix(c1, uri_to_prefix_suite, NULL, chunk);

//	printf("%s\n", ccnl_prefix_to_path(*prefix));
	return 1;
}

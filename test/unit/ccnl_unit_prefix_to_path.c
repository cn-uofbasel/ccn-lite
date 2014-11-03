#include "test.h"


int ccnl_test_prepare_prefix_to_path_1(void **prefix, void **out){
	*prefix = ccnl_malloc(sizeof(struct ccnl_prefix_s));
	struct ccnl_prefix_s *p = *prefix;
	p->compcnt = 3;
	p->comp = ccnl_malloc(sizeof(char*) * p->compcnt);
	p->complen = ccnl_malloc(sizeof(int) * p->compcnt);
	p->comp[0] = "path";
	p->complen[0] = 4;
	p->comp[1] = "to";
	p->complen[1] = 2;
	p->comp[2] = "data";
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
	p->comp[0] = "path";
	p->complen[0] = 4;
	p->comp[1] = "to";
	p->complen[1] = 2;
	p->comp[2] = "data";
	p->complen[2] = 4;
	p->comp[3] = "(@x call 2 /fun/func x)";
	p->complen[3] = strlen(p->comp[3]);

	p->nfnflags = 0;
	p->nfnflags |= 0x01;


	*out = "nfn[/path/to/data/(@x call 2 /fun/func x)]";
	return 1;
}

int ccnl_test_run_prefix_to_path(void *prefix, void *out){
	struct ccnl_prefix_s *p = prefix;
	char *cmp = out;

	int res = C_ASSERT_EQUAL_STRING(ccnl_prefix_to_path(p), cmp);

	return res;

}

int ccnl_test_cleanup_prefix_to_path(void *prefix, void *out){

	return 1;
}

//#undef CCNL_EXT_DEBUG
//#undef USE_DEBUG_MALLOC

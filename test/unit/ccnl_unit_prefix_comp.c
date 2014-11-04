#include "test.h"
#include "../../ccnl-headers.h"

//exact match
//---------------------------------------------------------------------------------------------------
int ccnl_test_prepare_prefix_cmp_exact_match(void **prefix1, void **prefix2){

	*prefix1 = ccnl_malloc(sizeof(struct ccnl_prefix_s));
	*prefix2 = ccnl_malloc(sizeof(struct ccnl_prefix_s));
	struct ccnl_prefix_s *p1 = *prefix1;
	struct ccnl_prefix_s *p2 = *prefix2;

	p1->compcnt = 3;
	p1->comp = ccnl_malloc(sizeof(char*) * p1->compcnt);
	p1->complen = ccnl_malloc(sizeof(int) * p1->compcnt);
	p1->comp[0] = (unsigned char*)"path";
	p1->complen[0] = 4;
	p1->comp[1] = (unsigned char*)"to";
	p1->complen[1] = 2;
	p1->comp[2] = (unsigned char*)"data";
	p1->complen[2] = 4;
	p1->nfnflags = 0;

	p2->compcnt = 3;
	p2->comp = ccnl_malloc(sizeof(char*) * p2->compcnt);
	p2->complen = ccnl_malloc(sizeof(int) * p2->compcnt);
	p2->comp[0] = (unsigned char*)"path";
	p2->complen[0] = 4;
	p2->comp[1] = (unsigned char*)"to";
	p2->complen[1] = 2;
	p2->comp[2] = (unsigned char*)"data";
	p2->complen[2] = 4;
	p2->nfnflags = 0;
	return 1;
}

int ccnl_test_run_prefix_cmp_exact_match(void *prefix1, void *prefix2){

	struct ccnl_prefix_s *p1 = prefix1;
	struct ccnl_prefix_s *p2 = prefix2;

	int res = ccnl_prefix_cmp(p1, 0, p2, CMP_EXACT);
	//NULL if match
	return !res;
}

int ccnl_test_cleanup_prefix_cmp_exact_match(void *prefix1, void *prefix2){

	return 1;
}


//exact mismatch
//---------------------------------------------------------------------------------------------------
int ccnl_test_prepare_prefix_cmp_exact_mismatch(void **prefix1, void **prefix2){

	*prefix1 = ccnl_malloc(sizeof(struct ccnl_prefix_s));
	*prefix2 = ccnl_malloc(sizeof(struct ccnl_prefix_s));
	struct ccnl_prefix_s *p1 = *prefix1;
	struct ccnl_prefix_s *p2 = *prefix2;

	p1->compcnt = 3;
	p1->comp = ccnl_malloc(sizeof(char*) * p1->compcnt);
	p1->complen = ccnl_malloc(sizeof(int) * p1->compcnt);
	p1->comp[0] = (unsigned char*)"path";
	p1->complen[0] = 4;
	p1->comp[1] = (unsigned char*)"to";
	p1->complen[1] = 2;
	p1->comp[2] = (unsigned char*)"data";
	p1->complen[2] = 4;
	p1->nfnflags = 0;

	p2->compcnt = 3;
	p2->comp = ccnl_malloc(sizeof(char*) * p2->compcnt);
	p2->complen = ccnl_malloc(sizeof(int) * p2->compcnt);
	p2->comp[0] = (unsigned char*)"path";
	p2->complen[0] = 4;
	p2->comp[1] = (unsigned char*)"to";
	p2->complen[1] = 2;
	p2->comp[2] = (unsigned char*)"data1";
	p2->complen[2] = 5;
	p2->nfnflags = 0;
	return 1;
}

int ccnl_test_run_prefix_cmp_exact_mismatch(void *prefix1, void *prefix2){

	struct ccnl_prefix_s *p1 = prefix1;
	struct ccnl_prefix_s *p2 = prefix2;

	int res = ccnl_prefix_cmp(p1, 0, p2, CMP_EXACT);
	//NULL if match
	return res;
}

int ccnl_test_cleanup_prefix_cmp_exact_mismatch(void *prefix1, void *prefix2){

	return 1;
}
//
//#undef CCNL_EXT_DEBUG
//#undef USE_DEBUG_MALLOC

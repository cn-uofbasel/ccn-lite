#ifndef CCNL_UNIT_H
#define CCNL_UNIT_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


int C_ASSERT_EQUAL_INT(int a, int b){
    return a == b ? 1 : 0;
}

int C_ASSERT_EQUAL_STRING(char *a, char *b){
    return !strcmp(a,b);
}

int C_ASSERT_EQUAL_STRING_N(char *a, char *b, int len){
    return !strncmp(a,b,len);
}
// COL:  0 = defalut, 1 = green, -1 = red
#define TESTMSG(LVL, COL, ...) do{ \
	if(COL == 0){\
		fprintf(stderr, "\033[0m");\
	}\
	else if(COL == 1){\
		fprintf(stderr, "\033[0;32m");\
	}\
	else if(COL == -1){\
		fprintf(stderr, "\033[0;31m");\
	}\
	fprintf(stderr, "Test[%d]: ", LVL);\
	fprintf(stderr, __VA_ARGS__);\
	fprintf(stderr, "\033[0m");\
    } while (0);

int RUN_TEST(int testnum, char *description,
		int (*prepare)(void **testdata, void **comp), 
		int(*run)(void *testdata, void *comp), 
		int(*cleanup)(void *testdata, void *comp), 
		void *testdata, void *comp){

	TESTMSG(testnum, 0, "%s \n", description);
	int ret = 0;
	ret = prepare(&testdata, &comp);
	if(!ret){
		TESTMSG(testnum, -1, "Error while preparing test\n");
		return 0;
	}
	ret = run(testdata, comp);
	if(!ret){
		TESTMSG(testnum, -1, "Error while running test\n");
		return 0;
	}
	ret = cleanup(testdata, comp);
	if(!ret){
		TESTMSG(testnum, -1, "Error while cleaning up test\n");
		return 0;
	}
	TESTMSG(testnum, 1, "SUCCESSFUL\n");
	return 1;
}

#endif // CCNL_UNIT_H


#include "test.h"
#include "../../src/ccnl-headers.h"

int ccnl_test_prepare_stack_type_const_str2const(void **con1, void **con2){

	struct const_s *con;

	*con1 = ccnl_calloc(1, sizeof(struct const_s) + 4);

	con = *con1;

	strcpy(con->str, "data");
	con->len = 4;
	*con2 = ccnl_nfn_krivine_str2const("\'data\'");


	return 1;
}

int ccnl_test_run_stack_type_const_str2const(void *con1, void *con2){

	int res = !memcmp(con1, con2, sizeof(struct const_s) +4);
	return res;
}

int ccnl_test_cleanup_stack_type_const_str2const(void *con1, void *con2){
	
	ccnl_free(con1);
	ccnl_free(con2);

	return 1;
}



int ccnl_test_prepare_stack_type_const_const2str(void **str, void **con2){

	*str = ccnl_calloc(1,sizeof(char)* 9);
	strcpy(*str, "\'data\'");

	*con2 = ccnl_nfn_krivine_str2const("\'data\'");


	return 1;
}

int ccnl_test_run_stack_type_const_const2str(void *str, void *con2){

	int res = !strcmp(str, ccnl_nfn_krivine_const2str(con2));
	return res;
}

int ccnl_test_cleanup_stack_type_const_const2str(void *str, void *con2){
	
	ccnl_free(str);
	ccnl_free(con2);

	return 1;
}

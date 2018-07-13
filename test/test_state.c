#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
 
#include "ccnl-fwd.h"
 
void test1()
{
  int result = ccnl_fwd_handleInterest(NULL, NULL, NULL, NULL);
  assert_int_equal(result, 0);
}
 
int main(void)
{
  const UnitTest tests[] = {
    unit_test(test1),
  };
 
  return run_tests(tests);
}

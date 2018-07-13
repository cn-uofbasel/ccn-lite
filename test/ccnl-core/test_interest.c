#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
 
#include "ccnl-interest.h"
 
void test1()
{
  int result = 0;
  assert_int_equal(result, 0);
}
 
int main(void)
{
  const UnitTest tests[] = {
    unit_test(test1),
  };
 
  return run_tests(tests);
}

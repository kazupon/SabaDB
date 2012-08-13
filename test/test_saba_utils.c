#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "saba_utils.h"


/*
 * prototype(s)
 */

int setup(void);
int teardown(void);
void test_array_size(void);
void test_container_of(void);


/*
 * runner
 */

int main() {
  CU_pSuite suite;

  CU_initialize_registry();
  suite = CU_add_suite("utilities", setup, teardown);
  //CU_add_test(suite, "test_add", test_add);
  CU_ADD_TEST(suite, test_array_size);
  CU_ADD_TEST(suite, test_container_of);

  //CU_console_run_tests();
  CU_basic_run_tests();
  CU_cleanup_registry();

  return 0;
}


/*
 * test(s)
 */

int setup(void) {
  /*
  printf("setup\n");
  */
  return 0;
}

int teardown(void) {
  /*
  printf("teardown\n");
  */
  return 0;
}

void test_array_size(void) {
  int32_t a[10];

  CU_ASSERT_EQUAL(ARRAY_SIZE(a), 10);
}

void test_container_of(void) {
  typedef struct {
    int32_t a;
  } hoge_t;

  typedef struct {
    int32_t a;
    hoge_t b;
    int64_t c;
  } foo_t;

  foo_t foo;
  hoge_t *hoge = &foo.b;
  foo_t *foo_ptr = container_of(hoge, foo_t, b);

  CU_ASSERT_PTR_EQUAL(foo_ptr, &foo);
}



#include "test_saba_utils.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


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


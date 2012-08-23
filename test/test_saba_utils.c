#include "test_saba_utils.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
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

void test_saba_time(void) {
  CU_ASSERT_NOT_EQUAL(saba_time(), 0.0);
}

void test_saba_jetlag(void) {
  CU_ASSERT(saba_jetlag() > -1);
}

void test_saba_date_www_format(void) {
  char date1[48];
  char date2[48];
  memset(date1, 0, 48);
  memset(date2, 0, 48);
  saba_date_www_format(NAN, INT32_MAX, 6, date2);
  printf("date_www_format: %s\n", date2);
  CU_ASSERT_STRING_NOT_EQUAL(date1, date2);
}


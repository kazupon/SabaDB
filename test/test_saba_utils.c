#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "saba_utils.h"
#include "saba_message.h"
#include "saba_message_queue.h"


/*
 * prototype(s)
 */

int setup(void);
int teardown(void);
void test_array_size(void);
void test_container_of(void);

void test_saba_message_alloc_and_free(void);

void test_saba_message_queue_alloc_and_free(void);
void test_saba_message_queue_is_empty(void);
void test_saba_message_queue_head(void);
void test_saba_message_queue_tail(void);
void test_saba_message_queue_insert_head(void);
void test_saba_message_queue_insert_tail(void);
void test_saba_message_queue_remove(void);
void test_saba_message_queue_clear(void);


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
  CU_ADD_TEST(suite, test_saba_message_alloc_and_free);
  CU_ADD_TEST(suite, test_saba_message_queue_alloc_and_free);
  CU_ADD_TEST(suite, test_saba_message_queue_is_empty);
  CU_ADD_TEST(suite, test_saba_message_queue_head);
  CU_ADD_TEST(suite, test_saba_message_queue_tail);
  CU_ADD_TEST(suite, test_saba_message_queue_insert_head);
  CU_ADD_TEST(suite, test_saba_message_queue_insert_tail);
  CU_ADD_TEST(suite, test_saba_message_queue_remove);
  CU_ADD_TEST(suite, test_saba_message_queue_clear);

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

void test_saba_message_alloc_and_free(void) {
  saba_message_t *msg = saba_message_alloc();

  CU_ASSERT_PTR_NOT_NULL(msg);
  CU_ASSERT_EQUAL(msg->kind, SABA_MESSAGE_KIND_UNKNOWN);
  CU_ASSERT_PTR_NULL(msg->data);

  saba_message_free(msg);
}

void test_saba_message_queue_alloc_and_free(void) {
  saba_message_queue_t *msg_q = saba_message_queue_alloc();

  CU_ASSERT_PTR_NOT_NULL(msg_q);

  saba_message_queue_free(msg_q);
}

void test_saba_message_queue_is_empty(void) {
  saba_message_queue_t *msg_q = saba_message_queue_alloc();
  saba_message_t *msg1 = saba_message_alloc();

  CU_ASSERT_TRUE(saba_message_queue_is_empty(msg_q));

  saba_message_queue_insert_tail(msg_q, msg1);
  CU_ASSERT_FALSE(saba_message_queue_is_empty(msg_q));

  saba_message_queue_remove(msg_q, msg1);
  CU_ASSERT_TRUE(saba_message_queue_is_empty(msg_q));

  saba_message_free(msg1);
  saba_message_queue_free(msg_q);
}

void test_saba_message_queue_head(void) {
  saba_message_queue_t *msg_q = saba_message_queue_alloc();
  saba_message_t *msg1 = saba_message_alloc();
  saba_message_t *msg2 = saba_message_alloc();

  saba_message_t *msg = NULL;
  msg = saba_message_queue_head(msg_q);
  CU_ASSERT_PTR_NULL(msg);

  saba_message_queue_insert_head(msg_q, msg1);
  msg = saba_message_queue_head(msg_q);
  CU_ASSERT_PTR_EQUAL(msg, msg1);

  saba_message_queue_insert_head(msg_q, msg2);
  msg = saba_message_queue_head(msg_q);
  CU_ASSERT_PTR_EQUAL(msg, msg2);

  saba_message_queue_remove(msg_q, msg2);
  saba_message_queue_remove(msg_q, msg1);
  saba_message_free(msg2);
  saba_message_free(msg1);
  saba_message_queue_free(msg_q);
}

void test_saba_message_queue_tail(void) {
  saba_message_queue_t *msg_q = saba_message_queue_alloc();
  saba_message_t *msg1 = saba_message_alloc();
  saba_message_t *msg2 = saba_message_alloc();

  saba_message_t *msg = NULL;
  msg = saba_message_queue_tail(msg_q);
  CU_ASSERT_PTR_NULL(msg);

  saba_message_queue_insert_tail(msg_q, msg1);
  msg = saba_message_queue_tail(msg_q);
  CU_ASSERT_PTR_EQUAL(msg, msg1);

  saba_message_queue_insert_tail(msg_q, msg2);
  msg = saba_message_queue_tail(msg_q);
  CU_ASSERT_PTR_EQUAL(msg, msg2);

  saba_message_queue_remove(msg_q, msg2);
  saba_message_queue_remove(msg_q, msg1);
  saba_message_free(msg2);
  saba_message_free(msg1);
  saba_message_queue_free(msg_q);
}

void test_saba_message_queue_insert_head(void) {
  saba_message_queue_t *msg_q = saba_message_queue_alloc();
  saba_message_t *msg1 = saba_message_alloc();

  saba_message_queue_insert_head(msg_q, msg1);
  saba_message_t *msg = saba_message_queue_head(msg_q);
  CU_ASSERT_PTR_EQUAL(msg, msg1);

  saba_message_queue_remove(msg_q, msg1);
  saba_message_free(msg1);
  saba_message_queue_free(msg_q);
}

void test_saba_message_queue_insert_tail(void) {
  saba_message_queue_t *msg_q = saba_message_queue_alloc();
  saba_message_t *msg1 = saba_message_alloc();

  saba_message_queue_insert_tail(msg_q, msg1);
  saba_message_t *msg = saba_message_queue_tail(msg_q);
  CU_ASSERT_PTR_EQUAL(msg, msg1);

  saba_message_queue_remove(msg_q, msg1);
  saba_message_free(msg1);
  saba_message_queue_free(msg_q);
}

void test_saba_message_queue_remove(void) {
  saba_message_queue_t *msg_q = saba_message_queue_alloc();
  saba_message_t *msg1 = saba_message_alloc();

  CU_ASSERT_TRUE(saba_message_queue_is_empty(msg_q));

  saba_message_queue_insert_tail(msg_q, msg1);
  CU_ASSERT_FALSE(saba_message_queue_is_empty(msg_q));

  saba_message_queue_remove(msg_q, msg1);
  CU_ASSERT_TRUE(saba_message_queue_is_empty(msg_q));

  saba_message_free(msg1);
  saba_message_queue_free(msg_q);
}

void test_saba_message_queue_clear(void) {
  saba_message_queue_t *msg_q = saba_message_queue_alloc();
  saba_message_t *msg1 = saba_message_alloc();
  saba_message_t *msg2 = saba_message_alloc();
  saba_message_t *msg3 = saba_message_alloc();

  saba_message_queue_insert_tail(msg_q, msg1);
  saba_message_queue_insert_tail(msg_q, msg2);
  saba_message_queue_insert_tail(msg_q, msg3);

  saba_message_queue_clear(msg_q);
  CU_ASSERT_TRUE(saba_message_queue_is_empty(msg_q));

  saba_message_free(msg3);
  saba_message_free(msg2);
  saba_message_free(msg1);
  saba_message_queue_free(msg_q);
}


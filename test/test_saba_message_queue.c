#include "test_saba_message_queue.h"
#include "saba_utils.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


void test_saba_message_alloc_and_free(void) {
  saba_message_t *msg = saba_message_alloc();

  CU_ASSERT_PTR_NOT_NULL(msg);
  CU_ASSERT_EQUAL(msg->kind, SABA_MESSAGE_KIND_UNKNOWN);
  CU_ASSERT_PTR_NULL(msg->data);
  CU_ASSERT_PTR_NULL(msg->stream);

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

void lock_unlock_do_work(void *arg) {
  saba_message_queue_t *msg_q = (saba_message_queue_t *)arg;

  saba_message_queue_lock(msg_q);

  saba_message_t *msg1 = saba_message_alloc();
  saba_message_t *msg2 = saba_message_alloc();
  saba_message_t *msg3 = saba_message_alloc();

  saba_message_queue_insert_tail(msg_q, msg1);
  saba_message_queue_insert_tail(msg_q, msg2);
  saba_message_queue_insert_tail(msg_q, msg3);

  saba_message_queue_clear(msg_q);

  saba_message_free(msg3);
  saba_message_free(msg2);
  saba_message_free(msg1);

  saba_message_queue_unlock(msg_q);
}

void test_saba_message_queue_lock_and_unlock(void) {
  uv_thread_t threads[5];
  int i;
  saba_message_queue_t *msg_q = saba_message_queue_alloc();

  int32_t ret;
  for (i = 0; i < ARRAY_SIZE(threads); i++) {
    ret = uv_thread_create(&threads[i], lock_unlock_do_work, msg_q);
    CU_ASSERT_EQUAL(ret, 0);
  }

  for (i = 0; i < ARRAY_SIZE(threads); i++) {
    ret = uv_thread_join(&threads[i]);
    CU_ASSERT_EQUAL(ret, 0);
  }

  CU_ASSERT_TRUE(saba_message_queue_is_empty(msg_q));
  saba_message_queue_free(msg_q);
}

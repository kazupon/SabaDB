#include "test_saba_worker.h"
#include "saba_common.h"
#include "saba_message_queue.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <unistd.h>


void test_saba_worker_alloc_and_free(void) {
  saba_worker_t *worker = saba_worker_alloc();

  CU_ASSERT_PTR_NOT_NULL(worker);
  CU_ASSERT_PTR_NULL(worker->master);
  CU_ASSERT_PTR_NULL(worker->req_queue);
  CU_ASSERT_PTR_NULL(worker->res_queue);
  CU_ASSERT_EQUAL(worker->state, SABA_WORKER_STATE_STOP);

  saba_worker_free(worker);
}

void test_saba_worker_start_and_stop(void) {
  saba_worker_t *worker = saba_worker_alloc();
  saba_message_queue_t *msg_q1 = saba_message_queue_alloc();
  saba_message_queue_t *msg_q2 = saba_message_queue_alloc();
  void *server = (void *)0; /* dummy */

  worker->master = server;
  worker->req_queue = msg_q1;
  worker->res_queue = msg_q2;

  CU_ASSERT_EQUAL(saba_worker_start(worker), SABA_ERR_OK);
  usleep(1000 * 100);
  CU_ASSERT_EQUAL(saba_worker_stop(worker), SABA_ERR_OK);

  saba_message_queue_free(msg_q2);
  saba_message_queue_free(msg_q1);
  saba_worker_free(worker);
}

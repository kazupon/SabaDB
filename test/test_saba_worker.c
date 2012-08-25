#include "test_saba_worker.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <unistd.h>
#include "saba_common.h"
#include "saba_message_queue.h"
#include "saba_logger.h"


typedef struct {
  saba_logger_t *logger;
  saba_worker_t *worker;
} cu_test_worker_t;


static const char *PATH = "./hoge.log";
static cu_test_worker_t data;
static uv_idle_t bootstraper;


static void on_setup(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_worker_t *d = (cu_test_worker_t *)handle->data;
  saba_err_t ret = saba_logger_open(d->logger, handle->loop, PATH, NULL);
  uv_close((uv_handle_t *)handle, NULL);
}

int test_saba_worker_setup(void) {
  unlink(PATH);
  data.logger = saba_logger_alloc();
  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_setup);
  int32_t ret = uv_run(loop);
  if (ret) {
    saba_logger_free(data.logger);
    data.logger = NULL;
    return 1;
  }
  return 0;
}


static void on_teardown(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_worker_t *d = (cu_test_worker_t *)handle->data;
  saba_err_t ret = saba_logger_close(d->logger, handle->loop, NULL);
  uv_close((uv_handle_t *)handle, NULL);
}

int test_saba_worker_teardown(void) {
  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_teardown);
  int32_t ret = uv_run(loop);
  saba_logger_free(data.logger);
  data.logger = NULL;
  unlink(PATH);
  return ret;
}


void test_saba_worker_alloc_and_free(void) {
  saba_worker_t *worker = saba_worker_alloc();

  CU_ASSERT_PTR_NOT_NULL(worker);
  CU_ASSERT_PTR_NULL(worker->master);
  CU_ASSERT_PTR_NULL(worker->req_queue);
  CU_ASSERT_PTR_NULL(worker->res_queue);
  CU_ASSERT_EQUAL(worker->state, SABA_WORKER_STATE_STOP);

  saba_worker_free(worker);
}

static void on_test_saba_worker_start_and_stop(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_worker_t *d = (cu_test_worker_t *)handle->data;

  saba_err_t ret = saba_worker_start(d->worker);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  usleep(1000 * 100);

  ret = saba_worker_stop(d->worker);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  uv_close((uv_handle_t *)handle, NULL);
}

void test_saba_worker_start_and_stop(void) {
  saba_worker_t *worker = saba_worker_alloc();
  saba_message_queue_t *msg_q1 = saba_message_queue_alloc();
  saba_message_queue_t *msg_q2 = saba_message_queue_alloc();
  void *server = (void *)0; /* dummy */

  worker->master = server;
  worker->req_queue = msg_q1;
  worker->res_queue = msg_q2;
  worker->logger = data.logger;
  data.worker = worker;
  
  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_test_saba_worker_start_and_stop);

  uv_run(loop);

  saba_message_queue_free(msg_q2);
  saba_message_queue_free(msg_q1);
  saba_worker_free(worker);
}



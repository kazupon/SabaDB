#include "test_saba_worker.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <unistd.h>
#include "debug.h"
#include "saba_common.h"
#include "saba_message.h"
#include "saba_message_queue.h"
#include "saba_logger.h"


typedef struct {
  saba_logger_t *logger;
  saba_worker_t *worker;
} cu_test_worker_t;

/* dummy master */
typedef struct {
  uv_async_t req_proc_done_notifier;
} saba_master_t ;


static const char *PATH = "./hoge.log";
static cu_test_worker_t data;
static uv_idle_t bootstraper;
static int32_t req_cb_count;


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

  saba_err_t ret = saba_worker_start(d->worker, d->logger);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  usleep(1000 * 100);

  ret = saba_worker_stop(d->worker);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
}

void test_saba_worker_start_and_stop(void) {
  uv_loop_t *loop = uv_default_loop();

  saba_master_t *master = (saba_master_t *)malloc(sizeof(saba_master_t));

  saba_message_queue_t *msg_q1 = saba_message_queue_alloc();
  saba_message_queue_t *msg_q2 = saba_message_queue_alloc();

  saba_worker_t *worker = saba_worker_alloc();

  worker->master = master;
  worker->req_queue = msg_q1;
  worker->res_queue = msg_q2;
  data.worker = worker;
  
  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_test_saba_worker_start_and_stop);

  uv_run(loop);

  saba_worker_free(worker);

  saba_message_queue_free(msg_q2);
  saba_message_queue_free(msg_q1);

  free(master);
}


static saba_message_t* on_worker_request(saba_worker_t *worker, saba_message_t *msg) {
  TRACE("worker=%p, msg=%p\n", worker, msg);
  CU_ASSERT_PTR_NOT_NULL(worker);
  CU_ASSERT_PTR_NOT_NULL(msg);
  CU_ASSERT_EQUAL(msg->kind, SABA_MESSAGE_KIND_ECHO);
  CU_ASSERT_STRING_EQUAL(msg->data, "hello");
  req_cb_count++;
  return NULL;
}

static void on_test_saba_worker_on_request(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_worker_t *d = (cu_test_worker_t *)handle->data;

  saba_message_t *msg1 = saba_message_alloc();
  msg1->kind = SABA_MESSAGE_KIND_ECHO;
  msg1->data = strdup("hello");
  saba_message_queue_insert_tail(d->worker->req_queue, msg1);
  saba_message_t *msg2 = saba_message_alloc();
  saba_message_queue_insert_tail(d->worker->res_queue, msg2);

  d->worker->req_cb = on_worker_request;

  saba_err_t ret = saba_worker_start(d->worker, d->logger);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  usleep(1000 * 100);

  saba_message_queue_remove(d->worker->res_queue, msg2);
  free(msg2->data);
  free(msg2);

  ret = saba_worker_stop(d->worker);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
}

void test_saba_worker_on_request(void) {
  uv_loop_t *loop = uv_default_loop();

  saba_master_t *master = (saba_master_t *)malloc(sizeof(saba_master_t));
  req_cb_count = 0;

  saba_message_queue_t *msg_q1 = saba_message_queue_alloc();
  saba_message_queue_t *msg_q2 = saba_message_queue_alloc();

  saba_worker_t *worker = saba_worker_alloc();

  worker->master = master;
  worker->req_queue = msg_q1;
  worker->res_queue = msg_q2;
  data.worker = worker;
  
  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_test_saba_worker_on_request);

  uv_run(loop);
  CU_ASSERT_EQUAL(req_cb_count, 1);

  saba_worker_free(worker);

  saba_message_queue_free(msg_q2);
  saba_message_queue_free(msg_q1);

  free(master);
}


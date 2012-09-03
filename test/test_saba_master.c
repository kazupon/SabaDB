#include "test_saba_master.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uv.h>
#include "debug.h"
#include "saba_utils.h"
#include "saba_common.h"
#include "saba_message.h"
#include "saba_message_queue.h"
#include "saba_logger.h"
#include "saba_worker.h"


typedef struct {
  saba_logger_t *logger;
  saba_master_t *master;
} cu_test_master_t;


static const char *PATH = "./hoge.log";
static cu_test_master_t data;
static uv_idle_t bootstraper;
static int32_t response_cb_count;
static int32_t request_cb_count;


static void on_setup(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_master_t *d = (cu_test_master_t *)handle->data;
  saba_err_t ret = saba_logger_open(d->logger, handle->loop, PATH, NULL);
  uv_close((uv_handle_t *)handle, NULL);
}

int test_saba_master_setup(void) {
  TRACE("\n");
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
  cu_test_master_t *d = (cu_test_master_t *)handle->data;
  saba_err_t ret = saba_logger_close(d->logger, handle->loop, NULL);
  uv_close((uv_handle_t *)handle, NULL);
}

int test_saba_master_teardown(void) {
  TRACE("\n");
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


void test_saba_master_alloc_and_free(void) {
  TRACE("\n");
  int32_t worker_num = 2;
  saba_master_t *master = saba_master_alloc(worker_num);

  CU_ASSERT_PTR_NOT_NULL(master);
  CU_ASSERT_PTR_NOT_NULL(master->req_queue);
  CU_ASSERT_PTR_NOT_NULL(master->res_queue);
  CU_ASSERT_PTR_NULL(master->logger);

  saba_master_free(master);
}


static void on_master_stop(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_master_t *d = (cu_test_master_t *)handle->data;

  usleep(1000 * 100);

  saba_err_t ret = saba_master_stop(d->master);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  uv_close((uv_handle_t *)handle, NULL);
}

static void on_test_saba_master_start_and_stop(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_master_t *d = (cu_test_master_t *)handle->data;

  saba_err_t ret = saba_master_start(d->master, handle->loop, d->logger, NULL, NULL);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  uv_idle_start(handle, on_master_stop);
}

void test_saba_master_start_and_stop(void) {
  TRACE("\n");
  uv_loop_t *loop = uv_default_loop();
  int32_t worker_num = 1;
  data.master = saba_master_alloc(worker_num);

  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_test_saba_master_start_and_stop);

  uv_run(loop);

  saba_master_free(data.master);
}


static void on_test_saba_master_put_request(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_master_t *d = (cu_test_master_t *)handle->data;

  saba_err_t ret = saba_master_start(d->master, handle->loop, d->logger, NULL, NULL);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  usleep(1000 * 100); /* timing */

  saba_message_t *msg = saba_message_alloc();
  msg->kind = SABA_MESSAGE_KIND_ECHO;
  msg->data = strdup("hello");
  ret = saba_master_put_request(d->master, msg);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  uv_idle_start(handle, on_master_stop);
}

void test_saba_master_put_request(void) {
  uv_loop_t *loop = uv_default_loop();
  int32_t worker_num = 1;
  data.master = saba_master_alloc(worker_num);

  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_test_saba_master_put_request);

  uv_run(loop);

  saba_master_free(data.master);
}


static void on_master_response(saba_master_t *master, saba_message_t *msg) {
  CU_ASSERT_PTR_NOT_NULL(master);
  CU_ASSERT_PTR_NOT_NULL(msg);
  CU_ASSERT_EQUAL(msg->kind, SABA_MESSAGE_KIND_ECHO); 
  CU_ASSERT_STRING_EQUAL(msg->data, "hello");

  response_cb_count++;
  uv_idle_start(&bootstraper, on_master_stop);
}

static saba_message_t* on_master_request(saba_worker_t *worker, saba_message_t *msg) {
  CU_ASSERT_PTR_NOT_NULL(worker);
  CU_ASSERT_PTR_NOT_NULL(msg);
  request_cb_count++;
  return NULL;
}

static void on_test_saba_master_on_response(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_master_t *d = (cu_test_master_t *)handle->data;

  saba_err_t ret = saba_master_start(
    d->master, handle->loop, d->logger, on_master_response, on_master_request
  );
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  usleep(2000 * 100); /* timing */

  saba_message_t *msg = saba_message_alloc();
  msg->kind = SABA_MESSAGE_KIND_ECHO;
  msg->data = strdup("hello");
  ret = saba_master_put_request(d->master, msg);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
}

void test_saba_master_on_response(void) {
  TRACE("\n");
  response_cb_count = 0;
  request_cb_count = 0;
  uv_loop_t *loop = uv_default_loop();
  int32_t worker_num = 1;
  data.master = saba_master_alloc(worker_num);

  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_test_saba_master_on_response);

  uv_run(loop);

  CU_ASSERT_EQUAL(response_cb_count, 1);
  CU_ASSERT_EQUAL(request_cb_count, 1);
  saba_master_free(data.master);
}


static void on_master_100_response(saba_master_t *master, saba_message_t *msg) {
  CU_ASSERT_PTR_NOT_NULL(master);
  CU_ASSERT_PTR_NOT_NULL(msg);
  CU_ASSERT_EQUAL(msg->kind, SABA_MESSAGE_KIND_ECHO); 
  CU_ASSERT_STRING_EQUAL(msg->data, "hello");

  response_cb_count++;
  TRACE("response_cb_count=%d\n", response_cb_count);
  if (response_cb_count == 100) {
    saba_err_t ret = saba_master_stop(master);
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
    uv_close((uv_handle_t *)&bootstraper, NULL);
  }
}

static saba_message_t* on_master_100_request(saba_worker_t *worker, saba_message_t *msg) {
  CU_ASSERT_PTR_NOT_NULL(worker);
  CU_ASSERT_PTR_NOT_NULL(msg);
  request_cb_count++;
  return NULL;
}

static void on_test_saba_master_100_put_request_and_on_response(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_master_t *d = (cu_test_master_t *)handle->data;

  saba_err_t ret = saba_master_start(
    d->master, handle->loop, d->logger, on_master_100_response, on_master_100_request
  );
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  usleep(1000 * 100); /* timing */

  int i = 0;
  for (i = 0; i < 100; i++) {
    saba_message_t *msg = saba_message_alloc();
    msg->kind = SABA_MESSAGE_KIND_ECHO;
    msg->data = strdup("hello");
    ret = saba_master_put_request(d->master, msg);
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
  }
}

void test_saba_master_100_put_request_and_on_response(void) {
  TRACE("\n");
  response_cb_count = 0;
  request_cb_count = 0;
  uv_loop_t *loop = uv_default_loop();
  int32_t worker_num = 4;
  data.master = saba_master_alloc(worker_num);

  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_test_saba_master_100_put_request_and_on_response);

  uv_run(loop);

  CU_ASSERT_EQUAL(response_cb_count, 100);
  CU_ASSERT_EQUAL(request_cb_count, 100);
  saba_master_free(data.master);
}


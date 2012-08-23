#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include "test_saba_logger.h"
#include "saba_utils.h"
#include "saba_common.h"
#include "debug.h"


static uv_idle_t idle;
static int32_t on_open_cb_count;
static int32_t on_close_cb_count;
static int32_t on_log_cb_count;
static volatile int32_t on_log_cb_count_for_thread;
static uv_mutex_t log_cb_count_mtx;
static const char *PATH = "./hoge.log";


void test_saba_logger_alloc_and_free(void) {
  saba_logger_t *logger = saba_logger_alloc();

  CU_ASSERT_PTR_NOT_NULL(logger);
  CU_ASSERT_EQUAL(logger->level, SABA_LOGGER_LEVEL_ERROR);
  CU_ASSERT_TRUE(logger->is_output_display);
  CU_ASSERT_PTR_NULL(logger->path);

  saba_logger_free(logger);
}


static void on_close(saba_logger_t *logger, saba_err_t err) {
  on_close_cb_count++;

  CU_ASSERT_PTR_NOT_NULL(logger);
  CU_ASSERT_EQUAL(err, SABA_ERR_OK);

  saba_logger_free(logger);

  struct stat st;
  CU_ASSERT(stat(PATH, &st) == 0);

  uv_close((uv_handle_t *)&idle, NULL);
}

static void on_open(saba_logger_t *logger, saba_err_t err) {
  on_open_cb_count++;

  CU_ASSERT_PTR_NOT_NULL(logger);
  CU_ASSERT_EQUAL(err, SABA_ERR_OK);
  CU_ASSERT_NOT_EQUAL(logger->fd, -1);

  saba_err_t ret = saba_logger_close(logger, uv_default_loop(), on_close);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
}

void on_test_saba_logger_open_with_specific_path_and_close(uv_idle_t *handle, int status) {
  uv_idle_stop(&idle);

  saba_logger_t *logger = saba_logger_alloc();
  saba_err_t ret = saba_logger_open(logger, handle->loop, PATH, on_open);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
}

void test_saba_logger_open_with_specific_path_and_close(void) {
  on_open_cb_count = 0;
  on_close_cb_count = 0;
  unlink(PATH);

  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &idle);
  uv_idle_start(&idle, on_test_saba_logger_open_with_specific_path_and_close);
  uv_run(loop);

  CU_ASSERT_EQUAL(on_open_cb_count, 1);
  CU_ASSERT_EQUAL(on_close_cb_count, 1);

  unlink(PATH);
}


void on_test_saba_logger_open_with_already_specific_path_and_close(uv_idle_t *handle, int status) {
  uv_idle_stop(&idle);

  saba_logger_t *logger = saba_logger_alloc();
  saba_err_t ret = saba_logger_open(logger, handle->loop, PATH, on_open);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
}

void test_saba_logger_open_with_already_specific_path_and_close(void) {
  on_open_cb_count = 0;
  on_close_cb_count = 0;
  close(open(PATH, O_CREAT, 0666));

  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &idle);
  uv_idle_start(&idle, on_test_saba_logger_open_with_already_specific_path_and_close);
  uv_run(loop);

  CU_ASSERT_EQUAL(on_open_cb_count, 1);
  CU_ASSERT_EQUAL(on_close_cb_count, 1);
  unlink(PATH);
}


void on_test_saba_logger_sync_open_and_close(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);

  saba_logger_t *logger = saba_logger_alloc();

  saba_err_t ret = saba_logger_open(logger, handle->loop, PATH, NULL);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
  struct stat st;
  CU_ASSERT(stat(PATH, &st) == 0);

  ret = saba_logger_close(logger, handle->loop, NULL);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  saba_logger_free(logger);

  uv_close((uv_handle_t *)handle, NULL);
}

void test_saba_logger_sync_open_and_close(void) {
  on_open_cb_count = 0;
  on_close_cb_count = 0;
  unlink(PATH);

  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &idle);
  uv_idle_start(&idle, on_test_saba_logger_sync_open_and_close);
  uv_run(loop);

  CU_ASSERT_EQUAL(on_open_cb_count, 0);
  CU_ASSERT_EQUAL(on_close_cb_count, 0);
  unlink(PATH);
}


void write_log(saba_logger_t *logger, uv_loop_t *loop, saba_logger_log_cb cb) {
  saba_logger_level_t set_levels[] = {
    SABA_LOGGER_LEVEL_DEBUG,
    SABA_LOGGER_LEVEL_INFO,
    SABA_LOGGER_LEVEL_TRACE,
    SABA_LOGGER_LEVEL_WARN,
    SABA_LOGGER_LEVEL_ERROR,
    SABA_LOGGER_LEVEL_DEBUG | SABA_LOGGER_LEVEL_INFO | SABA_LOGGER_LEVEL_TRACE | SABA_LOGGER_LEVEL_WARN | SABA_LOGGER_LEVEL_ERROR,
  };
  int i = 0;
  for (i = 0; i < ARRAY_SIZE(set_levels); i++) {
    logger->level = set_levels[i];
    saba_err_t ret = SABA_ERR_NG;
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_DEBUG, "hello debug\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_INFO, "hello info\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_TRACE, "こんにちは trace\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_WARN, "hello wran\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_ERROR, "hello error\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
  }

  logger->is_output_display = false;
  for (i = 0; i < ARRAY_SIZE(set_levels); i++) {
    logger->level = set_levels[i];
    saba_err_t ret = SABA_ERR_NG;
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_DEBUG, "hello debug\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_INFO, "hello info\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_TRACE, "こんにちは trace\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_WARN, "hello wran\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
    ret = saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_ERROR, "hello error\n");
    CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
  }
}

void on_log(saba_logger_t *logger, saba_logger_level_t level, saba_err_t ret) {
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
  CU_ASSERT_TRUE(
    level & SABA_LOGGER_LEVEL_ERROR || 
    level & SABA_LOGGER_LEVEL_DEBUG ||
    level & SABA_LOGGER_LEVEL_TRACE ||
    level & SABA_LOGGER_LEVEL_INFO ||
    level & SABA_LOGGER_LEVEL_WARN
  );
  on_log_cb_count++;
  if (on_log_cb_count == 20) {
    saba_logger_close(logger, uv_default_loop(), NULL);
    saba_logger_free(logger);
    uv_close(&idle, NULL);
  }
}

void on_test_saba_logger_log(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);

  uv_loop_t *loop = handle->loop;
  saba_logger_t *logger = saba_logger_alloc();
  saba_err_t ret = saba_logger_open(logger, loop, PATH, NULL);

  write_log(logger, loop, on_log);
}

void test_saba_logger_log(void) {
  on_log_cb_count = 0;
  unlink(PATH);

  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &idle);
  uv_idle_start(&idle, on_test_saba_logger_log);
  uv_run(loop);

  CU_ASSERT_EQUAL(on_log_cb_count, 20);
  unlink(PATH);
}


void on_test_saba_logger_sync_log(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  uv_loop_t *loop = handle->loop;

  saba_logger_t *logger = saba_logger_alloc();
  saba_logger_open(logger, loop, PATH, NULL);

  write_log(logger, loop, NULL);

  saba_logger_close(logger, loop, NULL);
  saba_logger_free(logger);

  uv_close((uv_handle_t *)handle, NULL);
}

void test_saba_logger_sync_log(void) {
  unlink(PATH);

  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &idle);
  uv_idle_start(&idle, on_test_saba_logger_sync_log);
  uv_run(loop);

  unlink(PATH);
}


void on_log_thread(saba_logger_t *logger, saba_logger_level_t level, saba_err_t ret) {
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);
  CU_ASSERT_TRUE(
    level & SABA_LOGGER_LEVEL_ERROR || 
    level & SABA_LOGGER_LEVEL_DEBUG ||
    level & SABA_LOGGER_LEVEL_TRACE ||
    level & SABA_LOGGER_LEVEL_INFO ||
    level & SABA_LOGGER_LEVEL_WARN
  );

  uv_mutex_lock(&log_cb_count_mtx);
  on_log_cb_count_for_thread++;
  uv_mutex_unlock(&log_cb_count_mtx);
}

void make_msg(char buf[], int32_t buf_len, const char *msg) {
  char date[48];
  memset(date, 0, sizeof(date));
  saba_date_www_format(NAN, INT32_MAX, 6, date);
  memset(buf, 0, buf_len);
  snprintf(buf, buf_len, "%s %s", date, msg);
}

void log_thread(void *arg) {
  saba_logger_t *logger = (saba_logger_t *)arg;

  uv_loop_t *loop = uv_loop_new();
  
  int32_t i = 0;
  char msg[1024];
  for (i = 0; i < 10; i++) {
    make_msg(msg, sizeof(msg), "hello debug\n");
    saba_logger_log(logger, loop, on_log_thread, SABA_LOGGER_LEVEL_DEBUG, msg);
    make_msg(msg, sizeof(msg), "hello info\n");
    saba_logger_log(logger, loop, on_log_thread, SABA_LOGGER_LEVEL_INFO, msg);
    make_msg(msg, sizeof(msg), "hello trace\n");
    saba_logger_log(logger, loop, on_log_thread, SABA_LOGGER_LEVEL_TRACE, msg);
    make_msg(msg, sizeof(msg), "hello warn\n");
    saba_logger_log(logger, loop, on_log_thread, SABA_LOGGER_LEVEL_WARN, msg);
    make_msg(msg, sizeof(msg), "hello error\n");
    saba_logger_log(logger, loop, on_log_thread, SABA_LOGGER_LEVEL_ERROR, msg);
  }

  uv_run(loop);
  uv_loop_delete(loop);
}

void on_test_saba_logger_log_by_thread(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  uv_loop_t *loop = handle->loop;

  uv_thread_t threads[5];
  saba_logger_t *logger = saba_logger_alloc();
  logger->level = SABA_LOGGER_LEVEL_ALL;
  saba_logger_open(logger, loop, PATH, NULL);

  int32_t i;
  int32_t ret;
  for (i = 0; i < ARRAY_SIZE(threads); i++) {
    ret = uv_thread_create(&threads[i], log_thread, logger);
    CU_ASSERT_EQUAL(ret, 0);
  }

  for (i = 0; i < ARRAY_SIZE(threads); i++) {
    ret = uv_thread_join(&threads[i]);
    CU_ASSERT_EQUAL(ret, 0);
  }

  saba_logger_close(logger, loop, NULL);
  saba_logger_free(logger);

  uv_close((uv_handle_t *)handle, NULL);
}

void test_saba_logger_log_by_thread(void) {
  on_log_cb_count_for_thread = 0;
  unlink(PATH);
  uv_mutex_init(&log_cb_count_mtx);

  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &idle);
  uv_idle_start(&idle, on_test_saba_logger_log_by_thread);
  uv_run(loop);

  uv_mutex_destroy(&log_cb_count_mtx);
  CU_ASSERT_EQUAL(on_log_cb_count_for_thread, 250);
  unlink(PATH);
}



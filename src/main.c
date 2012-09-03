/*
 * SabaDB: main etnry point
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "main.h"
#include "debug.h"
#include "saba_utils.h"
#include "saba_common.h"
#include "saba_logger.h"
#include "saba_server.h"
#include "saba_worker.h"
#include <assert.h>
#include <uv.h>


/*
 * internal defines
 */

typedef struct {
  saba_logger_t *logger;
  saba_server_t *server;
  uv_loop_t *loop;
} saba_bootstrap_info_t;


/*
 * internal prototypes
 */

static void on_signal(int signo);
static void on_logger_open(saba_logger_t *logger, saba_err_t ret);
static void on_logger_close(saba_logger_t *logger, saba_err_t ret);
static void on_logger_log(saba_logger_t *logger, saba_logger_level_t level, saba_err_t ret);
static void on_boot(uv_idle_t *handle, int status);


/*
 * internal variables
 */

static uv_idle_t bootstraper;
static saba_bootstrap_info_t bootstrap_info;
static uv_signal_t signal_detecter;


/*
 * event handlers
 */

static void on_logger_open(saba_logger_t *logger, saba_err_t ret) {
  assert(logger != NULL && ret == SABA_ERR_OK);
  TRACE("logger=%p, ret=%d\n", logger, ret);
}

static void on_logger_close(saba_logger_t *logger, saba_err_t ret) {
  assert(logger != NULL && ret == SABA_ERR_OK);
  TRACE("logger=%p, ret=%d\n", logger, ret);
}

static void on_logger_log(saba_logger_t *logger, saba_logger_level_t level, saba_err_t ret) {
  assert(logger != NULL && ret == SABA_ERR_OK);
  TRACE("logger=%p, ret=%d\n", logger, ret);
}

static void regist_signal_handler(int signal, void (*handler)(int)) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(signal, &sa, NULL);
}

static void on_signal_cb(uv_signal_t *handle, int signum) {
  assert(handle != NULL);

  uv_loop_t *loop = handle->loop;
  saba_bootstrap_info_t *bi = (saba_bootstrap_info_t *)handle->data;
  assert(bi != NULL);

  assert(bi->logger != NULL && bi->server != NULL);
  SABA_LOGGER_LOG(bi->logger, loop, NULL, INFO, "Occured signal number '%d'\n", signum);

  if (bi->server) {
    saba_err_t err = saba_server_stop(bi->server);
    TRACE("stop server: err=%d\n", err);
  }

  if (bi->logger) {
    saba_logger_close(bi->logger, loop, NULL);
  }

  uv_close((uv_handle_t *)handle, NULL);
}

static void on_signal(int signo) {
  assert(
    bootstrap_info.logger != NULL && 
    bootstrap_info.server != NULL && bootstrap_info.loop != NULL
  );
  SABA_LOGGER_LOG(bootstrap_info.logger, bootstrap_info.loop, NULL, INFO, "Occured signal number '%d'\n", signo);

  if (bootstrap_info.server) {
    saba_err_t err = saba_server_stop(bootstrap_info.server);
    TRACE("stop server: err=%d\n", err);
  }

  if (bootstrap_info.logger) {
    saba_logger_close(bootstrap_info.logger, bootstrap_info.loop, NULL);
  }

  uv_close((uv_handle_t *)&signal_detecter, NULL);
  uv_close((uv_handle_t *)&bootstraper, NULL);
}

static void on_boot(uv_idle_t *handle, int status) {
  assert(handle != NULL && status == 0);
  uv_idle_stop(handle);
  
  uv_loop_t *loop = handle->loop;

  saba_bootstrap_info_t *bi = (saba_bootstrap_info_t *)handle->data;
  assert(bi != NULL);

  uv_signal_init(loop, &signal_detecter);
  signal_detecter.data = handle->data;
  uv_signal_start(&signal_detecter, on_signal_cb, SIGINT);
  regist_signal_handler(SIGPIPE, SIG_IGN);

  saba_logger_open(bi->logger, loop, "./sabadb.log", NULL);

  saba_err_t r = saba_server_start(bi->server, loop, bi->logger, "0.0.0.0", 1978);
  TRACE("server start: ret=%d\n", r);
  SABA_LOGGER_LOG(bi->logger, bi->loop, NULL, INFO, "SabaDB start ...\n");

  uv_close((uv_handle_t *)handle, NULL);
}


/*
 * entry point
 */
int main () {
  uv_idle_t *loop = uv_default_loop();

  int32_t worker_num = 1;

  bootstrap_info.logger = saba_logger_alloc();
  bootstrap_info.logger->level = SABA_LOGGER_LEVEL_ALL;
  bootstrap_info.loop = loop;
  bootstrap_info.server = saba_server_alloc(worker_num);

  bootstraper.data = &bootstrap_info;
  uv_idle_init(loop, &bootstraper);
  uv_idle_start(&bootstraper, on_boot);

  int ret = uv_run(loop);
  TRACE("main loop ret: %d\n", ret);

  saba_server_free(bootstrap_info.server);
  saba_logger_free(bootstrap_info.logger);

  return ret;
}

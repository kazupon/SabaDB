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


/*
 * event handlers
 */

static void on_signal(int signo) {
  assert(
    bootstrap_info.logger != NULL && 
    bootstrap_info.server != NULL && bootstrap_info.loop != NULL
  );
  SABA_LOGGER_LOG(bootstrap_info.logger, bootstrap_info.loop, NULL, INFO, "Occured signal number '%d'\n", signo);

  if (bootstrap_info.server) {
    saba_err_t err = saba_server_stop(bootstrap_info.server);
    TRACE("stop server: err=%d\n", err);
    saba_server_free(bootstrap_info.server);
    bootstrap_info.server = NULL;
  }

  if (bootstrap_info.logger) {
    saba_logger_close(bootstrap_info.logger, bootstrap_info.loop, NULL);
  }

  uv_close((uv_handle_t *)&bootstraper, NULL);
}

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

static void on_boot(uv_idle_t *handle, int status) {
  assert(handle != NULL && status == 0);
  uv_idle_stop(handle);
  
  signal(SIGINT, &on_signal);
  signal(SIGTERM, &on_signal);
  signal(SIGPIPE, SIG_IGN);

  saba_bootstrap_info_t *bi = (saba_bootstrap_info_t *)handle->data;
  uv_loop_t *loop = handle->loop;
  assert(bi != NULL);

  saba_logger_open(bi->logger, loop, "./sabadb.log", NULL);

  int32_t worker_num = 1;
  bi->server = saba_server_alloc(worker_num);
  bi->server->logger = bi->logger;

  saba_err_t r = saba_server_start(bi->server, loop, "0.0.0.0", 1978);
  TRACE("server start: ret=%d\n", r);
  SABA_LOGGER_LOG(bi->logger, bi->loop, on_logger_log, INFO, "SabaDB start ...\n");
}


/*
 * entry point
 */
int main () {
  uv_idle_t *loop = uv_default_loop();

  bootstrap_info.logger = saba_logger_alloc();
  bootstrap_info.logger->level = SABA_LOGGER_LEVEL_ALL;
  bootstrap_info.loop = loop;

  bootstraper.data = &bootstrap_info;
  uv_idle_init(loop, &bootstraper);
  uv_idle_start(&bootstraper, on_boot);

  int ret = uv_run(loop);

  saba_logger_free(bootstrap_info.logger);

  return ret;
}

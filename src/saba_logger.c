/*
 * SabaDB: logging module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_logger.h"
#include "saba_utils.h"
#include "debug.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>


/*
 * internal prototype
 */

static int32_t _saba_logger_log_impl(
  saba_logger_t *logger, uv_loop_t *loop, saba_logger_level_t level, const char *msg
);

/*
 * internal macro
 */

#define SABA_LOGGER_REQ_CMN_FIELDS \
  uv_fs_t req; \
  saba_logger_t *logger

/*
 * internal defines
 */

typedef struct {
  SABA_LOGGER_REQ_CMN_FIELDS;
  saba_logger_open_cb cb;
} saba_logger_open_req_t;

typedef struct {
  SABA_LOGGER_REQ_CMN_FIELDS;
  saba_logger_close_cb cb;
} saba_logger_close_req_t;

typedef struct {
  SABA_LOGGER_REQ_CMN_FIELDS;
  saba_logger_log_cb cb;
  saba_logger_level_t level;
  const char *msg;
  int32_t result;
} saba_logger_log_req_t;
  

/*
 * internal variables
 */

static saba_logger_open_req_t open_req;
static saba_logger_close_req_t close_req;
static uv_fs_t write_req;


/*
 * event handlers
 */

static void on_fs_open(uv_fs_t *req) {
  TRACE("req=%p\n", req);
  assert(req != NULL);

  saba_logger_open_req_t *oreq = container_of(req, saba_logger_open_req_t, req);
  assert(oreq != NULL);
  TRACE("result=%zd, code=%d\n", req->result, req->errorno);
  oreq->logger->fd = req->result;

  if (oreq->cb) {
    saba_err_t ret = SABA_ERR_OK;
    if (req->result == -1) {
      ret= SABA_ERR_NG;
      if (oreq->logger->path) {
        free(oreq->logger->path);
      }
    }
    oreq->cb(oreq->logger, ret);
  }

  uv_fs_req_cleanup(req);
}

static void on_fs_close(uv_fs_t *req) {
  TRACE("req=%p\n", req);
  assert(req != NULL);

  saba_logger_close_req_t *creq = container_of(req, saba_logger_close_req_t, req);
  assert(creq != NULL);
  TRACE("result=%zd, code=%d\n", req->result, req->errorno);

  if (creq->cb) {
    creq->cb(creq->logger, (req->result == -1 ? SABA_ERR_NG : SABA_ERR_OK));
  }

  uv_fs_req_cleanup(req);
}

static void on_work(uv_work_t *req) {
  TRACE("req=%p\n", req);
  assert(req != NULL);

  saba_logger_log_req_t *lreq = (saba_logger_log_req_t *)req->data;
  assert(lreq != NULL);

  lreq->result = _saba_logger_log_impl(lreq->logger, req->loop, lreq->level, lreq->msg);
}

static void on_work_done(uv_work_t *req) {
  TRACE("req=%p\n", req);
  assert(req != NULL);

  saba_logger_log_req_t *lreq = (saba_logger_log_req_t *)req->data;
  assert(lreq != NULL);

  if (lreq->cb) {
    lreq->cb(lreq->logger, lreq->level, (lreq->result < 0 ? SABA_ERR_NG : SABA_ERR_OK));
  }
  lreq->cb = NULL;

  if (lreq->msg) {
    free(lreq->msg);
    lreq->msg = NULL;
  }
  lreq->logger = NULL;

  free(req->data);
  req->data = NULL;
  free(req);
}


/*
 * logger implement(s)
 */

saba_logger_t* saba_logger_alloc() {
  saba_logger_t *logger = (saba_logger_t *)malloc(sizeof(saba_logger_t));
  assert(logger != NULL);

  logger->level = SABA_LOGGER_LEVEL_ERROR;
  logger->is_output_display = true;
  logger->path = NULL;
  logger->fd = -1;
  memset(logger->date_buf, 0, sizeof(logger->date_buf));
  memset(logger->msg_buf, 0, sizeof(logger->msg_buf));
  
  uv_mutex_init(&logger->mtx);

  TRACE("logger=%p\n", logger);
  return logger;
}

void saba_logger_free(saba_logger_t *logger) {
  assert(logger != NULL);
  TRACE("logger=%p\n", logger);

  if (logger->path) {
    free(logger->path);
    logger->path = NULL;
  }

  /* TODO: should be close ? */

  uv_mutex_destroy(&logger->mtx);

  free(logger);
}

saba_err_t saba_logger_open(
  saba_logger_t *logger, uv_loop_t *loop, const char *path, saba_logger_open_cb cb) {
  assert(loop != NULL && path != NULL);
  TRACE("loop=%p, path=%p(%s), cb=%p\n", loop, path, path, cb);

  int32_t ret;
  if (cb) {
    logger->path = strdup(path);
    open_req.logger = logger;
    open_req.cb = cb;
    ret = uv_fs_open(
      loop, &open_req.req, path, O_CREAT | O_APPEND | O_WRONLY, 0600, on_fs_open
    );
  } else {
    ret = uv_fs_open(
      loop, &open_req.req, path, O_CREAT | O_APPEND | O_WRONLY, 0600, NULL
    );
    logger->fd = ret;
  }

  TRACE("ret: %d\n", ret);
  return (ret < 0 ? SABA_ERR_NG : SABA_ERR_OK);
}

saba_err_t saba_logger_close(saba_logger_t *logger, uv_loop_t *loop, saba_logger_close_cb cb) {
  assert(logger != NULL && loop != NULL);
  TRACE("logger=%p, cb=%p\n", logger, cb);

  int32_t ret;
  if (cb) {
    close_req.logger = logger;
    close_req.cb = cb;
    ret = uv_fs_close(loop, &close_req.req, logger->fd, on_fs_close);
  } else {
    ret = uv_fs_close(loop, &close_req.req, logger->fd, NULL);
  }

  TRACE("ret: %d\n", ret);
  return (ret < 0 ? SABA_ERR_NG : SABA_ERR_OK);
}

static int32_t _saba_logger_log_impl(
  saba_logger_t *logger, uv_loop_t *loop, saba_logger_level_t level, const char *msg) {
  assert(logger != NULL && loop != NULL);
  /* TODO: should be check arguments */

  if (!(logger->level & level)) {
    return 0; /* TODO: should be return error code ? */
  }

  uv_mutex_lock(&logger->mtx);

  const char *level_msg = "U";
  const char *color_code_format = "\033[90m%s\033[39m";
  switch (level) {
    case SABA_LOGGER_LEVEL_DEBUG:
      level_msg = "D";
      color_code_format = "\033[36m%s\033[39m";
      break;
    case SABA_LOGGER_LEVEL_TRACE:
      level_msg = "T";
      color_code_format = "\033[34m%s\033[39m";
      break;
    case SABA_LOGGER_LEVEL_INFO:
      level_msg = "I";
      color_code_format = "\033[32m%s\033[39m";
      break;
    case SABA_LOGGER_LEVEL_WARN:
      level_msg = "W";
      color_code_format = "\033[33m%s\033[39m";
      break;
    case SABA_LOGGER_LEVEL_ERROR:
      level_msg = "E";
      color_code_format = "\033[31m%s\033[39m";
      break;
  }

  char title[5];
  memset(title, 0, sizeof(title));
  int32_t len = snprintf(title, sizeof(title), "[%s]:", level_msg);

  int32_t color_schem_format_len = 16;
  char colored_title[strlen(title) + color_schem_format_len];
  memset(colored_title, 0, sizeof(colored_title));
  len = snprintf(colored_title, sizeof(colored_title), color_code_format, title);

  int32_t spece_len = 1;
  int32_t msg_len = strlen(msg) + 1;
  char buf[strlen(title) + spece_len + msg_len];
  memset(buf, 0, sizeof(buf));
  len = snprintf(buf, sizeof(buf), "%s %s", title, msg);

  assert(logger->fd != -1);
  uv_fs_req_cleanup(&write_req);
  int32_t ret = uv_fs_write(loop, &write_req, logger->fd, buf, strlen(buf), -1, NULL);

  if (logger->is_output_display) {
    printf("%s %s", colored_title, msg);
  }

  uv_mutex_unlock(&logger->mtx);

  return ret;
}

saba_err_t saba_logger_log(
  saba_logger_t *logger, uv_loop_t *loop, saba_logger_log_cb cb, 
  saba_logger_level_t level, const char *msg) {
  assert(logger != NULL && loop != NULL);
  /* TODO: should be check arguments */

  if (!(logger->level & level)) {
    return SABA_ERR_OK; /* TODO: should be return error code ? */
  }

  int32_t ret;
  if (cb) {
    uv_work_t *uv_req = (uv_work_t *)malloc(sizeof(uv_work_t));
    assert(uv_req != NULL);
    saba_logger_log_req_t *req = (saba_logger_log_req_t *)malloc(sizeof(saba_logger_log_req_t));
    req->msg = strdup(msg);
    req->logger = logger;
    req->level = level;
    req->cb = cb;
    uv_req->data = req;
    ret = uv_queue_work(loop, uv_req, on_work, on_work_done);
    if (ret) {
      uv_err_t err = uv_last_error(loop);
      TRACE("uv_queue_work error: %s(%d)\n", uv_strerror(err), err.code);
    }
  } else {
    ret = _saba_logger_log_impl(logger, loop, level, msg);
  }

  TRACE("ret: %d\n", ret);
  return (ret < 0 ? SABA_ERR_NG : SABA_ERR_OK);
}


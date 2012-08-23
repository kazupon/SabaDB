/*
 * SabaDB: logging module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef SABA_LOGGER_H
#define SABA_LOGGER_H


#include <stdbool.h>
#include <uv.h>
#include "saba_common.h"


typedef struct saba_logger_s saba_logger_t;


/*
 * logger level
 */

typedef enum {
  SABA_LOGGER_LEVEL_DEBUG = 1 << 0,
  SABA_LOGGER_LEVEL_INFO = 1 << 1,
  SABA_LOGGER_LEVEL_TRACE = 1 << 2,
  SABA_LOGGER_LEVEL_WARN = 1 << 3,
  SABA_LOGGER_LEVEL_ERROR = 1 << 4,
  SABA_LOGGER_LEVEL_ALL = SABA_LOGGER_LEVEL_DEBUG | SABA_LOGGER_LEVEL_INFO | SABA_LOGGER_LEVEL_TRACE | SABA_LOGGER_LEVEL_WARN | SABA_LOGGER_LEVEL_ERROR,
} saba_logger_level_t;

/*
 * callback define(s)
 */
typedef void (*saba_logger_open_cb)(saba_logger_t *logger, saba_err_t err);
typedef void (*saba_logger_close_cb)(saba_logger_t *logger, saba_err_t err);
typedef void (*saba_logger_log_cb)(saba_logger_t *logger, saba_logger_level_t level, saba_err_t ret);

/*
 * logger
 */
struct saba_logger_s {
  /* public */
  saba_logger_level_t level;
  bool is_output_display;
  void *data;
  const char *path;
  /*
  saba_logger_log_cb log_cb;
  */
  /* public */
  /* private */
  uv_file fd;
  uv_mutex_t mtx;
  /*
  saba_logger_alloc_cb alloc_cb;
  saba_logger_free_cb free_cb;
  */
  /* private */
};


/*
 * logger prototype(s)
 */

saba_logger_t* saba_logger_alloc();
void saba_logger_free(saba_logger_t *logger);
saba_err_t saba_logger_open(
  saba_logger_t *logger, uv_loop_t *loop,
  const char *path, saba_logger_open_cb
);
saba_err_t saba_logger_close(saba_logger_t *logger, uv_loop_t *loop, saba_logger_close_cb);
saba_err_t saba_logger_log(
  saba_logger_t *logger, uv_loop_t *loop, saba_logger_log_cb cb, 
  saba_logger_level_t level, const char *msg
);


#endif /* SABA_LOGGER_H */


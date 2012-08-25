/*
 * SabaDB: logging module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef SABA_LOGGER_H
#define SABA_LOGGER_H


#include <uv.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "saba_common.h"
#include "saba_utils.h"


/*
 * macros
 */

#define SABA_LOGGER_MSG(logger, format, ...) \
  do { \
    assert(logger != NULL); \
    memset(logger->date_buf, 0, sizeof(logger->date_buf)); \
    saba_date_www_format(NAN, INT32_MAX, 6, logger->date_buf); \
    memset(logger->msg_buf, 0, sizeof(logger->msg_buf)); \
    snprintf(logger->msg_buf, sizeof(logger->msg_buf), "%s (%p) " format, logger->date_buf, pthread_self(), ##__VA_ARGS__); \
  } while (0)

#define SABA_LOGGER_LOG(logger, loop, cb, level, format, ...) \
  SABA_LOGGER_MSG(logger, format, ##__VA_ARGS__); \
  saba_logger_log(logger, loop, cb, SABA_LOGGER_LEVEL_ ## level, logger->msg_buf)


typedef struct saba_logger_s saba_logger_t; /* logger declar */


/*
 * logger level const
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
 * logger callbacks
 */
typedef void (*saba_logger_open_cb)(saba_logger_t *logger, saba_err_t ret);
typedef void (*saba_logger_close_cb)(saba_logger_t *logger, saba_err_t ret);
typedef void (*saba_logger_log_cb)(saba_logger_t *logger, saba_logger_level_t level, saba_err_t ret);


/*
 * logger
 */

struct saba_logger_s {
  /* public */
  saba_logger_level_t level;
  bool is_output_display;
  const char *path;
  /* public */
  /* private */
  uv_file fd;
  uv_mutex_t mtx;
  char date_buf[48];
  char msg_buf[2048];
  /* private */
};


/*
 * logger prototypes
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


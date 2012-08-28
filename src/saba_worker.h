/*
 * SabaDB: worker module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef SABA_WORKER_H
#define SABA_WORKER_H


#include <uv.h>
#include "saba_common.h"
#include "saba_message_queue.h"
#include "saba_logger.h"


struct saba_server_s; /* server declar */


/*
 * worker state
 */

typedef enum {
  SABA_WORKER_STATE_IDLE,
  SABA_WORKER_STATE_BUSY,
  SABA_WORKER_STATE_INIT,
  SABA_WORKER_STATE_STOP,
} saba_worker_state_t;


/*
 * worker
 */

typedef struct {
  /* public */
  uv_thread_t tid;
  saba_message_queue_t *req_queue;
  saba_message_queue_t *res_queue;
  saba_logger_t *logger;
  volatile saba_worker_state_t state;
  struct saba_worker_s *master;
  /* public */
  /* private */
  uv_loop_t *loop;
  uv_idle_t queue_watcher;
  uv_async_t req_proc_notifier;
  uv_async_t stop_notifier;
  ngx_queue_t q;
  /* private */
} saba_worker_t;


/*
 * worker prototypes
 */

saba_worker_t* saba_worker_alloc(void);
void saba_worker_free(saba_worker_t *worker);
saba_err_t saba_worker_start(saba_worker_t *worker);
saba_err_t saba_worker_stop(saba_worker_t *worker);


#endif /* SABA_WORKER_H */


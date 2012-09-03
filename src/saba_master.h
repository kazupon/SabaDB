/*
 * SabaDB: master module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef SABA_MASTER_H
#define SABA_MASTER_H


#include <uv.h>
#include <kclangc.h>
#include "saba_common.h"
#include "saba_message.h"
#include "saba_message_queue.h"
#include "saba_logger.h"
#include "saba_worker.h"


typedef struct saba_master_s saba_master_t; /* declar */

/*
 * master callbacks
 */
typedef void (*saba_master_response_cb)(saba_master_t *master, saba_message_t *msg);


/*
 * master
 */

typedef struct saba_master_s {
  /* public */
  saba_message_queue_t *req_queue;
  saba_message_queue_t *res_queue;
  ngx_queue_t workers; 
  saba_logger_t *logger;
  /* public */
  /* private */
  uv_loop_t *loop;
  saba_master_response_cb res_cb;
  uv_idle_t res_queue_watcher;
  uv_async_t req_proc_done_notifier;
  /* private */
};


/*
 * master prototypes
 */

saba_master_t* saba_master_alloc(int32_t worker_num);
void saba_master_free(saba_master_t *master);
saba_err_t saba_master_start(
  saba_master_t *master, uv_loop_t *loop, saba_logger_t *looger,
  saba_master_response_cb res_cb, saba_worker_on_request req_cb
);
saba_err_t saba_master_stop(saba_master_t *master);
saba_err_t saba_master_put_request(saba_master_t *master, saba_message_t *msg);


#endif /* SABA_MASTER_H */


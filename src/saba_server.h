/*
 * SabaDB: server module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef SABA_SERVER_H
#define SABA_SERVER_H


#include <uv.h>
#include <kclangc.h>
#include "saba_common.h"
#include "saba_message_queue.h"
#include "saba_worker.h"


/*
 * server
 */
typedef struct {
  uv_tcp_t tcp;
  KCDB *db; /* TODO: */
  saba_message_queue_t *req_queue;
  saba_message_queue_t *res_queue;
  saba_worker_t **workers;
  uv_idle_t res_queue_watcher;
  uv_async_t req_proc_done_notifier;
} saba_server_t;


/*
 * server prototype(s)
 */
saba_server_t* saba_server_alloc();
void saba_server_free(saba_server_t *server);
saba_err_t saba_server_start(saba_server_t *server, uv_loop_t *loop, const char *address, uint16_t port);
saba_err_t saba_server_stop(saba_server_t *server);


#endif /* SABA_SERVER_H */


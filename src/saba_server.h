/*
 * SabaDB: server module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#ifndef SABA_SERVER_H
#define SABA_SERVER_H


#include <uv.h>
#include <kclangc.h>
#include "saba_common.h"
#include "saba_logger.h"
#include "saba_master.h"


/*
 * server
 */

typedef struct saba_server_s {
  /* public */
  uv_tcp_t tcp;
  KCDB *db; /* TODO: */
  uv_loop_t *loop;
  saba_logger_t *logger;
  /* public */
  /* private */
  saba_master_t *master;
  /* private */
} saba_server_t;


/*
 * server prototypes
 */

saba_server_t* saba_server_alloc(int32_t worker_num, saba_logger_t *logger);
void saba_server_free(saba_server_t *server);
saba_err_t saba_server_start(
  saba_server_t *server, uv_loop_t *loop, const char *address, uint16_t port
);
saba_err_t saba_server_stop(saba_server_t *server);


#endif /* SABA_SERVER_H */


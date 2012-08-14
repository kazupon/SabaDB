/*
 * SabaDB: main etnry point
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "main.h"
#include "debug.h"
#include "saba_utils.h"
#include "saba_common.h"
#include "saba_server.h"
#include "saba_worker.h"
#include <assert.h>
#include <uv.h>


static saba_server_t *server;

static void on_signal(int signo) {
  TRACE("signo: %d\n", signo);

  if (server) {
    saba_err_t err = saba_server_stop(server);
    TRACE("stop server: err=%d\n", err);
  }
}

int main () {
  signal(SIGINT, &on_signal);
  signal(SIGTERM, &on_signal);
  signal(SIGPIPE, SIG_IGN);

  uv_loop_t *loop = uv_default_loop();

  server = saba_server_alloc();
  assert(server != NULL);

  int32_t ret = saba_server_start(server, loop, "0.0.0.0", 1978);
  TRACE("server start: ret=%d\n", ret);

  ret = uv_run(loop);
  saba_server_free(server);

  return ret;
}

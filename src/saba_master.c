/*
 * SabaDB: master module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_master.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "saba_worker.h"
#include "debug.h"
#include "saba_utils.h"



/*
 * event handlers
 */

static void on_master_log(saba_logger_t *logger, saba_logger_level_t level, saba_err_t ret) {
  TRACE("logger=%p, level=%d, ret=%d\n", logger, level, ret);
}

static void on_watch_res_queue(uv_idle_t *watcher, int status) {
  assert(watcher != NULL && status == 0);
  TRACE("watcher=%p, status=%d\n", watcher, status);

  uv_loop_t *loop = watcher->loop;

  saba_master_t *master = container_of(watcher, saba_master_t, res_queue_watcher);
  assert(master != NULL && master->res_queue != NULL);

  saba_message_queue_lock(master->res_queue);

  /* check queue */
  if (saba_message_queue_is_empty(master->res_queue)) {
    TRACE("respose message is nothing in response message queue\n");
    int32_t ret = uv_idle_stop(&master->res_queue_watcher);
    if (ret) {
      uv_err_t err = uv_last_error(watcher->loop);
      SABA_LOGGER_LOG(
        master->logger, watcher->loop, on_master_log, ERROR,
        "stop response queue watching: %s (%d)\n", uv_strerror(err), err.code
      );
      abort();
    }
    saba_message_queue_unlock(master->res_queue);
    return;
  }

  /* get message */
  saba_message_t *msg = saba_message_queue_head(master->res_queue);
  if (msg == NULL) {
    TRACE("get reponse message is null\n");
    saba_message_queue_unlock(master->res_queue);
    return;
  }
  TRACE("get response message: kind=%d, stream=%p, data=%p\n", msg->kind, msg->stream, msg->data);

  /* call response callback */
  if (master->res_cb) {
    TRACE("call response callback\n");
    master->res_cb(master, msg);
  }

  /* remove messge from respone message queue */
  saba_message_queue_remove(master->res_queue, msg);

  if (msg->data) {
    free(msg->data);
  }
  free(msg);

  saba_message_queue_unlock(master->res_queue);
}

static void on_notify_req_proc_done(uv_async_t *notifier, int status) {
  assert(notifier != NULL && status == 0);
  TRACE("notifier=%p, status=%d\n", notifier, status);

  saba_master_t *master = container_of(notifier, saba_master_t, req_proc_done_notifier);
  assert(master != NULL && master->res_queue != NULL);

  saba_message_queue_lock(master->res_queue);

  /* check queue */
  if (!saba_message_queue_is_empty(master->res_queue)) {
    TRACE("respose message in response message queue.\n");
    int32_t ret = uv_idle_start(&master->res_queue_watcher, on_watch_res_queue);
    if (ret) {
      uv_err_t err = uv_last_error(notifier->loop);
      SABA_LOGGER_LOG(
        master->logger, notifier->loop, on_master_log, ERROR,
        "start response queue watching: %s (%d)\n", uv_strerror(err), err.code
      );
      abort();
    }
  }

  saba_message_queue_unlock(master->res_queue);
}


/*
 * master implements
 */

saba_master_t* saba_master_alloc(int32_t worker_num) {
  saba_master_t *master = (saba_master_t *)malloc(sizeof(saba_master_t));
  assert(master != NULL);
  TRACE("master=%p\n", master);

  master->logger = NULL;
  master->loop = NULL;
  master->req_queue = saba_message_queue_alloc();
  master->res_queue = saba_message_queue_alloc();
  assert(master->req_queue != NULL && master->res_queue != NULL);
  TRACE("req_queue=%p, res_queue=%p\n", master->req_queue, master->res_queue);

  int32_t i = 0;
  ngx_queue_init(&master->workers);
  for (i = 0; i < worker_num; i++) {
    saba_worker_t *worker = saba_worker_alloc();
    worker->master = master;
    worker->req_queue = master->req_queue;
    worker->res_queue = master->res_queue;
    ngx_queue_insert_tail(&master->workers, &worker->q);
  }

  return master;
}

void saba_master_free(saba_master_t *master) {
  assert(master != NULL);
  TRACE("master=%p\n", master);

  while (!ngx_queue_empty(&master->workers)) {
    ngx_queue_t *q = ngx_queue_head(&master->workers);
    assert(q != NULL);
    ngx_queue_remove(q);
    saba_worker_t *worker = ngx_queue_data(q, saba_worker_t, q);
    assert(worker != NULL);
    saba_worker_free(worker);
  }

  saba_message_queue_free(master->req_queue);
  saba_message_queue_free(master->res_queue);
  master->req_queue = NULL;
  master->res_queue = NULL;

  free(master);
}

saba_err_t saba_master_start(
  saba_master_t *master, uv_loop_t *loop, saba_logger_t *logger,
  saba_master_response_cb res_cb, saba_worker_on_request req_cb
) {
  assert(master != NULL && loop != NULL);
  TRACE(
    "master=%p, loop=%p, logger=%p, res_cb=%p, req_cb=%p\n", 
    master, loop, logger, res_cb, req_cb
  );
  
  int32_t ret = uv_idle_init(loop, &master->res_queue_watcher);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      master->logger, loop, on_master_log, ERROR,
      "response queue watcher init error: %s (%d)\n", uv_strerror(err), err.code
    );
    /* TODO: should be released saba_server_t resource */
    return SABA_ERR_NG;
  }

  ret = uv_idle_start(&master->res_queue_watcher, on_watch_res_queue);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      master->logger, loop, on_master_log, ERROR,
      "watch response queue start error: %s (%d)\n", uv_strerror(err), err.code
    );
    /* TODO: should be released saba_server_t resource */
    return SABA_ERR_NG;
  }
  uv_ref((uv_handle_t *)&master->res_queue_watcher);

  ret = uv_async_init(loop, &master->req_proc_done_notifier, on_notify_req_proc_done);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      master->logger, loop, on_master_log, ERROR,
      "request want notifier init error: %s (%d)\n", uv_strerror(err), err.code
    );
    /* TODO: should be released saba_server_t resource */
    return SABA_ERR_NG;
  }
  uv_ref((uv_handle_t *)&master->req_proc_done_notifier);

  master->loop = loop;
  master->logger = logger;
  master->res_cb = res_cb;

  ngx_queue_t *q;
  ngx_queue_foreach(q, &master->workers) {
    saba_worker_t *worker = ngx_queue_data(q, saba_worker_t, q);
    worker->req_cb = req_cb;
    saba_err_t err = saba_worker_start(worker, master->logger);
    SABA_LOGGER_LOG(
      master->logger, loop, on_master_log, INFO,
      "start worker ...\n"
    );
    TRACE("start worker: err=%d\n", err);
  }

  return (saba_err_t)ret;
}

saba_err_t saba_master_stop(saba_master_t *master) {
  assert(master != NULL);
  TRACE("master=%p\n", master);

  int32_t ret = 0;
  ngx_queue_t *q;
  ngx_queue_foreach(q, &master->workers) {
    saba_worker_t *worker = ngx_queue_data(q, saba_worker_t, q);
    SABA_LOGGER_LOG(master->logger, master->loop, NULL, INFO, "stop worker ...\n");
    saba_err_t err = saba_worker_stop(worker);
    SABA_LOGGER_LOG(master->logger, master->loop, NULL, INFO, "... worker stop\n");
    TRACE("stop worker: ret=%d\n", err);
    ret = err;
  }

  ret = uv_idle_stop(&master->res_queue_watcher);
  if (ret) {
    uv_err_t err = uv_last_error(&master->req_proc_done_notifier.loop);
    SABA_LOGGER_LOG(
      master->logger, master->loop, NULL, ERROR,
      "stop response queue watcher error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }
  uv_unref((uv_handle_t *)&master->res_queue_watcher);
  uv_close((uv_handle_t *)&master->res_queue_watcher, NULL);

  uv_unref((uv_handle_t *)&master->req_proc_done_notifier);
  uv_close((uv_handle_t *)&master->req_proc_done_notifier, NULL);

  return (saba_err_t)ret;
}

saba_err_t saba_master_put_request(saba_master_t *master, saba_message_t *msg) {
  assert(master != NULL && msg != NULL);
  TRACE("master=%p, msg=%p\n", master, msg);

  saba_message_queue_lock(master->req_queue);

  /* insert message to request message queue */
  saba_message_queue_insert_tail(master->req_queue, msg);
  TRACE("put request message\n");

  ngx_queue_t *q = ngx_queue_head(&master->workers);
  assert(q != NULL);
  ngx_queue_remove(q);
  saba_worker_t *worker = ngx_queue_data(q, saba_worker_t, q);
  assert(worker != NULL);
  int32_t ret = uv_async_send(&worker->req_proc_notifier);
  if (ret) {
    uv_err_t err = uv_last_error(master->loop);
    SABA_LOGGER_LOG(
      master->logger, master->loop, on_master_log, ERROR,
      "request proc notifier error: %s (%d)\n", uv_strerror(err), err.code
    );
    saba_message_queue_unlock(master->req_queue);
    return SABA_ERR_NG;
  }

  ngx_queue_insert_tail(&master->workers, &worker->q);

  saba_message_queue_unlock(master->req_queue);

  return SABA_ERR_OK;
}


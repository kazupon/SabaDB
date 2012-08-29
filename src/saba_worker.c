/*
 * SabaDB: worker module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_worker.h"
#include "debug.h"
#include "saba_utils.h"
#include "saba_master.h"
#include <assert.h>
#include <stdlib.h>


/*
 * event handlers
 */

static void on_worker_log(saba_logger_t *logger, saba_logger_level_t level, saba_err_t ret) {
  TRACE("logger=%p, level=%d, ret=%d\n", logger, level, ret);
}

static void on_notify_stopping(uv_async_t *notifier, int status) {
  TRACE("notifier=%p, status=%d\n", notifier, status);
  
  saba_worker_t *worker = container_of(notifier, saba_worker_t, stop_notifier);
  assert(worker != NULL && worker->req_queue != NULL);
  TRACE("worker->state=%d\n", worker->state);

  SABA_LOGGER_LOG(worker->logger, notifier->loop, on_worker_log, INFO, "fire stop worker\n");
  switch (worker->state) {
    case SABA_WORKER_STATE_IDLE:
      uv_unref((uv_handle_t *)&worker->queue_watcher);
      uv_close((uv_handle_t *)&worker->queue_watcher, NULL);
      uv_unref((uv_handle_t *)&worker->req_proc_notifier);
      uv_close((uv_handle_t *)&worker->req_proc_notifier, NULL);
      uv_unref((uv_handle_t *)&worker->stop_notifier);
      uv_close((uv_handle_t *)&worker->stop_notifier, NULL);
      break;
    case SABA_WORKER_STATE_BUSY:
      /* TODO: should be implemented !! */
      break;
    case SABA_WORKER_STATE_INIT:
      break;
    case SABA_WORKER_STATE_STOP:
      TRACE("already worker stop\n");
      break;
    default:
      assert(0);
      break;
  }
}

static void on_watch_req_queue(uv_idle_t *watcher, int status) {
  assert(watcher != NULL && status == 0);
  TRACE("watcher=%p, status=%d\n", watcher, status);

  uv_loop_t *loop = watcher->loop;
  assert(loop != NULL);

  saba_worker_t *worker = container_of(watcher, saba_worker_t, queue_watcher);
  assert(worker != NULL && worker->req_queue != NULL);

  saba_message_queue_lock(worker->req_queue);
  TRACE("worker state(before): %d\n", worker->state);

  /* check queue */
  if (saba_message_queue_is_empty(worker->req_queue)) {
    TRACE("request message is nothing in request message queue\n");
    int32_t ret = uv_idle_stop(&worker->queue_watcher);
    if (ret) {
      uv_err_t err = uv_last_error(loop);
      SABA_LOGGER_LOG(
        worker->logger, loop, on_worker_log, ERROR,
        "stop request queue watching error: %s (%d)\n", uv_strerror(err), err.code
      );
      abort();
    }
    worker->state = SABA_WORKER_STATE_IDLE;
    TRACE("worker state(after): %d\n", worker->state);
    saba_message_queue_unlock(worker->req_queue);
    return;
  } else {
    worker->state = SABA_WORKER_STATE_BUSY;
    TRACE("worker state(after): %d\n", worker->state);
  }

  /* get message */
  saba_message_t *msg = saba_message_queue_head(worker->req_queue);
  assert(msg != NULL);
  TRACE("get request message: kind=%d, stream=%p, data=%p\n", msg->kind, msg->stream, msg->data);

  /* TODO: somthing todo ... */
  if (worker->req_cb) {
    TRACE("request callback: %p\n", worker->req_cb);
    worker->req_cb(worker, msg);
  }

  /* remove message to request message queue */
  saba_message_queue_remove(worker->req_queue, msg);

  saba_message_queue_unlock(worker->req_queue);

  assert(worker->res_queue != NULL);
  saba_message_queue_lock(worker->res_queue);
  
  /* put response message in response queue */
  bool is_empty = saba_message_queue_is_empty(worker->res_queue);

  saba_message_t *res_msg = saba_message_alloc();
  res_msg->kind = msg->kind;
  res_msg->data = strdup(msg->data);
  res_msg->stream = msg->stream;

  saba_message_queue_insert_tail(worker->res_queue, res_msg);
  TRACE("put response message in response message queue\n");
  
  if (is_empty) {
    //int32_t ret = uv_async_send(&((saba_server_t *)worker->master)->req_proc_done_notifier);
    int32_t ret = uv_async_send(&((saba_master_t *)worker->master)->req_proc_done_notifier);
    if (ret) {
      uv_err_t err = uv_last_error(loop);
      SABA_LOGGER_LOG(
        worker->logger, loop, on_worker_log, ERROR,
        "request procedure done notify error: %s (%d)\n", uv_strerror(err), err.code
      );
      abort();
    }
  }
  
  if (msg->data) {
    free(msg->data);
  }
  free(msg);

  saba_message_queue_unlock(worker->res_queue);
}

static void on_notify_req_proc(uv_async_t *notifier, int status) {
  TRACE("notifier=%p, status=%d\n", notifier, status);
  
  saba_worker_t *worker = container_of(notifier, saba_worker_t, req_proc_notifier);
  assert(worker != NULL && worker->req_queue != NULL);

  saba_message_queue_lock(worker->req_queue);

  /* check queue */
  if (!saba_message_queue_is_empty(worker->req_queue)) {
    TRACE("request message in request message queue.\n");
    int32_t ret = uv_idle_start(&worker->queue_watcher, on_watch_req_queue);
    if (ret) {
      uv_err_t err = uv_last_error(notifier->loop);
      SABA_LOGGER_LOG(
        worker->logger, notifier->loop, on_worker_log, ERROR,
        "start request queue watching: %s (%d)\n", uv_strerror(err), err.code
      );
      abort();
    }
  }

  saba_message_queue_unlock(worker->req_queue);
}


/* 
 * thread entry point
 */
static void do_work(void *arg) {
  TRACE("arg=%p\n", arg);
  saba_worker_t *worker = (saba_worker_t *)arg;
  assert(worker != NULL);

  worker->state = SABA_WORKER_STATE_INIT;

  uv_loop_t *loop = uv_loop_new();
  worker->loop = loop;
  assert(loop != NULL);
  TRACE("loop=%p\n", loop);


  int32_t ret = uv_async_init(loop, &worker->stop_notifier, on_notify_stopping);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      worker->logger, loop, on_worker_log, ERROR,
      "init request stop notifier error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }
  uv_ref((uv_handle_t *)&worker->stop_notifier);

  ret = uv_async_init(loop, &worker->req_proc_notifier, on_notify_req_proc);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      worker->logger, loop, on_worker_log, ERROR,
      "init request procedure notifier error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }
  uv_ref((uv_handle_t *)&worker->req_proc_notifier);

  ret = uv_idle_init(loop, &worker->queue_watcher);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      worker->logger, loop, on_worker_log, ERROR,
      "init queue watcher error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }

  ret = uv_idle_start(&worker->queue_watcher, on_watch_req_queue);
  if (ret) {
    uv_err_t err = uv_last_error(loop);
    SABA_LOGGER_LOG(
      worker->logger, loop, on_worker_log, ERROR,
      "start queue watcher error: %s (%d)\n", uv_strerror(err), err.code
    );
    abort();
  }
  uv_ref((uv_handle_t *)&worker->queue_watcher);

  TRACE("run ...\n");
  SABA_LOGGER_LOG(worker->logger, loop, on_worker_log, INFO, "... worker run\n");
  ret = uv_run(loop);
  assert(ret == 0);
  TRACE("... run done\n");

  uv_loop_delete(loop);

  worker->state = SABA_WORKER_STATE_STOP;
}


/*
 * worker implements
 */

saba_worker_t* saba_worker_alloc(void) {
  saba_worker_t *worker = (saba_worker_t *)malloc(sizeof(saba_worker_t));
  assert(worker != NULL);
  TRACE("worker=%p\n", worker);

  worker->master = NULL;
  worker->req_queue = NULL;
  worker->res_queue = NULL;
  worker->logger = NULL;
  worker->loop = NULL;
  worker->state = SABA_WORKER_STATE_STOP;
  worker->req_cb = NULL;

  return worker;
}

void saba_worker_free(saba_worker_t *worker) {
  TRACE("worker=%p\n", worker);

  /* TODO: should be stop thread !! */
  worker->master = NULL;
  worker->logger = NULL;
  worker->loop = NULL;
  worker->req_queue = NULL;
  worker->res_queue = NULL;
  worker->req_cb = NULL;

  free(worker);
}

saba_err_t saba_worker_start(saba_worker_t *worker) {
  TRACE("worker=%p\n", worker);

  int32_t ret = uv_thread_create(&worker->tid, do_work, worker);
  assert(ret == 0);

  return (saba_err_t)ret;
}

saba_err_t saba_worker_stop(saba_worker_t *worker) {
  TRACE("worker=%p\n", worker);

  if (worker->state == SABA_WORKER_STATE_STOP ||
      worker->state == SABA_WORKER_STATE_INIT) {
    return SABA_ERR_OK;
  }

  int32_t ret = uv_async_send(&worker->stop_notifier);
  if (ret) {
    uv_err_t err = uv_last_error(worker->stop_notifier.loop);
    SABA_LOGGER_LOG(
      worker->logger, worker->loop, on_worker_log, ERROR,
      "send a siginal to stop notify error: %s (%d)\n", uv_strerror(err), err.code
    );
    return SABA_ERR_NG;
  }
  assert(ret == 0);

  ret = uv_thread_join(&worker->tid);
  assert(ret == 0);
  TRACE("uv_thread_join: ret=%d\n", ret);

  return (saba_err_t)ret;
}


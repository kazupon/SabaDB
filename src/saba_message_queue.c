/*
 * SabaDB: message queue module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_message_queue.h"
#include "debug.h"
#include <assert.h>
#include <stdlib.h>


saba_message_queue_t* saba_message_queue_alloc(void) {
  saba_message_queue_t *msg_q = (saba_message_queue_t *)malloc(sizeof(saba_message_queue_t));
  assert(msg_q != NULL);
  TRACE("msg_q =%p\n", msg_q);

  ngx_queue_init(&msg_q->queue);
  uv_mutex_init(&msg_q->mtx);

  return msg_q;
}

void saba_message_queue_free(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);
  TRACE("msg_q=%p\n", msg_q);

  saba_message_queue_clear(msg_q);

  uv_mutex_lock(&msg_q->mtx);
  free(msg_q);
  uv_mutex_unlock(&msg_q->mtx);

  uv_mutex_destroy(&msg_q->mtx);
}

bool saba_message_queue_is_empty(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);
  TRACE("msg_q=%p\n", msg_q);

  uv_mutex_lock(&msg_q->mtx);
  bool ret = ngx_queue_empty(&msg_q->queue);
  uv_mutex_unlock(&msg_q->mtx);
  return ret;
}

saba_message_t* saba_message_queue_head(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);
  TRACE("msg_q=%p\n", msg_q);

  if (saba_message_queue_is_empty(msg_q)) {
    return NULL;
  }
  
  uv_mutex_lock(&msg_q->mtx);
  ngx_queue_t *head = ngx_queue_head(&msg_q->queue);
  assert(head != NULL);
  saba_message_t *msg = ngx_queue_data(head, saba_message_t, q);
  assert(msg != NULL);
  uv_mutex_unlock(&msg_q->mtx);

  return msg;
}

saba_message_t* saba_message_queue_tail(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);
  TRACE("msg_q=%p\n", msg_q);

  if (saba_message_queue_is_empty(msg_q)) {
    return NULL;
  }

  uv_mutex_lock(&msg_q->mtx);
  ngx_queue_t *tail = ngx_queue_last(&msg_q->queue);
  assert(tail!= NULL);
  saba_message_t *msg = ngx_queue_data(tail, saba_message_t, q);
  assert(msg != NULL);
  uv_mutex_unlock(&msg_q->mtx);

  return msg;
}

void saba_message_queue_insert_head(saba_message_queue_t *msg_q, saba_message_t *msg) {
  assert(msg_q != NULL && msg != NULL);
  TRACE("msg_q=%p, msg=%p\n", msg_q, msg);

  uv_mutex_lock(&msg_q->mtx);
  ngx_queue_insert_head(&msg_q->queue, &msg->q);
  uv_mutex_unlock(&msg_q->mtx);
}

void saba_message_queue_insert_tail(saba_message_queue_t *msg_q, saba_message_t *msg) {
  assert(msg_q != NULL && msg != NULL);
  TRACE("msg_q=%p, msg=%p\n", msg_q, msg);

  uv_mutex_lock(&msg_q->mtx);
  ngx_queue_insert_tail(&msg_q->queue, &msg->q);
  uv_mutex_unlock(&msg_q->mtx);
}

void saba_message_queue_remove(saba_message_queue_t *msg_q, saba_message_t *msg) {
  assert(msg_q != NULL && msg != NULL);
  TRACE("msg_q=%p, msg=%p\n", msg_q, msg);

  uv_mutex_lock(&msg_q->mtx);
  ngx_queue_remove(&msg->q);
  uv_mutex_unlock(&msg_q->mtx);
}

void saba_message_queue_clear(saba_message_queue_t *msg_q) {
  assert(msg_q != NULL);
  TRACE("msg_q=%p\n", msg_q);

  if (!saba_message_queue_is_empty(msg_q)) {
    uv_mutex_lock(&msg_q->mtx);
    ngx_queue_t *q;
    saba_message_t *msg = NULL;
    ngx_queue_foreach(q, &msg_q->queue) {
      msg = ngx_queue_data(q, saba_message_t, q);
      assert(msg != NULL);
      ngx_queue_remove(&msg->q);
    }
    uv_mutex_unlock(&msg_q->mtx);
  }
}


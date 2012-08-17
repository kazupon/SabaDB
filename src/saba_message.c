/*
 * SabaDB: message module
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "saba_message.h"
#include "debug.h"
#include <assert.h>
#include <stdlib.h>


saba_message_t* saba_message_alloc(void) {
  saba_message_t *msg = (saba_message_t *)malloc(sizeof(saba_message_t));
  assert(msg != NULL);

  msg->kind = SABA_MESSAGE_KIND_UNKNOWN;
  msg->stream = NULL;
  msg->data = NULL;

  return msg;
}

void saba_message_free(saba_message_t *msg) {
  assert(msg != NULL);
  msg->stream = NULL;
  free(msg);
}


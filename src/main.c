/*
 * main
 * Copyright (C) 2012 kazuya kawaguchi & frapwings. See Copyright Notice in main.h
 */

#include "main.h"
#include "debug.h"
#include <stdio.h>
#include <uv.h>
#include <kclangc.h>


int main () {
  uv_loop_t *loop = uv_default_loop();
  TRACE("hello libuv\n");
  KCDB *db = kcdbnew();
  TRACE("hello kyotocabinet for c lang\n");
  kcdbdel(db);
  uv_run(loop);
  return 0;
}

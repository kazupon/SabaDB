#include <stdio.h>
#include <uv.h>
#include <kclangc.h>


int main () {
  uv_loop_t *loop = uv_default_loop();
  printf("hello libuv\n");
  KCDB *db = kcdbnew();
  printf("hello kyotocabinet for c lang\n");
  kcdbdel(db);
  uv_run(loop);
  return 0;
}

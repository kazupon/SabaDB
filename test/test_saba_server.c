#include "test_saba_server.h"
#include "saba_common.h"
#include "saba_message_queue.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <unistd.h>
#include <uv.h>


static uv_timer_t stop_timer;
static saba_err_t stop_ret = 0;

void on_stop_timer(uv_timer_t *timer, int status) {
  saba_server_t *server = (saba_server_t *)timer->data;
  stop_ret = saba_server_stop(server);
  uv_timer_stop(timer);
  uv_close((uv_handle_t *)timer, NULL);
}

void test_saba_server_alloc_and_free(void) {
  saba_server_t *server = saba_server_alloc();

  CU_ASSERT_PTR_NOT_NULL(server);
  CU_ASSERT_PTR_NOT_NULL(server->workers);
  CU_ASSERT_PTR_NOT_NULL(server->req_queue);
  CU_ASSERT_PTR_NOT_NULL(server->res_queue);

  saba_server_free(server);
}

void test_saba_server_start_and_stop(void) {
  uv_loop_t *loop = uv_default_loop();
  saba_server_t *server = saba_server_alloc();

  uv_timer_init(loop, &stop_timer);
  uv_timer_start(&stop_timer, on_stop_timer, 100, 1);
  stop_timer.data = server;

  CU_ASSERT_EQUAL(saba_server_start(server, loop, "0.0.0.0", 1978), SABA_ERR_OK);

  uv_run(loop);
  CU_ASSERT_EQUAL(stop_ret, SABA_ERR_OK);

  saba_server_free(server);
}

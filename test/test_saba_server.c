#include "test_saba_server.h"
#include "debug.h"
#include "saba_utils.h"
#include "saba_common.h"
#include "saba_message_queue.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <unistd.h>
#include <uv.h>


typedef struct {
  uv_tcp_t tcp;
  uv_connect_t connection;
  saba_server_t *server;
  uint64_t count;
} echo_t;

static uint64_t ECHO_COUNT;
static uv_timer_t stop_timer;
static uv_timer_t echo_timer;
static saba_err_t stop_ret = 0;


/*
 * handlers
 */

void on_stop_timer(uv_timer_t *timer, int status) {
  TRACE("timer=%p, status=%d\n", timer, status);
  saba_server_t *server = (saba_server_t *)timer->data;

  stop_ret = saba_server_stop(server);
  uv_timer_stop(timer);
  uv_close((uv_handle_t *)timer, NULL);
  timer->data = NULL;
}

uv_buf_t on_alloc_buf(uv_handle_t *tcp, size_t size) {
  TRACE("tcp=%p, size=%zd\n", tcp, size);
  return uv_buf_init(malloc(size), size);
}

void on_read(uv_stream_t *tcp, ssize_t nread, uv_buf_t buf) {
  TRACE("tcp=%p, nread=%zd, buf=%p, buf.base=%s\n", tcp, nread, &buf, buf.base);
  echo_t *echo = container_of(tcp, echo_t, tcp);

  if (nread < 0) {
    CU_ASSERT(uv_last_error(tcp->loop).code == UV_EOF);
    uv_timer_stop(&echo_timer);
    saba_server_t *server = (saba_server_t *)echo->server;
    saba_server_stop(server);
    uv_close((uv_handle_t *)tcp, NULL);
    return;
  }

  if (!strcmp("hello", buf.base)) {
    echo->count++;
    saba_server_t *server = echo->server;
    TRACE("count=%lld\n", echo->count);
    if (echo->count == ECHO_COUNT) {
      uv_timer_stop(&echo_timer);
      saba_server_t *server = (saba_server_t *)echo->server;
      saba_server_stop(server);
      uv_close((uv_handle_t *)tcp, NULL);
    }
  }
}

void on_write(uv_write_t *req, int status) {
  TRACE("req=%p, status=%d\n", req, status);
  free(req);
}

void on_write_timer(uv_timer_t *timer, int status) {
  TRACE("timer=%p, status=%d\n", timer, status);
  echo_t *echo = (echo_t *)timer->data;

  uv_buf_t buf;
  buf.base = "hello";
  buf.len = strlen(buf.base);
  uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
  if (uv_write(req, (uv_stream_t *)&echo->tcp, &buf, 1, on_write)) {
    CU_FAIL("uv_write failed\n");
  }
}

void on_connect(uv_connect_t *connection, int status) {
  TRACE("connection=%p, status=%d\n", connection, status);
  echo_t *echo = container_of(connection, echo_t, connection);

  if (uv_read_start(connection->handle, on_alloc_buf, on_read)) {
    CU_FAIL("uv_read_start failed\n");
  }
}


/*
 * tests
 */
void test_saba_server_alloc_and_free(void) {
  int32_t worker_num = 2;
  saba_server_t *server = saba_server_alloc(worker_num);

  CU_ASSERT_PTR_NOT_NULL(server);
  CU_ASSERT_PTR_NOT_NULL(server->req_queue);
  CU_ASSERT_PTR_NOT_NULL(server->res_queue);

  saba_server_free(server);
}

void test_saba_server_start_and_stop(void) {
  uv_loop_t *loop = uv_default_loop();
  int32_t worker_num = 1;
  saba_server_t *server = saba_server_alloc(worker_num);

  uv_timer_init(loop, &stop_timer);
  uv_timer_start(&stop_timer, on_stop_timer, 100, 1);
  stop_timer.data = server;

  CU_ASSERT_EQUAL(saba_server_start(server, loop, "0.0.0.0", 1978), SABA_ERR_OK);

  uv_run(loop);
  CU_ASSERT_EQUAL(stop_ret, SABA_ERR_OK);

  saba_server_free(server);
}

void echo(int32_t worker_num, int32_t echo_count) {
  uv_loop_t *loop = uv_default_loop();
  ECHO_COUNT = echo_count;
  saba_server_t *server = saba_server_alloc(worker_num);
  saba_server_start(server, loop, "0.0.0.0", 1978);

  /* setup client */
  echo_t echo;
  struct sockaddr_in client_addr = uv_ip4_addr("0.0.0.0", 0);
  struct sockaddr_in server_addr = uv_ip4_addr("127.0.0.1", 1978);
  uv_tcp_init(loop, &echo.tcp);
  uv_tcp_bind(&echo.tcp, client_addr);
  uv_tcp_connect(&echo.connection, &echo.tcp, server_addr, on_connect);
  echo.count = 0;
  echo.server = server;

  uv_timer_init(loop, &echo_timer);
  uv_timer_start(&echo_timer, on_write_timer, 10, 5);
  echo_timer.data = (void *)&echo;

  uv_run(loop);

  CU_ASSERT_EQUAL(echo.count, ECHO_COUNT);
  saba_server_free(server);
}

void test_saba_server_echo(void) {
  uint64_t time;
  uint64_t echos = 2000;

  time = uv_hrtime();
  echo(1, echos);
  time = uv_hrtime() - time;
  printf("worker: 1, echo: %lld, %.2f secounds %.3f(roundtrips/s)\n", echos, (time / 1e9), (echos / (time /1e9)));

  time = uv_hrtime();
  echo(5, echos);
  time = uv_hrtime() - time;
  printf("worker: 5, echo: %lld, %.2f secounds %.3f(roundtrips/s)\n", echos, (time / 1e9), (echos / (time /1e9)));
}


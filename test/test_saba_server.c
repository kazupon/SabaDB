#include "test_saba_server.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uv.h>
#include "debug.h"
#include "saba_utils.h"
#include "saba_common.h"
#include "saba_message_queue.h"
#include "saba_logger.h"


static void on_write_timer(uv_timer_t *handle, int status);


typedef struct {
  uv_tcp_t tcp;
  uv_connect_t connection;
  saba_server_t *server;
  uint64_t count;
} echo_t;

typedef struct {
  saba_logger_t *logger;
  saba_server_t *server;
  uv_tcp_t tcp;
  uv_connect_t connection;
  uint64_t count;
} cu_test_server_t;

typedef struct {
  uv_write_t req;
  uv_buf_t buf;
} cu_write_req_t;

static const char *PATH = "./hoge.log";
static uint64_t ECHO_COUNT;
static cu_test_server_t data;
static uv_idle_t bootstraper;
static uv_timer_t echo_timer;


static void on_setup(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_server_t *d = (cu_test_server_t *)handle->data;
  saba_err_t ret = saba_logger_open(d->logger, handle->loop, PATH, NULL);
  uv_close((uv_handle_t *)handle, NULL);
}

int test_saba_server_setup(void) {
  TRACE("\n");
  unlink(PATH);
  data.logger = saba_logger_alloc();
  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_setup);
  int32_t ret = uv_run(loop);
  if (ret) {
    saba_logger_free(data.logger);
    data.logger = NULL;
    return 1;
  }
  return 0;
}


static void on_teardown(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_server_t *d = (cu_test_server_t *)handle->data;
  saba_err_t ret = saba_logger_close(d->logger, handle->loop, NULL);
  uv_close((uv_handle_t *)handle, NULL);
}

int test_saba_server_teardown(void) {
  TRACE("\n");
  uv_loop_t *loop = uv_default_loop();
  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_teardown);
  int32_t ret = uv_run(loop);
  saba_logger_free(data.logger);
  data.logger = NULL;
  unlink(PATH);
  return ret;
}


void test_saba_server_alloc_and_free(void) {
  TRACE("\n");
  int32_t worker_num = 2;
  saba_server_t *server = saba_server_alloc(worker_num);

  CU_ASSERT_PTR_NOT_NULL(server);
  CU_ASSERT_PTR_NOT_NULL(server->master);
  CU_ASSERT_PTR_NULL(server->loop);
  CU_ASSERT_PTR_NULL(server->db);

  saba_server_free(server);
}


static void on_server_stop(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_server_t *d = (cu_test_server_t *)handle->data;

  usleep(1000 * 100);

  saba_err_t ret = saba_server_stop(d->server);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  uv_close((uv_handle_t *)handle, NULL);
}

static void on_test_saba_server_start_and_stop(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_server_t *d = (cu_test_server_t *)handle->data;

  saba_err_t ret = saba_server_start(d->server, handle->loop, d->logger, "0.0.0.0", 1978);
  CU_ASSERT_EQUAL(ret, SABA_ERR_OK);

  uv_idle_start(handle, on_server_stop);
}

void test_saba_server_start_and_stop(void) {
  TRACE("\n");
  uv_loop_t *loop = uv_default_loop();
  int32_t worker_num = 1;
  data.server = saba_server_alloc(worker_num);

  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_test_saba_server_start_and_stop);

  uv_run(loop);

  saba_server_free(data.server);
}


static uv_buf_t on_alloc_buf(uv_handle_t *tcp, size_t size) {
  TRACE("tcp=%p, size=%zd\n", tcp, size);
  return uv_buf_init(malloc(size), size);
}

static void on_read(uv_stream_t *tcp, ssize_t nread, uv_buf_t buf) {
  TRACE("tcp=%p, nread=%zd, buf=%p\n", tcp, nread, &buf);
  cu_test_server_t *d = container_of(tcp, cu_test_server_t, tcp);

  if (nread < 0) {
    CU_ASSERT(uv_last_error(tcp->loop).code == UV_EOF);
    uv_timer_stop(&echo_timer);
    saba_server_stop(d->server);
    uv_close((uv_handle_t *)tcp, NULL);
    uv_close((uv_handle_t *)&echo_timer, NULL);
  } else {
    d->count++;
    TRACE("count=%lld\n", d->count);
    if (d->count == ECHO_COUNT) {
      uv_timer_stop(&echo_timer);
      saba_server_stop(d->server);
      uv_close((uv_handle_t *)tcp, NULL);
      uv_close((uv_handle_t *)&echo_timer, NULL);
    }
  }

  if (buf.base) {
    free(buf.base);
  }
}

static void on_write(uv_write_t *req, int status) {
  TRACE("req=%p, status=%d\n", req, status);

  cu_write_req_t *r = container_of(req, cu_write_req_t, req);
  if (r->buf.base) {
    free(r->buf.base);
  }
  free(r);

  uv_timer_start(&echo_timer, on_write_timer, 1, 5);
}

static void on_write_timer(uv_timer_t *handle, int status) {
  TRACE("handle=%p, status=%d\n", handle, status);
  cu_test_server_t *d = (cu_test_server_t *)handle->data;
  uv_timer_stop(handle);

  cu_write_req_t *req = (cu_write_req_t *)malloc(sizeof(cu_write_req_t));
  req->buf.base = strdup("hello");
  req->buf.len = strlen(req->buf.base);
  if (uv_write(&req->req, (uv_stream_t *)&d->tcp, &req->buf, 1, on_write)) {
    CU_FAIL("uv_write failed\n");
  }
}

static void on_connect(uv_connect_t *connection, int status) {
  TRACE("connection=%p, status=%d\n", connection, status);
  cu_test_server_t *d = container_of(connection, cu_test_server_t, connection);

  if (uv_read_start(connection->handle, on_alloc_buf, on_read)) {
    CU_FAIL("uv_read_start failed\n");
  }

  uv_timer_init(connection->handle->loop, &echo_timer);
  echo_timer.data = d;
  uv_timer_start(&echo_timer, on_write_timer, 1, 5);
}

static void on_boot_echo(uv_idle_t *handle, int status) {
  uv_idle_stop(handle);
  cu_test_server_t *d = (cu_test_server_t *)handle->data;

  saba_server_start(d->server, handle->loop, d->logger, "0.0.0.0", 1978);
  
  /* setup client */
  struct sockaddr_in client_addr = uv_ip4_addr("0.0.0.0", 0);
  struct sockaddr_in server_addr = uv_ip4_addr("127.0.0.1", 1978);
  uv_tcp_init(handle->loop, &d->tcp);
  uv_tcp_bind(&d->tcp, client_addr);
  uv_tcp_connect(&d->connection, &d->tcp, server_addr, on_connect);
  d->count = 0;
  
  uv_close((uv_handle_t *)handle, NULL);
}

static void echo(int32_t worker_num, int32_t echo_count) {
  uv_loop_t *loop = uv_default_loop();
  data.server = saba_server_alloc(worker_num);

  ECHO_COUNT = echo_count;
  uv_idle_init(loop, &bootstraper);
  bootstraper.data = &data;
  uv_idle_start(&bootstraper, on_boot_echo);

  uv_run(loop);

  CU_ASSERT_EQUAL(data.count, ECHO_COUNT);
  saba_server_free(data.server);
}

void test_saba_server_echo(void) {
  TRACE("\n");
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


#include "test_memtext.h"
#include "memtext.h"
#include "debug.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdio.h>


typedef struct {
  int32_t get_called;
  int32_t gets_called;
  int32_t set_called;
  int32_t add_called;
  int32_t replace_called;
  int32_t append_called;
  int32_t prepend_called;
  int32_t cas_called;
  int32_t delete_called;
  int32_t incr_called;
  int32_t decr_called;
  int32_t version_called;
} cu_memtext_test_t;


static int on_get(void *user, memtext_command cmd, memtext_request_retrieval *req);
static int on_gets(void *user, memtext_command cmd, memtext_request_retrieval *req);
static int on_set(void *user, memtext_command cmd, memtext_request_storage *req);
static int on_add(void *user, memtext_command cmd, memtext_request_storage *req);
static int on_replace(void *user, memtext_command cmd, memtext_request_storage *req);
static int on_append(void *user, memtext_command cmd, memtext_request_storage *req);
static int on_prepend(void *user, memtext_command cmd, memtext_request_storage *req);
static int on_cas(void *user, memtext_command cmd, memtext_request_cas *req);
static int on_delete(void *user, memtext_command cmd, memtext_request_delete* req);
static int on_incr(void *user, memtext_command cmd, memtext_request_numeric* req);
static int on_decr(void *user, memtext_command cmd, memtext_request_numeric* req);
static int on_version(void *user, memtext_command cmd, memtext_request_other* req);


static memtext_parser parser;
static memtext_callback callbacks = {
  on_get, // get
  on_gets, // gets
  on_set, // set
  on_add, // add
  on_replace, // replace
  on_append, // append
  on_prepend, // prepend
  on_cas, // cas
  on_delete, // delete
  on_incr, // incr
  on_decr, // decr
  on_version, // version
};
static cu_memtext_test_t data;



int test_memtext_setup(void) {
  data.get_called = 0;
  data.gets_called = 0;
  data.set_called = 0;
  data.add_called = 0;
  data.replace_called = 0;
  data.append_called = 0;
  data.prepend_called = 0;
  data.cas_called = 0;
  data.incr_called = 0;
  data.decr_called = 0;
  data.version_called = 0;
	memtext_init(&parser, &callbacks, &data);
  return 0;
}

int test_memtext_teardown(void) {
  return 0;
}


static int on_get(void *user, memtext_command cmd, memtext_request_retrieval *req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->get_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_GET);
  CU_ASSERT_PTR_NOT_NULL(req);
  int i;
  for (i = 0; i < req->key_num; i++) {
    printf("get key:");
    fwrite(req->key[i], req->key_len[i], 1, stdout);
    printf("\n");
  }
  return 0;
}

static int on_gets(void *user, memtext_command cmd, memtext_request_retrieval *req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->gets_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_GETS);
  CU_ASSERT_PTR_NOT_NULL(req);
  int i;
  for (i = 0; i < req->key_num; i++) {
    printf("gets key:");
    fwrite(req->key[i], req->key_len[i], 1, stdout);
    printf("\n");
  }
  return 0;
}

static int on_set(void *user, memtext_command cmd, memtext_request_storage *req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->set_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_SET);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

static int on_add(void *user, memtext_command cmd, memtext_request_storage *req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->add_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_ADD);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

static int on_replace(void *user, memtext_command cmd, memtext_request_storage *req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->replace_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_REPLACE);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

static int on_append(void *user, memtext_command cmd, memtext_request_storage *req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->append_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_APPEND);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

static int on_prepend(void *user, memtext_command cmd, memtext_request_storage *req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->prepend_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_PREPEND);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

static int on_cas(void *user, memtext_command cmd, memtext_request_cas *req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->cas_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_CAS);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

static int on_delete(void *user, memtext_command cmd, memtext_request_delete* req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->delete_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_DELETE);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

static int on_incr(void *user, memtext_command cmd, memtext_request_numeric* req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->incr_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_INCR);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

static int on_decr(void *user, memtext_command cmd, memtext_request_numeric* req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->decr_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_DECR);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

static int on_version(void *user, memtext_command cmd, memtext_request_other* req) {
  CU_ASSERT_PTR_NOT_NULL(user);
  cu_memtext_test_t *d = (cu_memtext_test_t *)user;
  d->version_called++;
  CU_ASSERT_EQUAL(cmd, MEMTEXT_CMD_VERSION);
  CU_ASSERT_PTR_NOT_NULL(req);
  return 0;
}

void test_memtext_parse(void) {
	const char buf[] =
		"get bar\r\n"
		"get hoge bar baz\r\n"
		"gets bar\r\n"
		"gets hoge bar baz\r\n"
		"set foo 0 0 2 noreply\r\nax\r\n"
		"set foo 0 0 2 noreply\r\nax\r\n"
    "add foo 0 0 2\r\n10\r\n"
    "replace foo 0 0 2\r\n10\r\n"
    "append foo 0 0 2\r\n10\r\n"
    "prepend foo 0 0 2\r\n10\r\n"
		"cas baz 0 0 2 1152921504606846976\r\nax\r\n"
		"delete bar\r\n"
		"delete bar 10\r\n"
		"delete bar noreply\r\n"
		"delete bar 10 noreply\r\n"
    "incr foo 3\r\n"
    "decr foo 3\r\n"
    /*
    "version\r\n"
    */
  ;
  size_t off = 0;
  int ret = memtext_execute(&parser, buf, strlen(buf), &off);
  CU_ASSERT(ret);
  CU_ASSERT_EQUAL(data.get_called, 2);
  CU_ASSERT_EQUAL(data.gets_called, 2);
  CU_ASSERT_EQUAL(data.set_called, 2);
  CU_ASSERT_EQUAL(data.add_called, 1);
  CU_ASSERT_EQUAL(data.replace_called, 1);
  CU_ASSERT_EQUAL(data.append_called, 1);
  CU_ASSERT_EQUAL(data.prepend_called, 1);
  CU_ASSERT_EQUAL(data.cas_called, 1);
  CU_ASSERT_EQUAL(data.delete_called, 4);
  CU_ASSERT_EQUAL(data.incr_called, 1);
  CU_ASSERT_EQUAL(data.decr_called, 1);
  /*
  CU_ASSERT_EQUAL(data.version_called, 1);
  */
}


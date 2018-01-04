/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "mem.h"
#include "eventpool.h"

static uv_thread_t pth;

struct data_t {
	int a;
} data_t;

static uv_async_t *async_close_req = NULL;
static int check = 0;
static int i = 0;
static int x = 0;
static int y = 0;

static void close_cb(uv_handle_t *handle) {
	FREE(handle);
}

static void *done(void *param) {
	FREE(param);
	return NULL;
}

static void *listener1(int reason, void *param) {
	struct data_t *data = param;
	printf("listener1: %d\n", data->a);

	if(data->a == 5) {
		uv_async_send(async_close_req);
	}
	return NULL;
}

static void *listener2(int reason, void *param) {
	struct data_t *data = param;
	printf("listener2: %d\n", data->a);

	return NULL;
}

static void *listener3(int reason, void *param) {
	struct data_t *data = param;
	printf("listener3: %d\n", data->a);

	struct data_t *tmp = MALLOC(sizeof(struct data_t));
	if(tmp == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	tmp->a = data->a;
	eventpool_trigger(REASON_TEST1, done, tmp);

	return NULL;
}

static void walk_cb(uv_handle_t *handle, void *arg) {
	if(!uv_is_closing(handle)) {
		uv_close(handle, close_cb);
	}
}

static void loop(void *param) {
	i = 0, x = 0,	y = 0, check = 0;

	for(i=0;i<6;i++) {
		struct data_t *tmp = MALLOC(sizeof(struct data_t));
		if(tmp == NULL) {
			OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
		}
		tmp->a = i;
		switch(i) {
			case 0:
				eventpool_trigger(REASON_TEST1, done, tmp);
			break;
			case 1:
				eventpool_trigger(REASON_TEST1, done, tmp);
			break;
			case 2: {
				eventpool_trigger(REASON_TEST1, done, tmp);
			} break;
			case 3: {
				eventpool_trigger(REASON_TEST1, done, tmp);
			} break;
			case 4: {
				eventpool_trigger(REASON_TEST2, done, tmp);
			} break;
			case 5: {
				eventpool_trigger(REASON_TEST3, done, tmp);
			} break;
		}
	}
	return;
}

static void async_close_cb(uv_async_t *handle) {
	if(!uv_is_closing((uv_handle_t *)handle)) {
		uv_close((uv_handle_t *)handle, close_cb);
	}
	uv_stop(uv_default_loop());
}

int main(void) {
	async_close_req = MALLOC(sizeof(uv_async_t));
	if(async_close_req == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	uv_async_init(uv_default_loop(), async_close_req, async_close_cb);

	eventpool_init(EVENTPOOL_THREADED);
	eventpool_callback(REASON_TEST1, listener1);

	eventpool_callback(REASON_TEST2, listener1);
	eventpool_callback(REASON_TEST2, listener2);

	eventpool_callback(REASON_TEST3, listener3);

	uv_thread_create(&pth, loop, NULL);

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_walk(uv_default_loop(), walk_cb, NULL);
	uv_run(uv_default_loop(), UV_RUN_ONCE);

	while(uv_loop_close(uv_default_loop()) == UV_EBUSY) {
		uv_run(uv_default_loop(), UV_RUN_ONCE);
	}

	uv_thread_join(&pth);

	eventpool_gc();
}
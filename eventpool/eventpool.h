/*
	Copyright (C) 2015 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _EVENTPOOL_H_
#define _EVENTPOOL_H_

#include <uv.h>

enum eventpool_threads_t {
	EVENTPOOL_NO_THREADS,
	EVENTPOOL_THREADED
} eventpool_threads_t;

#define REASON_TEST										0
#define REASON_TEST1									1
#define REASON_TEST2									2
#define REASON_TEST3									3
#define REASON_END										4

typedef struct threadpool_data_t {
	int reason;
	int priority;
	char name[255];
	uv_sem_t *ref;
	void *userdata;
	void *(*func)(int, void *);
	void *(*done)(void *);
} threadpool_data_t;

typedef struct threadpool_tasks_t {
	unsigned long id;
	char *name;
	void *(*func)(int, void *);
	void *(*done)(void *);
	uv_sem_t *ref;
	int priority;
	int reason;

	struct {
		struct timespec first;
		struct timespec second;
	}	timestamp;

	void *userdata;

	struct threadpool_tasks_t *next;
} threadpool_tasks_t;

typedef struct eventpool_listener_t {
	void *(*func)(int, void *);
	void *userdata;
	int reason;
	struct eventpool_listener_t *next;
} eventpool_listener_t;

void eventpool_callback(int, void *(*)(int, void *));
void eventpool_trigger(int, void *(*)(void *), void *);
void eventpool_init(enum eventpool_threads_t);
int eventpool_gc(void);

#endif

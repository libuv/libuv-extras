/*
	Copyright (C) 2015 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <uv.h>

#include "eventpool.h"
#include "mem.h"

static uv_async_t *async_req = NULL;

struct eventqueue_t {
	int reason;
	void *(*done)(void *);
	void *data;
	struct eventqueue_t *next;
} eventqueue_t;

#ifdef _WIN32
static volatile long nrlisteners[REASON_END] = {0};
#else
static int nrlisteners[REASON_END] = {0};
#endif
static struct eventqueue_t *eventqueue = NULL;

static int threads = EVENTPOOL_NO_THREADS;
static uv_mutex_t listeners_lock;
static int lockinit = 0;
static int eventpoolinit = 0;

static struct eventpool_listener_t *eventpool_listeners = NULL;

static struct reasons_t {
	int number;
	char *reason;
	int priority;
} reasons[REASON_END+1] = {
	{	REASON_TEST, "REASON_TEST", 0 },
	{	REASON_TEST1, "REASON_TEST1", 0 },
	{	REASON_TEST2, "REASON_TEST2", 0 },
	{	REASON_TEST3, "REASON_TEST3", 0 },
	{	REASON_END, "REASON_END", 0 }
};

static void fib_free(uv_work_t *req, int status) {
	FREE(req->data);
	FREE(req);
}

static void fib(uv_work_t *req) {
	struct threadpool_data_t *data = req->data;

	data->func(data->reason, data->userdata);

	int x = 0;
	if(data->ref != NULL) {
		x = uv_sem_trywait(data->ref);
	}
	if((data->ref == NULL) || (x == UV__EAGAIN)) {
		if(data->done != NULL && data->reason != REASON_END) {
			data->done(data->userdata);
		}
		if(data->ref != NULL) {
			FREE(data->ref);
		}
	}
}

void eventpool_callback(int reason, void *(*func)(int, void *)) {
	if(lockinit == 1) {
		uv_mutex_lock(&listeners_lock);
	}

	struct eventpool_listener_t *node = MALLOC(sizeof(struct eventpool_listener_t));
	if(node == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	node->func = func;
	node->reason = reason;
	node->next = NULL;

	node->next = eventpool_listeners;
	eventpool_listeners = node;

#ifdef _WIN32
	InterlockedIncrement(&nrlisteners[reason]);
#else
	__sync_add_and_fetch(&nrlisteners[reason], 1);
#endif

	if(lockinit == 1) {
		uv_mutex_unlock(&listeners_lock);
	}
}

void eventpool_trigger(int reason, void *(*done)(void *), void *data) {
	if(eventpoolinit == 0) {
		return;
	}

#ifdef _WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#else
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	sched.sched_priority = 80;
	pthread_setschedparam(pthread_self(), SCHED_RR, &sched);
#endif
	int eventqueue_size = 0;

	struct eventqueue_t *node = MALLOC(sizeof(struct eventqueue_t));
	if(node == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	memset(node, 0, sizeof(struct eventqueue_t));
	node->reason = reason;
	node->done = done;
	node->data = data;

	uv_mutex_lock(&listeners_lock);
	struct eventqueue_t *tmp = eventqueue;
	if(tmp != NULL) {
		while(tmp->next != NULL) {
			eventqueue_size++;
			tmp = tmp->next;
		}
		tmp->next = node;
		node = tmp;
	} else {
		node->next = eventqueue;
		eventqueue = node;
	}

	/*
	 * If the eventqueue size is above
	 * 50 entries then there must be a bug
	 * at the trigger side.
	 */
	assert(eventqueue_size < 50);

	uv_mutex_unlock(&listeners_lock);

	uv_async_send(async_req);
}

static void eventpool_execute(uv_async_t *handle) {
	struct threadpool_tasks_t **node = NULL;
	int nrlisteners1[REASON_END] = {0};
	int nr1 = 0, nrnodes = 16, nrnodes1 = 0, i = 0;

	if((node = MALLOC(sizeof(struct threadpool_tasks_t *)*nrnodes)) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}

	uv_mutex_lock(&listeners_lock);

	struct eventqueue_t *queue = NULL;
	while(eventqueue) {
		queue = eventqueue;
		uv_sem_t *ref = NULL;

#ifdef _WIN32
		if((nr1 = InterlockedExchangeAdd(&nrlisteners[queue->reason], 0)) == 0) {
#else
		if((nr1 = __sync_add_and_fetch(&nrlisteners[queue->reason], 0)) == 0) {
#endif
			if(queue->done != NULL) {
				queue->done((void *)queue->data);
			}
		} else {
			if(threads == EVENTPOOL_THREADED) {
				if((ref = MALLOC(sizeof(uv_sem_t))) == NULL) {
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				uv_sem_init(ref, nr1-1);
			}

			struct eventpool_listener_t *listeners = eventpool_listeners;
			if(listeners == NULL) {
				if(queue->done != NULL) {
					queue->done((void *)queue->data);
				}
			}

			while(listeners) {
				if(listeners->reason == queue->reason) {
					if(nrnodes1 == nrnodes) {
						nrnodes *= 2;
						/*LCOV_EXCL_START*/
						if((node = REALLOC(node, sizeof(struct threadpool_tasks_t *)*nrnodes)) == NULL) {
							OUT_OF_MEMORY
						}
						/*LCOV_EXCL_STOP*/
					}
					if((node[nrnodes1] = MALLOC(sizeof(struct threadpool_tasks_t))) == NULL) {
						OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
					}
					node[nrnodes1]->func = listeners->func;
					node[nrnodes1]->userdata = queue->data;
					node[nrnodes1]->done = queue->done;
					node[nrnodes1]->ref = ref;
					node[nrnodes1]->reason = listeners->reason;
					nrnodes1++;
					if(threads == EVENTPOOL_THREADED) {
						nrlisteners1[queue->reason]++;
					}
				}
				listeners = listeners->next;
			}
		}
		eventqueue = eventqueue->next;
		FREE(queue);
	}
	uv_mutex_unlock(&listeners_lock);

	if(nrnodes1 > 0) {
		for(i=0;i<nrnodes1;i++) {
			if(threads == EVENTPOOL_NO_THREADS) {
				nrlisteners1[node[i]->reason]++;
				node[i]->func(node[i]->reason, node[i]->userdata);

#ifdef _WIN32
				if(nrlisteners1[node[i]->reason] == InterlockedExchangeAdd(&nrlisteners[node[i]->reason], 0)) {
#else
				if(nrlisteners1[node[i]->reason] == __sync_add_and_fetch(&nrlisteners[node[i]->reason], 0)) {
#endif
					if(node[i]->done != NULL) {
						node[i]->done((void *)node[i]->userdata);
					}
					nrlisteners1[node[i]->reason] = 0;
				}
			} else {
				struct threadpool_data_t *tpdata = NULL;
				tpdata = MALLOC(sizeof(struct threadpool_data_t));
				if(tpdata == NULL) {
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				tpdata->userdata = node[i]->userdata;
				tpdata->func = node[i]->func;
				tpdata->done = node[i]->done;
				tpdata->ref = node[i]->ref;
				tpdata->reason = node[i]->reason;
				tpdata->priority = reasons[node[i]->reason].priority;

				uv_work_t *tp_work_req = MALLOC(sizeof(uv_work_t));
				if(tp_work_req == NULL) {
					OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
				}
				tp_work_req->data = tpdata;
				if(uv_queue_work(uv_default_loop(), tp_work_req, fib, fib_free) < 0) {
					if(node[i]->done != NULL) {
						node[i]->done((void *)node[i]->userdata);
					}
					FREE(tpdata);
					FREE(node[i]->ref);
				}
			}
			FREE(node[i]);
		}
	}
	for(i=0;i<REASON_END;i++) {
		nrlisteners1[i] = 0;
	}
	FREE(node);
	uv_mutex_lock(&listeners_lock);
	if(eventqueue != NULL) {
		uv_async_send(async_req);
	}
	uv_mutex_unlock(&listeners_lock);
}

int eventpool_gc(void) {
	if(lockinit == 1) {
		uv_mutex_lock(&listeners_lock);
	}
	struct eventqueue_t *queue = NULL;
	while(eventqueue) {
		queue = eventqueue;
		if(eventqueue->data != NULL && eventqueue->done != NULL) {
			eventqueue->done(eventqueue->data);
		}
		eventqueue = eventqueue->next;
		FREE(queue);
	}
	struct eventpool_listener_t *listeners = NULL;
	while(eventpool_listeners) {
		listeners = eventpool_listeners;
		eventpool_listeners = eventpool_listeners->next;
		FREE(listeners);
	}
	if(eventpool_listeners != NULL) {
		FREE(eventpool_listeners);
	}
	threads = EVENTPOOL_NO_THREADS;

	int i = 0;
	for(i=0;i<REASON_END;i++) {
		nrlisteners[i] = 0;
	}

	if(lockinit == 1) {
		uv_mutex_unlock(&listeners_lock);
	}
	eventpoolinit = 0;
	return 0;
}

void eventpool_init(enum eventpool_threads_t t) {
	if(eventpoolinit == 1) {
		return;
	}
	eventpoolinit = 1;
	threads = t;

	if((async_req = MALLOC(sizeof(uv_async_t))) == NULL) {
		OUT_OF_MEMORY /*LCOV_EXCL_LINE*/
	}
	uv_async_init(uv_default_loop(), async_req, eventpool_execute);

	if(lockinit == 0) {
		lockinit = 1;
		uv_mutex_init(&listeners_lock);
	}
}

enum eventpool_threads_t eventpool_threaded(void) {
	return threads;
}

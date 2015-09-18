/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <linux/input.h>

#include "minui/minui.h"
#include "timer.h"

struct ui_timer_s
{
	ui_timer_cb_t cb;
	struct timeval next_trigger;
	int delay_ms;
	int enabled;
	int ui_timer;
	void *data;
	struct ui_timer_s *next;
};
struct ui_timer_s *gTimerRoot;
static pthread_mutex_t gTimerMutex = PTHREAD_MUTEX_INITIALIZER;

void restart_timer(ui_timer_t *t, struct timeval *_now) {
	struct timeval now;
	struct timeval delay;
	if (t->enabled == 0)
		return;
	if (_now)
		now = *_now;
	else
		gettimeofday(&now, NULL);
	delay.tv_sec = t->delay_ms/1000;
	delay.tv_usec = (t->delay_ms%1000)*1000;
	timeradd(&now, &delay, &t->next_trigger);
}

/* allocate a timer */
ui_timer_t *ui_alloc_timer(ui_timer_cb_t cb, int ui_timer, void *data)
{
	ui_timer_t *t = calloc(1, sizeof(ui_timer_t));
	if (t != NULL) {
		t->cb = cb;
		t->ui_timer = ui_timer;
		t->data = data;
		pthread_mutex_lock(&gTimerMutex);
		t->next = gTimerRoot;
		gTimerRoot = t;
		pthread_mutex_unlock(&gTimerMutex);
	}
	return t;
}

/* start timer */
void ui_start_timer(ui_timer_t *t, int delay_ms)
{
        pthread_mutex_lock(&gTimerMutex);
	t->enabled = 1;
	t->delay_ms = delay_ms;
	restart_timer(t, NULL);
        pthread_mutex_unlock(&gTimerMutex);
}

/* stop timer */
void ui_stop_timer(ui_timer_t *t)
{
        pthread_mutex_lock(&gTimerMutex);
	t->enabled = 0;
        pthread_mutex_unlock(&gTimerMutex);
}

/* this calls the expired timer, and return the next timer
   or -1 if no timer */
extern int ui_get_screen_state();
int ui_get_next_timer_ms(void)
{
	ui_timer_t *t = gTimerRoot;
	struct timeval next;
	struct timeval tmp;
	struct timeval now;
	int screen_state = ui_get_screen_state();
	int rv = -1;
	next.tv_sec = INT_MAX;
	next.tv_usec = 0;
	gettimeofday(&now, NULL);
        pthread_mutex_lock(&gTimerMutex);
	while(t) {
		if (t->enabled && (t->ui_timer==0 || screen_state)) {
			if (timercmp(&now, &t->next_trigger, >)) {
				int res;
				pthread_mutex_unlock(&gTimerMutex);
				res = t->cb(t->data);
				pthread_mutex_lock(&gTimerMutex);
				t->enabled = res;
				restart_timer(t, &now);
			}
		}
		if (t->enabled && (t->ui_timer==0 || screen_state)) {
			timersub(&t->next_trigger, &now, &tmp);
			if (timercmp(&tmp, &next, <))
				next = tmp;
		}
		t = t->next;
	}
        pthread_mutex_unlock(&gTimerMutex);
	if (next.tv_sec != INT_MAX)
		rv = ((next.tv_usec%1000)?1:0) +
			next.tv_sec*1000 + next.tv_usec/1000;
	return rv;
}

#ifdef TEST
// gcc timer.c minui/events.c  -o timer_test -D TEST -Wall -lpthread && ./timer_test
int timer1_cb(void*data)
{
	static int i = 0;
	printf("timer1\n");
	if (i++<200)
		return TIMER_AGAIN;
	return TIMER_STOP;
}
int timer2_cb(void*data)
{
	static int i = 0;
	printf("timer2\n");
	if (i++<20)
		return TIMER_AGAIN;
	return TIMER_STOP;
}
ui_timer_t *t1;
ui_timer_t *t2;
void* start_timers(void *a)
{
	sleep(2);
	if (t1 != NULL){
		ui_start_timer(t1, 200);
	}else{
		perror("t1 could not be start!");
	}
	sleep(1);
	if (t2 != NULL){
		ui_start_timer(t2, 1159);
	}else{
		perror("t2 could not be start!");
	}
	return 0;
}
int main(int argc, char **argv)
{
	int next=0;
	pthread_t thr;
	struct input_event ev;
	ev_init();
	t1 = ui_alloc_timer(timer1_cb, NULL);
	t2 = ui_alloc_timer(timer2_cb, NULL);

	pthread_create(&thr, NULL, start_timers, NULL);
	while(1)
	{
		printf("next %d\n",next);
		ev_get(&ev, next);
		next = ui_get_next_timer_ms();
	}
	return 0;
}
#endif

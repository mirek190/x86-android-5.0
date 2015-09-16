/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef TIMER_H
#define TIMER_H

/* timer list */
enum {
	TIMER_STOP,
	TIMER_AGAIN,
};
typedef int (*ui_timer_cb_t)(void *data);
typedef struct ui_timer_s ui_timer_t;

/* public api */
/* allocate a timer
   a ui_timer will be disabled when screen is off
*/
ui_timer_t *ui_alloc_timer(ui_timer_cb_t cb, int ui_timer, void *data);
/* start timer */
void ui_start_timer(ui_timer_t *, int delay_ms);
/* stop timer */
void ui_stop_timer(ui_timer_t *);

/* this calls the expired timer, and return the next timer
   or -1 if no timer */
int ui_get_next_timer_ms(void);

#endif	/* TIMER_H */

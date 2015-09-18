/* * Copyright (C) Intel 2014
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UNBLOCKER_H
#define UNBLOCKER_H

void unblock_start(int logid, int logprio);
void unblock_ack();
int unblock_is_sync(struct log_msg *msg);
void unblock_resume();
void unblock_kmsg_consumed();
#endif /* UNBLOCKER_H */

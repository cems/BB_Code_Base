#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include "session.h"

typedef pthread_t timer;

/* There's a timer for every packet. So store session details, packet data, length and time in this structure */
typedef struct timer_callback_data {
        session_t *session;
        char *packet;
        int packet_len;
        int time;
} timer_data;

typedef struct take_member {
        timer timer_t;
        timer_data *data;
} task;

timer create_new_timer (void (*handler) (void *ptr), timer_data *data);
int delete_timer (timer timer_t);
void delete_task (task *task_t);

#endif

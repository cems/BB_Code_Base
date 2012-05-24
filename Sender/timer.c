#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <stdarg.h>
#include <pthread.h>
#include <errno.h>
#include "session.h"
#include "transport_l.h"
#include "timer.h"
#include "signal.h"

/*  Creates a new timer */
timer create_new_timer (void (*handler) (void *ptr), timer_data *data) {

	timer timer_t;

	pthread_create (&timer_t, NULL, (void *) handler, (void *) data);

	return timer_t;
}


/*  Deletes a timer */
int delete_timer (timer timer_t) {
	
        pthread_cancel(timer_t);
        return pthread_detach(timer_t);
}


/* delte taks */


void delete_task(task *task_t) {
	if (!task_t) {
		printf("Task is Null\n");
		return;
	}
	delete_timer(task_t->timer_t);
	free(task_t->data);
	free(task_t);
}

//
// Created by proje on 30/12/2022.
//

#ifndef WEBSERVER_FILES_QUEUE_H
#define WEBSERVER_FILES_QUEUE_H
#include "assert.h"

typedef struct queue{
	int process_fd;
	struct queue* next;
}*Queue;

Queue init_Queue();
int getQueueSize(Queue q);
void enqueue(Queue q,int process_fd);  // push from tail
int dequeue(Queue q,int process_fd); // pop from head
void remove_from_queue(Queue q, int process_fd);//remove specific instance
void destroy_Queue(Queue q);
//TODO ADD MORE
#endif //WEBSERVER_FILES_QUEUE_H

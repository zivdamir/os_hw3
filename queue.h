//
// Created by proje on 30/12/2022.
//

#ifndef WEBSERVER_FILES_QUEUE_H
#define WEBSERVER_FILES_QUEUE_H
#include "assert.h"

typedef struct queue {
	int process_fd;
	struct queue* next;
}*Queue;
//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
Queue initQueue();
int getQueueSize(Queue q);
void enqueue(Queue q,int process_fd);  // push from tail
int dequeue(Queue q); // pop from head
void removeFromQueue(Queue q, int process_fd);//remove specific instance
void destroyQueue(Queue q);
void printQueue(Queue q);

//TODO ADD MORE
#endif //WEBSERVER_FILES_QUEUE_H

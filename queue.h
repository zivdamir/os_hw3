//
// Created by proje on 30/12/2022.
//

#ifndef WEBSERVER_FILES_QUEUE_H
#define WEBSERVER_FILES_QUEUE_H
#include "assert.h"

typedef struct queue {
	int process_fd;
    struct timeval  arrival_time;
    struct timeval  dispatch_time;
	struct queue* next;
}*Queue;

Queue initQueue();
int getQueueSize(Queue q);
void enqueue(Queue q,int process_fd,struct timeval arrival_time,struct timeval dispatch_time ,int is_waiting_queue);  // push from tail
int dequeue(Queue q); // pop from head
void removeFromQueue(Queue q, int process_fd);//remove specific instance
void destroyQueue(Queue q);

int getQueueHead(Queue q);
struct timeval getArrivalTime(Queue q,int process_fd);

int getKthElementQueue(Queue q, int k); // k is between 1 to queue_size
//TODO ADD MORE
#endif //WEBSERVER_FILES_QUEUE_H

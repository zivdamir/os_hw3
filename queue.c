
#include <stdlib.h>
#include "segel.h"
#include "queue.h"
#define DUMMY -1
// queue is like this : dummy->TAIL->...->...->HEAD->NULL
Queue initQueue()
{
	Queue q=(Queue)malloc(sizeof(*q));
	if (q == NULL)
	{
        unix_error("Malloc error");
		exit(1);
	}
	else{
		//create dummy node for easier implementation
		q->next=NULL;
		q->process_fd=DUMMY;
        gettimeofday(&q->arrival_time,NULL);
        gettimeofday(&q->dispatch_time,NULL);

	}
	return q;
}
void enqueue(Queue q,int process_fd,struct timeval arrival_time,struct timeval dispatch_time)  // push from tail
{
	//malloc new queue object
	assert(q != NULL);
	Queue new_obj = (Queue)malloc(sizeof(*q));
	if(new_obj == NULL)
	{
        unix_error("Malloc error");
		exit(1);
	}
	new_obj->process_fd = process_fd;
	//insert into queue from tail
	Queue curr_tail = q->next;//could be NULL, who cares..
	q->next = new_obj;
	new_obj->next = curr_tail;
    new_obj->arrival_time=arrival_time;
    new_obj->dispatch_time=dispatch_time;
}
int dequeue(Queue q){
	//first get to head...
	if(q->next == NULL) {
		return -1;
	}

	Queue prev = q;
	Queue curr = q->next;
	while(curr->next!=NULL)
	{
		prev = curr;
		curr = curr->next;
	}
	prev->next = NULL;
	int process_head_fd = curr->process_fd;
	free(curr);
	return process_head_fd;


} // pop from head
void removeFromQueue(Queue q, int process_fd){
    Queue iterator = q;
    Queue to_delete = NULL;
    while(iterator->next!=NULL)
    {

        if(iterator->next->process_fd == process_fd)
        {
            to_delete = iterator->next;
            iterator->next = iterator->next->next;
            free(to_delete);
            break;
        }

        iterator = iterator->next;
    }
}//remove specific instance

void destroyQueue(Queue q){
    assert(q!=NULL);
    Queue iterator = q;
    Queue to_delete = NULL;
    while(iterator!=NULL)
    {
        to_delete = iterator;
        iterator = iterator->next;
        to_delete->next = NULL;
        free(to_delete);
    }
}
int getQueueSize(Queue q)
{
    assert(q!=NULL);
    int count = 0;

    Queue curr = q->next;
    while(curr != NULL)
    {
        count++;
        curr = curr->next;
    }
    return count;
}

void printQueue(Queue q)
{
    Queue iterator = q;
    while(iterator!=NULL)
    {
        printf("%d --> ",iterator->process_fd);
        fflush(stdout);
        iterator = iterator->next;
    }
    printf("end copy rot\n");
}
struct timeval* getArrivalTime(Queue q,int process_fd)
{
    struct timeval* required_timeval= NULL;
    Queue curr=q;
    while(curr!= NULL)
    {
        if(curr->process_fd == process_fd)
        {
            required_timeval=&curr->arrival_time;
            break;
        }
        curr=curr->next;
    }
    return required_timeval;
}
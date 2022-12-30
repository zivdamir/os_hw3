
#include <stdlib.h>

#include "queue.h"
#define DUMMY -1
// queue is like this : dummy->TAIL->...->...->HEAD->NULL
Queue initQueue()
{
	Queue q=(Queue)malloc(sizeof(*q));
	if (q == NULL )
	{
		perror("Malloc error");
		exit(1);
	}
	else{
		//create dummy node for easier implementation
		q->next=NULL;
		q->process_fd=DUMMY;
	}
	return q;
}
void enqueue(Queue q,int process_fd)  // push from tail
{
	//malloc new queue object
	assert(q != NULL);
	Queue new_obj=(Queue)malloc(sizeof(*q));

	if(new_obj == NULL)
	{
		perror("Malloc error");
		exit(1);
	}
	else
	{
	new_obj->process_fd=process_fd;
	//insert into queue from tail
	Queue curr_tail=q->next;//could be NULL, who cares..
	q->next=new_obj;
	new_obj->next=curr_tail;
	}

}
int dequeue(Queue q,int process_fd){
	//first get to head..
	if(q->next == NULL)
	{
		return -1;
	}
	else return 1;
} // pop from head
void removeFromQueue(Queue q, int process_fd){

}//remove specific instance
void destroyQueue(Queue q){
}
int getQueueSize(Queue q);
{
    int count =0;
    if(q->next== NULL ) return count;
    else{
      curr=q->next;
      while(curr!=NULL){
          count++;
          curr=curr->next;
      }
  }
  return count;
}
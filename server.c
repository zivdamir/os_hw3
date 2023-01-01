#include "segel.h"

#include "queue.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
// queue waiting requests , queue working reqes

// A thread which handles requests.
typedef struct thread_worker
{
	int connfd; // The current client's fd
    int request_type; // wether the corrent request is a dynamic/static/.. request
	pthread_t thread; // The working thread struct.
}*ThreadWorker;

// threads_num - number of working treads.
int threads_num;

// An Array of ThreadWorkers
ThreadWorker workers;

// Synchronization mechanism objects
pthread_cond_t queue_is_not_empty;
pthread_cond_t queue_has_space;
pthread_mutex_t waiting_queue_lock;

// Queues
Queue working_queue;
Queue waiting_queue;

// HW3: Parse the new arguments too
void getargs(int *port,int* num_threads,int* queue_size, int argc, char *argv[])
{
	if (argc < 4) {
		fprintf(stderr, "Usage: %s <port> <threads> \n", argv[0]);
		exit(1);
	}
	*port = atoi(argv[1]);
	*num_threads=atoi(argv[2]);
	*queue_size=atoi(argv[3]);
	if(*num_threads < 0)
	{
		unix_error("error");
	}
}

void* threadRequestHandlerWrapper()
{
    while(1)
    {
        pthread_mutex_lock(&waiting_queue_lock);
        while(getQueueSize(waiting_queue))
        {

        }
        // wait for request;
        // requestHandle();
    }
}

int main(int argc, char *argv[])
{
    /**received code**/
	int listenfd, connfd, port, clientlen;
	struct sockaddr_in clientaddr;

	/**added by us**/
    working_queue = initQueue();
    waiting_queue = initQueue();

    pthread_mutex_init(&waiting_queue_lock,NULL);
    pthread_cond_init(&queue_has_space,NULL);
    pthread_cond_init(&queue_is_not_empty,NULL);

    struct timeval time;
    int queue_size;
    getargs(&port,&threads_num,&queue_size, argc, argv);
	workers = (ThreadWorker)malloc(sizeof(ThreadWorker)*threads_num);
	if(workers == NULL)
	{
		perror("Malloc failed");
		exit(0);
	}

    listenfd = Open_listenfd(port);

    for(int i = 0; i<threads_num; i++)
    {
        //hello
        pthread_create(&workers[i].thread, NULL, threadRequestHandlerWrapper, NULL);
    }

	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        gettimeofday(&time, NULL);

        pthread_mutex_lock(&waiting_queue_lock);
        if(getQueueSize(waiting_queue)+getQueueSize(working_queue) < queue_size)
        {
            enqueue(waiting_queue, connfd);
            pthread_cond_signal(&queue_is_not_empty);
        }
        else
        {
            pthread_cond_broadcast(&queue_is_not_empty);
            pthread_cond_wait(&queue_has_space,&waiting_queue_lock);
            //activate no space procedure
        }

        pthread_mutex_unlock(&waiting_queue_lock);

        //insert to thread function
        //requestHandle(connfd);
        //Close(connfd);
	}

}


    


 

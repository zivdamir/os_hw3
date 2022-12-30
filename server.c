#include "segel.h"
#include "request.h"


// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
// queue waiting requests , queue working reqes

typedef struct thread_worker
{
	int connfd;
	pthread_t worker;
}*ThreadWorker;

int num_threads;
ThreadWorker workers;
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

int main(int argc, char *argv[])
{
	int listenfd, connfd, port, clientlen; // num_threads- number of working htreads.
	struct sockaddr_in clientaddr;
	int queue_size;
	getargs(&port,&num_threads,&queue_size, argc, argv);
	workers = (ThreadWorker)malloc(sizeof(*workers)*num_threads);
	if(workers == NULL)
	{
		perror("Malloc failed");
		exit(0);
	}

	//create workers
	//
	// HW3: Create some threads...
	//

	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

		//
		// HW3: In general, don't handle the request in the main thread.
		// Save the relevant info in a buffer and have one of the worker threads
		// do the work.
		//
		//pthread_create(&somethreadfrompool, NULL, requestHandle, connfd);
		requestHandle(connfd);

		Close(connfd);
	}

}


    


 

#include "segel.h"
#include "request.h"
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
    int total_http_requsts;//counting also non-wokring requests trying to handle
    int static_requests_handled_count;//only working requests
    int dynamic_requests_handled_count;//only working requests..
	pthread_t thread; // The working thread struct.
}*ThreadWorker;

// threads_num - number of working treads.
int threads_num;
char* schedule_algorithm;
// An Array of ThreadWorkers
ThreadWorker workers;

// Synchronization mechanism objects
pthread_cond_t queue_is_not_empty;
pthread_cond_t queue_has_space;
pthread_mutex_t waiting_queue_lock;
//pthread_mutex_t working_queue_lock;
// Queues
Queue working_queue= NULL;
Queue waiting_queue= NULL;

// HW3: Parse the new arguments too
void getargs(int *port,int* num_threads,int* queue_size,char** schedule_algorithm_arg, int argc, char *argv[])
{
	if (argc < 5) {
		fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <schedalg> \n", argv[0]);
		exit(1);
	}
	*port = atoi(argv[1]);
	*num_threads=atoi(argv[2]);
	*queue_size=atoi(argv[3]);
    *schedule_algorithm_arg=(char*)malloc((strlen(argv[4])+1)*sizeof(char));
    strcpy(*schedule_algorithm_arg,argv[4]);
	if(*num_threads < 0)
	{
        free(*schedule_algorithm_arg);
		unix_error("error");
	}
    if(*queue_size<=0)
    {
        unix_error("error");
    }
}

void* threadRequestHandlerWrapper(void* arg)
{



    while(1)
    {
       // printf("wait for lock\n");
        pthread_mutex_lock(&waiting_queue_lock);
       // printf("got lock\n");
        while(getQueueSize(waiting_queue)==0)
        {
          //  printf("queue size is 0\n");
            pthread_cond_wait(&queue_is_not_empty,&waiting_queue_lock);
        }

        //pthread_cond_signal(&queue_is_not_empty);
        pthread_mutex_unlock(&waiting_queue_lock);

      //  printf("waiting for lock in line 83\n");
        pthread_mutex_lock(&waiting_queue_lock);
        int connection_fd=getQueueHead(waiting_queue);
        struct timeval dispatch_time;
        gettimeofday(&dispatch_time,NULL);
        //do not wake reader here please!
     //   printf("waiting for getarrival in line 90\n");
        struct timeval* arrival_time_p= getArrivalTime(waiting_queue,connection_fd);
        removeFromQueue(waiting_queue,connection_fd);
//assert(arrival_time_p!= NULL); //can't happen!, but still crashes here..TODO
        struct timeval arrival_time=*arrival_time_p;
        if(arrival_time_p == NULL)
        {
     //       printf("waiting for getarrival WHICH GOT NULL in line 95\n");
        }
        enqueue(working_queue,connection_fd,arrival_time,dispatch_time);
        pthread_mutex_unlock(&waiting_queue_lock);
      //  printf("UNLOCK AND THEN SIGNAL NEXT WHICH GOT NULL in line 95\n");
        pthread_cond_signal(&queue_is_not_empty);
        //enqueue into working queue
        //pthread_mutex_lock(&working_queue_lock);
        //enqueue(working_queue,connection_fd,arrival_time,dispatch_time);
        //pthread_mutex_unlock(&working_queue_lock);
        int i = 0;
        pthread_t self = pthread_self();
        for(; i < threads_num; i++)
        {
            if(workers[i].thread == self)
                break;
        }
        //printf("REQUEST HANDLE 112\n");
        requestType req_type = requestHandle(connection_fd,&arrival_time,&dispatch_time,
                                             workers[i].total_http_requsts,
                                             i,
                                             workers[i].static_requests_handled_count,
                                             workers[i].dynamic_requests_handled_count);// NEED TO CHAGNE requesthandle code  SO IT'LL check  DYNAMIC STATIC, error, also prints AND SUCH
        workers[i].total_http_requsts++;
        if(req_type!= ERROR)
        {
            if(req_type==STATIC)
            {
              //  printf("closing static \n");

                workers[i].static_requests_handled_count++;
            }
            else{
               // printf("closing dyn\n ");

                workers[i].dynamic_requests_handled_count++;
            }
        }
        else
        {
          //  printf("REMOVE IN LOIINE 136 HANDLE 112\n");
            removeFromQueue(working_queue,connection_fd);
        }
        pthread_mutex_lock(&waiting_queue_lock);
        removeFromQueue(working_queue,connection_fd);
        //wake reader if needed

        pthread_mutex_unlock(&waiting_queue_lock);
        pthread_cond_signal(&queue_has_space);
        //printf("close in 124 \n ");
        Close(connection_fd);

        }
        // requestHandle();
        //we need to modify  handlerequest and send it more info so we can know whether the request is static or dynamic...
}


int main(int argc, char *argv[])
{
    /**received code**/
	int listenfd, connfd, port, clientlen;
	struct sockaddr_in clientaddr;

	/**added by us**/
    working_queue = initQueue();
    waiting_queue = initQueue();
    //pthread_mutex_init(&working_queue_lock,NULL);
    pthread_mutex_init(&waiting_queue_lock,NULL);
    pthread_cond_init(&queue_has_space,NULL);
    pthread_cond_init(&queue_is_not_empty,NULL);
    //struct timeval arrival_time;
    int queue_size;
    getargs(&port,&threads_num,&queue_size,&schedule_algorithm, argc, argv);
	workers = (ThreadWorker)malloc(sizeof(struct thread_worker)*threads_num);
	if(workers == NULL)
	{

		perror("Malloc failed");
		exit(0);
	}

    listenfd = Open_listenfd(port);

    for(int i = 0; i < threads_num; i++)
    {
        //initalize a worker

        workers[i].total_http_requsts=workers[i].dynamic_requests_handled_count=workers[i].static_requests_handled_count=0;
        pthread_create(&workers[i].thread, NULL, &threadRequestHandlerWrapper, NULL);
    }

	while (1) {
		clientlen = sizeof(clientaddr);

		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        struct timeval arrival_time;
        gettimeofday(&arrival_time, NULL);



        pthread_mutex_lock(&waiting_queue_lock);
        if(getQueueSize(waiting_queue)+getQueueSize(working_queue) < queue_size)
        {
            enqueue(waiting_queue, connfd,arrival_time, arrival_time);
            pthread_cond_signal(&queue_is_not_empty);
        }
        else
        {
            if(strcmp(schedule_algorithm,"block"))
            {
                while(getQueueSize(waiting_queue)+getQueueSize(working_queue) >= queue_size)
                {
                    pthread_cond_signal(&queue_is_not_empty);
                    pthread_cond_wait(&queue_has_space,&waiting_queue_lock);
                }
            }
            else if(strcmp(schedule_algorithm,"dt"))
            {
               // printf("close in 194 \n");
                close(connfd); //todo
                //unlock the mutex so we can listen to a new request
                pthread_mutex_unlock(&waiting_queue_lock);
                //continue without unlocking again(skip the unlock after the all elses
                continue;
            }
            else if(strcmp(schedule_algorithm,"dh"))
            {
                int head_connfd = dequeue(waiting_queue);
                if(head_connfd == -1) // meaning the list is empty and
                {
                  //  printf("close in 206 \n");
                    Close(connfd);
                    pthread_mutex_unlock(&waiting_queue_lock);
                    continue;
                }
                //
                enqueue(waiting_queue,connfd,arrival_time,arrival_time);
                pthread_cond_signal(&queue_is_not_empty);
              //  printf("close in line 213 \n");
                Close(head_connfd);
                pthread_mutex_unlock(&waiting_queue_lock);
                continue;
            }
            else if(strcmp(schedule_algorithm,"random"))
            {
//hello
                int drop_number = getQueueSize(waiting_queue) * 0.5;
                if(getQueueSize(waiting_queue) % 10 != 0) drop_number++;
                if (drop_number >= getQueueSize(waiting_queue))
                {
                    int is_dropped = 0;
                    while(dequeue(waiting_queue) != -1)
                    { is_dropped = 1; }

                    if(!is_dropped)
                    {
                    //    printf("close in random  230\n");
                        Close(connfd);
                        pthread_mutex_unlock(&waiting_queue_lock);
                        continue; //
                    }
                }
                else
                {
                    int* fd_arr = getFdArrayQueue(waiting_queue);
                    for(int i = 0; i < drop_number; i++)
                    {
                        int rnd_value = rand() % getQueueSize(waiting_queue);
                        if(fd_arr[rnd_value] == -1) {i--; continue;}
                        removeFromQueue(waiting_queue, fd_arr[rnd_value]);
                    //    printf("close in 244\n");
                        Close(fd_arr[rnd_value]);
                        fd_arr[rnd_value] = -1;
                    }
                    free(fd_arr);
            }

            }

	}

        pthread_mutex_unlock(&waiting_queue_lock);

}

    }
    


 

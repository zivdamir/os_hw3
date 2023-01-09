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
    int id_in_worker_array;
    int total_http_requsts;//counting also non-wokring requests trying to handle
    int static_requests_handled_count;//only working requests
    int dynamic_requests_handled_count;//only working requests..

}*ThreadWorker;



// threads_num - number of working treads.
int threads_num=0;
char* schedule_algorithm;
// An Array of ThreadWorkers
ThreadWorker workers;

// Synchronization mechanism objects
pthread_cond_t workers_cond;
pthread_cond_t master_cond;
pthread_mutex_t queue_lock;

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
    if(*schedule_algorithm_arg== NULL)
    {
        unix_error("error");
    }
    strcpy(*schedule_algorithm_arg,argv[4]);
    if(*num_threads < 0)
    {
        free(*schedule_algorithm_arg);
        unix_error("error");
    }
    if(*queue_size<=0)
    {
        free(*schedule_algorithm_arg);
        unix_error("error");
    }
    if(!(strcmp(*schedule_algorithm_arg,"random")==0||strcmp(*schedule_algorithm_arg,"dh")==0||strcmp(*schedule_algorithm_arg,"dt")==0||strcmp(*schedule_algorithm_arg,"block")==0))
    {
        free(*schedule_algorithm_arg);
        unix_error("invalid algorithm detected");
    }
}

void* threadRequestHandlerWrapper(void* arg)
{
    struct thread_worker current_thread_stats=*((struct thread_worker*)arg);
    while(1)
    {

        pthread_mutex_lock(&queue_lock);

        while(getQueueSize(waiting_queue)==0)
        {
            pthread_cond_wait(&workers_cond, &queue_lock);
        }

        int connection_fd=getQueueHead(waiting_queue);
        struct timeval dispatch_time;
        gettimeofday(&dispatch_time,NULL);


        struct timeval arrival_time= getArrivalTime(waiting_queue,connection_fd);
        dequeue(waiting_queue);
        enqueue(working_queue,connection_fd,arrival_time,dispatch_time,0);
        pthread_mutex_unlock(&queue_lock);



        requestHandle(connection_fd,arrival_time,dispatch_time,
                      &current_thread_stats.total_http_requsts,
                      current_thread_stats.id_in_worker_array,
                      &current_thread_stats.static_requests_handled_count,
                      &current_thread_stats.dynamic_requests_handled_count);


        pthread_mutex_lock(&queue_lock);
        removeFromQueue(working_queue,connection_fd);
        pthread_cond_signal(&master_cond);
        Close(connection_fd);
        pthread_mutex_unlock(&queue_lock);
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



    pthread_mutex_init(&queue_lock, NULL);
    pthread_cond_init(&master_cond, NULL);
    pthread_cond_init(&workers_cond, NULL);

    int queue_size=0;
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
        workers[i].id_in_worker_array=i;
        workers[i].total_http_requsts=0;
        workers[i].dynamic_requests_handled_count=0;
        workers[i].static_requests_handled_count=0;
        pthread_t thread;
        pthread_create(&thread, NULL, &threadRequestHandlerWrapper, &workers[i]);
    }

    while (1) {
        clientlen = sizeof(clientaddr);

        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        struct timeval arrival_time;
        gettimeofday(&arrival_time, NULL);
        pthread_mutex_lock(&queue_lock);

        if(getQueueSize(waiting_queue)+getQueueSize(working_queue) >= queue_size)
        {


            if(strcmp(schedule_algorithm,"block")==0)
            {
                while(getQueueSize(waiting_queue)+getQueueSize(working_queue) >= queue_size)
                {
                    pthread_cond_wait(&master_cond, &queue_lock);
                }
            }
            else if(strcmp(schedule_algorithm,"dt")==0)
            {

                Close(connfd);
                pthread_mutex_unlock(&queue_lock);

                continue;
            }
            else if(strcmp(schedule_algorithm,"random")==0)
            {

                int drop_number = getQueueSize(waiting_queue)/2 + getQueueSize(waiting_queue)%2;
                if(getQueueSize(waiting_queue)==0)
                {
                    Close(connfd);
                    pthread_mutex_unlock(&queue_lock);
                    continue;
                }
                if(getQueueSize(waiting_queue)==1)
                {
                    Close(dequeue(waiting_queue));
                    enqueue(waiting_queue,connfd,arrival_time,arrival_time,1);
                    pthread_mutex_unlock(&queue_lock);
                    continue;
                }
                else
                {
                    while(drop_number>0) {
                        int random_chosen= rand()% (getQueueSize(waiting_queue));
                        random_chosen+=1;//in order for random_chosen to be from 1 to get_queue_size exactlyt
                        int random_fd = getKthElementQueue(waiting_queue, random_chosen);

                        Close(random_fd);
                        removeFromQueue(waiting_queue,random_fd);
                        drop_number--;
                    }


                }

            }
            else if(strcmp(schedule_algorithm,"dh")==0)
            {

                int head_connfd = dequeue(waiting_queue);
                if(head_connfd == -1) // meaning the list is empty and
                {
                    Close(connfd);
                    pthread_mutex_unlock(&queue_lock);
                    continue;
                }
                Close(head_connfd);
            }
        }
        enqueue(waiting_queue,connfd,arrival_time,arrival_time,1);
        pthread_cond_signal(&workers_cond);
        pthread_mutex_unlock(&queue_lock);


    }
}




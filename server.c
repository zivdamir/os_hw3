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

/*typedef struct thread_worker
{
   // int id_in_worker_array;
    int total_http_requsts;//counting also non-wokring requests trying to handle
    int static_requests_handled_count;//only working requests
    int dynamic_requests_handled_count;//only working requests..
    pthread_t thread; // The working thread struct.
}*ThreadWorker;*/
// threads_num - number of working treads.
int threads_num=0;
char* schedule_algorithm;
// An Array of ThreadWorkers
ThreadWorker workers;

// Synchronization mechanism objects
pthread_cond_t workers_cond;
pthread_cond_t master_cond;
pthread_mutex_t queue_lock;
//pthread_mutex_t working_queue_lock;
// Queues
Queue working_queue= NULL;
Queue waiting_queue= NULL;

// HW3: Parse the new arguments too
void getargs(int *port,int* num_threads,int* queue_size,char** schedule_algorithm_arg, int argc, char *argv[])
{
	if (argc < 4) {
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
    struct thread_worker current_thread_stats=*((struct thread_worker*)arg);
    while(1)
    {
       // printf("wait for lock\n");
        pthread_mutex_lock(&queue_lock);
       // printf("got lock\n");
        while(getQueueSize(waiting_queue)==0)
        {

            //printf("Thread: queue size is 0 \n");
            pthread_cond_wait(&workers_cond, &queue_lock);
        }

        int connection_fd=getQueueHead(waiting_queue);
        struct timeval dispatch_time;
        gettimeofday(&dispatch_time,NULL);

        //pthread_mutex_unlock(&queue_lock);
        struct timeval arrival_time= getArrivalTime(waiting_queue,connection_fd);
        dequeue(waiting_queue);
        enqueue(working_queue,connection_fd,arrival_time,dispatch_time,0);
        pthread_mutex_unlock(&queue_lock);
  //problem here, we're giving wrong thread probably. TDOO

        //printf("REQUEST HANDLE 112\n");


         requestHandle(connection_fd,arrival_time,dispatch_time,
                                     &current_thread_stats.total_http_requsts,
                                      current_thread_stats.id_in_worker_array,
                                     &current_thread_stats.static_requests_handled_count,
                                     &current_thread_stats.dynamic_requests_handled_count);// NEED TO CHAGNE requesthandle code  SO IT'LL check  DYNAMIC STATIC, error, also prints AND SUCH

        //workers[i].total_http_requsts+=1
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


    //pthread_mutex_init(&working_queue_lock,NULL);
    pthread_mutex_init(&queue_lock, NULL);
    pthread_cond_init(&master_cond, NULL);
    pthread_cond_init(&workers_cond, NULL);
    //struct timeval arrival_time;
    int queue_size=0;
    getargs(&port,&threads_num,&queue_size,&schedule_algorithm, argc, argv);
   // printf("port %d, threads num %d, queue_size %d, scheduele alg %s \n",port , threads_num, queue_size, schedule_algorithm);


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

          //  printf("%s\n",schedule_algorithm);
            if(strcmp(schedule_algorithm,"block")==0)
            {
                while(getQueueSize(waiting_queue)+getQueueSize(working_queue) >= queue_size)
                {


                    pthread_cond_wait(&master_cond, &queue_lock);
                }
            }
            else if(strcmp(schedule_algorithm,"dt")==0)
            {
                // printf("close in 194 \n");
                close(connfd); //todo
                pthread_mutex_unlock(&queue_lock);
                //continue without unlocking again(skip the unlock after the all elses
                continue;
            }
            else if(strcmp(schedule_algorithm,"random")==0)
            {
//hello
                int drop_number = getQueueSize(waiting_queue) * 0.5;
               // if(getQueueSize(waiting_queue) % 10 != 0) drop_number++;
                if (drop_number >= getQueueSize(waiting_queue))
                {
                    int is_dropped = 0;
                    while(dequeue(waiting_queue) != -1) {
                        is_dropped = 1;}

                        if (!is_dropped) {
                            //    printf("close in random  230\n");
                            Close(connfd);
                            pthread_mutex_unlock(&queue_lock);
                            continue; //

                    }
                }
                else
                {
                    int* fd_arr = getFdArrayQueue(waiting_queue);
                    for(int i = 0; i < drop_number; i++)
                    {
                        int rnd_value = rand() % getQueueSize(waiting_queue);
                        if(fd_arr[rnd_value] == -1) {
                            i--;
                            continue;}
                        removeFromQueue(waiting_queue, fd_arr[rnd_value]);
                        //    printf("close in 244\n");
                        Close(fd_arr[rnd_value]);
                        fd_arr[rnd_value] = -1;
                    }
                    free(fd_arr);
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
    


 

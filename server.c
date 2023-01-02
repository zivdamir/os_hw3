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
    int id;
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
pthread_mutex_t working_queue_lock;
// Queues
Queue working_queue;
Queue waiting_queue;

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
}

void* threadRequestHandlerWrapper(void* arg)
{
    assert(arg != NULL);
    int* p_id=(int*)arg;//used to change things... (like counter and such)
    int id=*p_id;
    printf("id as argument is %d, id from workers[id] is %d\n",id,workers[id].id);
    assert(workers[id].id == id);//should be the same

    while(1)
    {
        pthread_mutex_lock(&waiting_queue_lock);

        while(getQueueSize(waiting_queue)==0)
        {
            pthread_cond_wait(&queue_is_not_empty,&waiting_queue_lock);
        }
        int connection_fd=dequeue(waiting_queue);
        struct timeval dispatch_time;
        gettimeofday(&dispatch_time,NULL);
        //do not wake reader here please!
        struct timeval* arrival_time_p= getArrivalTime(waiting_queue,connection_fd);
        assert(arrival_time_p!= NULL); //can't happen!, but still crashes here..TODO
        struct timeval arrival_time=*arrival_time_p;



        pthread_mutex_unlock(&waiting_queue_lock);
        //enqueue into working queue
        pthread_mutex_lock(&working_queue_lock);
        enqueue(working_queue,connection_fd,arrival_time,dispatch_time);
        pthread_mutex_unlock(&working_queue_lock);

        requestType req_type = requestHandle(connection_fd,arrival_time,dispatch_time,
                                             workers[id].total_http_requsts,
                                             id,
                                             workers[id].static_requests_handled_count,
                                             workers[id].dynamic_requests_handled_count);// NEED TO CHAGNE requesthandle code  SO IT'LL check  DYNAMIC STATIC, error, also prints AND SUCH
        workers[id].total_http_requsts++;
        if(req_type!= ERROR)
        {
            if(req_type==STATIC)
            {
                workers[id].static_requests_handled_count++;
            }
            else{
                workers[id].dynamic_requests_handled_count++;
            }
        }
        pthread_mutex_lock(&working_queue_lock);
        removeFromQueue(working_queue,connection_fd);
        //wake reader if needed
        pthread_cond_signal(&queue_has_space);
        pthread_mutex_unlock(&working_queue_lock);

        close(connection_fd);
        // wait for request;
        //get request now
        //check type of request somehow to see if its dynamic or not..
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
    pthread_mutex_init(&working_queue_lock,NULL);
    pthread_mutex_init(&waiting_queue_lock,NULL);
    pthread_cond_init(&queue_has_space,NULL);
    pthread_cond_init(&queue_is_not_empty,NULL);
    struct timeval arrival_time;
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
        workers[i].id=i;
        workers[i].total_http_requsts=workers[i].dynamic_requests_handled_count=workers[i].static_requests_handled_count=0;
        pthread_create(&workers[i].thread, NULL, threadRequestHandlerWrapper, (void*)&workers[i].id);
    }

	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        gettimeofday(&arrival_time, NULL);

        //TODO (to finish): 1)implement time 2)in request.c add printing for statistics. 3) add random sched support 4)for queue add way to find time of arrival by connfd and support with dispatch and arrival for all queue operations

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
                pthread_cond_wait(&queue_has_space,&waiting_queue_lock);
            }
            else if(strcmp(schedule_algorithm,"dt"))
            {
                //close the socket
                close(connfd);
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
                    close(connfd);
                    pthread_mutex_unlock(&waiting_queue_lock);
                    continue;
                }
                close(head_connfd);
            }
            else if(strcmp(schedule_algorithm,"random"))
            {
                assert(0);
            }
        }

        pthread_mutex_unlock(&waiting_queue_lock);

        //insert to thread function
        //requestHandle(connfd);
        //Close(connfd);
	}

}


    


 

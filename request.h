#ifndef __REQUEST_H__
/** modified by us**/

#define DYNAMIC 0
#define STATIC 1
#define ERROR 2
int requestHandle(int fd,struct timeval arrival_time,struct timeval dispatch_time, int* http_total_count,int thread_id, int* static_req_count, int* dynamic_req_count);

#endif

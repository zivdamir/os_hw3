#ifndef __REQUEST_H__
/** modified by us**/
typedef enum REQTYPE {DYNAMIC =0 ,STATIC= 1 ,ERROR=2} requestType;
requestType requestHandle(int fd,struct timeval* arrival_time,struct timeval* dispatch_time, int http_total_count,int thread_id, int static_req_count, int dynamic_req_count);

#endif

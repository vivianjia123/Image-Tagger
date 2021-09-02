#ifndef _HTTP_
#define _HTTP_

/* strip s with all \n and \r */
char *strip_copy(const char *s);

/* handle the http request */
bool handle_http_request(int sockfd, Dict dict);

#endif

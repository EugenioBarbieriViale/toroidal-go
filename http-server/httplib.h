#ifndef HTTPLIB_H
#define HTTPLIB_H

typedef struct {
  char *name;
  char *value;
} Header;

typedef struct {
  char *status_line;
  Header *headers;
  int n_headers;
  int content_len;
  char *content;
} HttpResponse;

enum Methods {
  GET,
  POST,
  NONE,
};

typedef struct {
  enum Methods method;
  char *path;
  char *version;
} RequestLine;

typedef struct {
  RequestLine request_line;
  Header *headers;
  int n_headers;
  int content_len;
  char *content;
} HttpRequest;

HttpResponse parse_http_response(char *);
char *parse_status_line(char **);

HttpRequest parse_http_request(char *);
RequestLine parse_request_line(char **);

char *parse_content(char *, int);
int parse_header(Header **, int, char *);

void free_response(HttpResponse *);
void free_request(HttpRequest *);
void free_header(Header *, int);

#endif

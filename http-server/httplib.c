#include "httplib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HttpResponse parse_http_response(char *str) {
  char *status_line = parse_status_line(&str);

  int cap = 2;
  Header *headers = (Header *)malloc(cap * sizeof(Header));
  if (!headers) {
    printf("Failed to allocate memory!");
    exit(EXIT_FAILURE);
  }

  int n_headers = parse_header(&headers, cap, str);

  // do this in parse_headers
  int content_len = 0;
  for (int i = 0; i < n_headers; i++) {
    if (strcmp(headers[i].name, "Content-length") == 0) {
      content_len = atoi(headers[i].value);
      break;
    }
  }

  char *content = parse_content(str, content_len);

  HttpResponse r = {0};
  r.status_line = status_line;
  r.headers = headers;
  r.n_headers = n_headers;
  r.content_len = content_len;
  r.content = content;

  return r;
}

char *parse_status_line(char **str) {
  char *status_line_end = strstr(*str, "\r\n");
  if (!status_line_end)
    exit(EXIT_FAILURE);

  int len = status_line_end - *str;

  char *status_line = malloc(len + 1);
  memcpy(status_line, *str, len);

  status_line[len] = '\0';
  *str = status_line_end + 2;

  return status_line;
}

HttpRequest parse_http_request(char *str) {
  RequestLine rl = parse_request_line(&str);

  int cap = 2;
  Header *headers = (Header *)malloc(cap * sizeof(Header));
  if (!headers) {
    printf("Failed to allocate memory!");
    exit(EXIT_FAILURE);
  }

  int n_headers = parse_header(&headers, cap, str);

  // do this in parse_headers
  int content_len = 0;
  for (int i = 0; i < n_headers; i++) {
    if (strcmp(headers[i].name, "Content-length") == 0) {
      content_len = atoi(headers[i].value);
      break;
    }
  }

  char *content = parse_content(str, content_len);

  HttpRequest r = {0};
  r.request_line = rl;
  r.headers = headers;
  r.n_headers = n_headers;
  r.content_len = content_len;
  r.content = content;

  return r;
}

RequestLine parse_request_line(char **str) {
  char *line_end = strstr(*str, "\r\n");
  if (!line_end)
    exit(EXIT_FAILURE);
  int line_len = line_end - *str;

  char *method_end = memchr(*str, ' ', line_len);
  int method_len = method_end - *str;

  char *method_str = malloc(method_len + 1);
  memcpy(method_str, *str, method_len);
  method_str[method_len] = '\0';

  *str = method_end + 1;

  enum Methods m = NONE;
  if (strcmp(method_str, "GET") == 0) {
    m = GET;
  } else if (strcmp(method_str, "POST") == 0) {
    m = POST;
  }

  char *path_end = memchr(*str, ' ', line_len);
  int path_len = path_end - *str;

  char *path = malloc(path_len + 1);
  memcpy(path, *str, path_len);
  path[path_len] = '\0';

  *str = path_end + 1;

  char *version_end = strstr(*str, "\r\n");
  int version_len = version_end - *str;

  char *version = malloc(version_len + 1);
  memcpy(version, *str, version_len);
  version[version_len] = '\0';

  *str = version_end + 2;

  free(method_str);

  RequestLine rl;
  rl.method = m;
  rl.path = path;
  rl.version = version;

  return rl;
}

char *parse_content(char *str, int content_len) {
  char *header_end = strstr(str, "\r\n\r\n");
  if (!header_end)
    exit(EXIT_FAILURE);
  int header_len = header_end - str;

  int diff = strlen(str) - header_len - 4;
  if (diff != content_len) {
    printf("Invalid content length: expected %d but found %d\n", diff,
           content_len);
    exit(EXIT_FAILURE);
  }

  str = header_end + 4;

  char *content = malloc(content_len + 1);
  memcpy(content, str, content_len);
  content[content_len] = '\0';

  return content;
}

int parse_header(Header **headers, int cap, char *str) {
  Header h = {0};
  int n_headers = 0;

  while (1) {
    char *line_end = strstr(str, "\r\n");
    if (!line_end) {
      printf("Malformed header (%d) at new line!\n", n_headers);
      free_header(*headers, n_headers);
      exit(EXIT_FAILURE);
    }

    int line_len = line_end - str;
    if (line_len == 0)
      break;

    char *name_end = memchr(str, ':', line_len);
    if (!name_end) {
      printf("Malformed header (%d) at colon!\n", n_headers);
      free_header(*headers, n_headers);
      exit(EXIT_FAILURE);
    }

    int name_len = name_end - str;

    h.name = malloc(name_len + 1);
    memcpy(h.name, str, name_len);
    h.name[name_len] = '\0';

    char *value_start = name_end + 1;
    while (*value_start == ' ')
      value_start++;
    int value_len = line_end - value_start;

    h.value = malloc(value_len + 1);
    memcpy(h.value, value_start, value_len);
    h.value[value_len] = '\0';

    if (n_headers >= cap) {
      cap *= 2;
      Header *tmp_ptr = realloc(*headers, cap * sizeof(Header));
      if (tmp_ptr) {
        *headers = tmp_ptr;
      } else {
        perror("Failed to reallocate memory");
        free_header(*headers, n_headers);
        exit(EXIT_FAILURE);
      }
    }

    (*headers)[n_headers] = h;
    n_headers++;

    str = line_end + 2;
  }

  return n_headers;
}

void free_response(HttpResponse *res) {
  free(res->status_line);
  free_header(res->headers, res->n_headers);
  free(res->content);
}

void free_request(HttpRequest *req) {
  free(req->request_line.path);
  free(req->request_line.version);
  free_header(req->headers, req->n_headers);
  free(req->content);
}

void free_header(Header *headers, int n_headers) {
  for (int i = 0; i < n_headers; i++) {
    free(headers[i].name);
    free(headers[i].value);
  }
  free(headers);
}

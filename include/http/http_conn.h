#ifndef PIORUN_HTTP_HTTP_PARSE_H_
#define PIORUN_HTTP_HTTP_PARSE_H_

#include "socket.h"

namespace pio {
class HttpConn {
 public:
  static const int write_buffer_size = 1024;
  static const int read_buffer_size = 2048;

  enum Method {
    GET = 0,
    POST,
    HEAD,
    PUT,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT,
    PATH
  };

  enum CheckState {
    CHECK_STATE_REQUESTLINE = 0,
    CHECK_STATE_HEADER,
    CHECK_STATE_CONTENT
  };

  enum HttpCode {
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
  };

  enum LineStatus { LINE_OK = 0, LINE_BAD, LINE_OPEN };

 public:
  HttpConn();
  ~HttpConn();

 public:
  void init(Socket sock);

 private:
  void init();
  HttpCode ProcessRead();
  bool ProcessWrite(HttpCode ret);
  HttpCode ParseRequestLine(char *text);
  HttpCode ParseHeaders(char *text);
  HttpCode ParseContent(char *text);
  char *GetLine() { return read_buf_ + start_line_; };
  LineStatus ParseLine();
  bool AddResponse(const char *format, ...);
  bool AddContent(const char *content);
  bool AddStatusLine(int status, const char *title);
  bool AddHeaders(int content_length);
  bool AddContentType();
  bool AddContentLength(int content_length);
  bool AddLinger();
  bool AddBlankLine();

 private:
  int sockfd_;
  char read_buf_[read_buffer_size];
  uint32_t read_idx_;
  uint32_t checked_idx_;
  uint32_t start_line_;
  char write_buf_[write_buffer_size];
  char write_idx_;
  CheckState check_state_;
  Method method_;
  char *url_;
  char *version_;
  char *host_;
  uint32_t content_length_;
  bool linger_;
};
}  // namespace pio

#endif
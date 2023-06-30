#include "http/http_conn.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form =
    "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form =
    "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form =
    "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form =
    "There was an unusual problem serving the request file.\n";

namespace pio {

void HttpConn::init() {
  read_idx_ = 0;
  write_idx_ = 0;
  checked_idx_ = 0;
  start_line_ = 0;
  write_idx_ = 0;
  check_state_ = CHECK_STATE_REQUESTLINE;
  method_ = GET;
  url_ = 0;
  version_ = 0;
  host_ = 0;
  content_length_ = 0;
  linger_ = false;
  bytes_to_send = 0;
  bytes_have_send = 0;
}

// 从状态机，用于分析出一行内容
// 返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
HttpConn::LineStatus HttpConn::ParseLine() {
  char temp;
  for (; checked_idx_ < read_idx_; ++checked_idx_) {
    temp = read_buf_[checked_idx_];
    if (temp == '\r') {
      if ((checked_idx_ + 1) == read_idx_)
        return LINE_OPEN;
      else if (read_buf_[checked_idx_ + 1] == '\n') {
        read_buf_[checked_idx_++] = '\0';
        read_buf_[checked_idx_++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    } else if (temp == '\n') {
      if (checked_idx_ > 1 && read_buf_[checked_idx_ - 1] == '\r') {
        read_buf_[checked_idx_ - 1] = '\0';
        read_buf_[checked_idx_++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    }
  }
  return LINE_OPEN;
}

// 解析http请求行，获得请求方法，目标url及http版本号
HttpConn::HttpCode HttpConn::ParseRequestLine(char *text) {
  url_ = strpbrk(text, " \t");
  if (!url_) {
    return BAD_REQUEST;
  }
  *url_++ = '\0';
  char *method = text;
  if (strcasecmp(method, "GET") == 0)
    method_ = GET;
  else if (strcasecmp(method, "POST") == 0) {
    method_ = POST;
  } else
    return BAD_REQUEST;
  url_ += strspn(url_, " \t");
  version_ = strpbrk(url_, " \t");
  if (!version_) return BAD_REQUEST;
  *version_++ = '\0';
  version_ += strspn(version_, " \t");
  if (strcasecmp(version_, "HTTP/1.1") != 0) return BAD_REQUEST;
  if (strncasecmp(url_, "http://", 7) == 0) {
    url_ += 7;
    url_ = strchr(url_, '/');
  }

  if (strncasecmp(url_, "https://", 8) == 0) {
    url_ += 8;
    url_ = strchr(url_, '/');
  }

  if (!url_ || url_[0] != '/') return BAD_REQUEST;
  // 当url为/时，显示判断界面
  if (strlen(url_) == 1) strcat(url_, "judge.html");
  check_state_ = CHECK_STATE_HEADER;
  return NO_REQUEST;
}

// 解析http请求的一个头部信息
HttpConn::HttpCode HttpConn::ParseHeaders(char *text) {
  if (text[0] == '\0') {
    if (content_length_ != 0) {
      check_state_ = CHECK_STATE_CONTENT;
      return NO_REQUEST;
    }
    return GET_REQUEST;
  } else if (strncasecmp(text, "Connection:", 11) == 0) {
    text += 11;
    text += strspn(text, " \t");
    if (strcasecmp(text, "keep-alive") == 0) {
      linger_ = true;
    }
  } else if (strncasecmp(text, "Content-length:", 15) == 0) {
    text += 15;
    text += strspn(text, " \t");
    content_length_ = atol(text);
  } else if (strncasecmp(text, "Host:", 5) == 0) {
    text += 5;
    text += strspn(text, " \t");
    host_ = text;
  } else {
  }
  return NO_REQUEST;
}

// 判断http请求是否被完整读入
HttpConn::HttpCode HttpConn::ParseContent(char *text) {
  if (read_idx_ >= (content_length_ + checked_idx_)) {
    text[content_length_] = '\0';
    content_ = text;
    return GET_REQUEST;
  }
  return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::ProcessRead() {
  LineStatus line_status = LINE_OK;
  HttpCode ret = NO_REQUEST;
  char *text = 0;

  while ((check_state_ == CHECK_STATE_CONTENT && line_status == LINE_OK) ||
         ((line_status = ParseLine()) == LINE_OK)) {
    text = GetLine();
    start_line_ = checked_idx_;
    switch (check_state_) {
      case CHECK_STATE_REQUESTLINE: {
        ret = ParseRequestLine(text);
        if (ret == BAD_REQUEST) return BAD_REQUEST;
        break;
      }
      case CHECK_STATE_HEADER: {
        ret = ParseHeaders(text);
        if (ret == BAD_REQUEST)
          return BAD_REQUEST;
        else if (ret == GET_REQUEST) {
          return DoRequest();
        }
        break;
      }
      case CHECK_STATE_CONTENT: {
        ret = ParseContent(text);
        if (ret == GET_REQUEST) return DoRequest();
        line_status = LINE_OPEN;
        break;
      }
      default:
        return INTERNAL_ERROR;
    }
  }
  return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::DoRequest() { strcpy(write_buf_, read_buf_); }

bool HttpConn::AddResponse(const char *format, ...) {
  if (write_idx_ >= write_buffer_size) return false;
  va_list arg_list;
  va_start(arg_list, format);
  int len = vsnprintf(write_buf_ + write_idx_,
                      write_buffer_size - 1 - write_idx_, format, arg_list);
  if (len >= (write_buffer_size - 1 - write_idx_)) {
    va_end(arg_list);
    return false;
  }
  write_idx_ += len;
  va_end(arg_list);

  return true;
}
bool HttpConn::AddStatusLine(int status, const char *title) {
  return AddResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}
bool HttpConn::AddHeaders(int content_len) {
  return AddContentLength(content_len) && AddLinger() && AddBlankLine();
}
bool HttpConn::AddContentLength(int content_len) {
  return AddResponse("Content-Length:%d\r\n", content_len);
}
bool HttpConn::AddContentType() {
  return AddResponse("Content-Type:%s\r\n", "text/html");
}
bool HttpConn::AddLinger() {
  return AddResponse("Connection:%s\r\n",
                     (linger_ == true) ? "keep-alive" : "close");
}
bool HttpConn::AddBlankLine() { return AddResponse("%s", "\r\n"); }
bool HttpConn::AddContent(const char *content) {
  return AddResponse("%s", content);
}

bool HttpConn::ProcessWrite(HttpCode ret) {
  switch (ret) {
    case INTERNAL_ERROR: {
      AddStatusLine(500, error_500_title);
      AddHeaders(strlen(error_500_form));
      if (!AddContent(error_500_form)) return false;
      break;
    }
    case BAD_REQUEST: {
      AddStatusLine(404, error_404_title);
      AddHeaders(strlen(error_404_form));
      if (!AddContent(error_404_form)) return false;
      break;
    }
    case FORBIDDEN_REQUEST: {
      AddStatusLine(403, error_403_title);
      AddHeaders(strlen(error_403_form));
      if (!AddContent(error_403_form)) return false;
      break;
    }
    default:
      return false;
  }
  bytes_to_send = write_idx_;
  return true;
}

void HttpConn::Process() {
  read_idx_ = strlen(read_buf_);
  HttpCode read_ret = ProcessRead();
  if (read_ret == NO_REQUEST) {
    return;
  }
  // bool write_ret = ProcessWrite(read_ret);
  // if (!write_ret) {
  // TODO: close conn
  // CloseConn()
  // }
}

}  // namespace pio
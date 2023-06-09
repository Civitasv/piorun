#ifndef PIORUN_COROUTINE_ASYNC_READ_H_
#define PIORUN_COROUTINE_ASYNC_READ_H_

#include <span>

#include "coroutine/task/chainable.h"
#include "socket.h"

namespace pio {

task::Chainable AsyncRead(SocketView s, std::span<std::byte>& data);

task::Chainable AsyncRead(SocketView s, char* buffer);

}  // namespace pio

#endif  // PIORUN_COROUTINE_ASYNC_READ_H_
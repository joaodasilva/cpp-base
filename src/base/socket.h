#ifndef BASE_SOCKET_H
#define BASE_SOCKET_H

#include "base/base.h"
#include "base/file.h"
#include "base/memory.h"

struct addrinfo;

class Socket : public FileDescriptor {
 public:
  // Returns a new socket connected to |addr|, or NULL.
  static unique_ptr<Socket> OpenSocket(const addrinfo& addr);

  // Returns a new socket listening at |addr|, or NULL.
  static unique_ptr<Socket> OpenServerSocket(const addrinfo& addr);

  // Returns the size in bytes, or -1 if not available.
  int ReceiveBufferSize() const;

  // Allows reuse of the underlying socket address. Only valid for server
  // sockets.
  bool SetReuseAddr();

  // Disables Nagles algorithm. Returns true if successful.
  bool SetNoDelay();

  // Returns the number of bytes ready to read without blocking, in bytes.
  // Returns -1 if not available.
  int ReadyToReadSize() const;

  // Returns a new Socket. This is only valid on server sockets. If there is no
  // new connection, NULL is returned instead.
  unique_ptr<Socket> AcceptConnection();

 protected:
  explicit Socket(int fd) : FileDescriptor(fd) {}

  static unique_ptr<Socket> CreateSocket(const addrinfo& addr);

 private:
  bool is_server_;

  DISALLOW_COPY_AND_ASSIGN(Socket);
};

#endif  // BASE_SOCKET_H

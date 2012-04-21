#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "base/logging.h"
#include "base/socket.h"

// TODO: move to its own file.
struct DNS {
  static std::string ToString(const addrinfo& addr) {
    void* ptr = NULL;
    uint16 port = 0;
    std::stringstream ss;
    if (addr.ai_family == PF_INET) {
      sockaddr_in *sin = (sockaddr_in *) addr.ai_addr;
      ptr = &sin->sin_addr;
      port = ntohs(sin->sin_port);
    } else if (addr.ai_family == PF_INET6) {
      sockaddr_in6 *sin6 = (sockaddr_in6 *) addr.ai_addr;
      ptr = &sin6->sin6_addr;
      port = ntohs(sin6->sin6_port);
    } else {
      ss << "Unknown family: " << addr.ai_family;
    }

    if (ptr) {
      static char buffer[INET6_ADDRSTRLEN];
      ss << inet_ntop(addr.ai_family, ptr, buffer, sizeof(buffer))
         << ":" << port;
      if (addr.ai_socktype == SOCK_STREAM)
        ss << " (TCP)";
      else if (addr.ai_socktype == SOCK_DGRAM)
        ss << " (UDP)";
      else
        ss << " (type " << addr.ai_socktype << ")";
    }

    return ss.str();
  }
};

// static
unique_ptr<Socket> Socket::OpenSocket(const addrinfo& addr) {
  unique_ptr<Socket> sock = CreateSocket(addr);
  if (!sock)
    return NULL;
  sock->is_server_ = false;

  // Non-blocking sockets can fail to connect() with EINPROGRESS. That means the
  // connection is in progress; polling for write-ready will succeed once the
  // connection is ready.
  int ret = connect(sock->fd(), addr.ai_addr, addr.ai_addrlen);
  if (ret != 0 && errno != EINPROGRESS) {
    DLOGE(ERROR) << "failed to connect to " << DNS::ToString(addr);
    return NULL;
  }

  return sock;
}

// static
unique_ptr<Socket> Socket::OpenServerSocket(const addrinfo& addr) {
  unique_ptr<Socket> sock = CreateSocket(addr);
  if (!sock)
    return NULL;
  sock->is_server_ = true;

  if (!sock->SetReuseAddr())
    return NULL;

  if (bind(sock->fd(), addr.ai_addr, addr.ai_addrlen) != 0) {
    DLOGE(ERROR) << "failed to bind to " << DNS::ToString(addr);
    return NULL;
  }

  if (listen(sock->fd(), 10) != 0) {
    DLOGE(ERROR) << "failed to listen at " << DNS::ToString(addr);
    return NULL;
  }

  return sock;
}

int Socket::ReceiveBufferSize() const {
  int size;
  socklen_t len = sizeof(size);
  int ret = getsockopt(fd(), SOL_SOCKET, SO_RCVBUF, &size, &len);
  if (ret != 0) {
    DLOGE(ERROR) << "getsockopt failed";
    return -1;
  }
  return size;
}

bool Socket::SetReuseAddr() {
  DCHECK(is_server_);
  int yes = 1;
  if (setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    DLOGE(ERROR) << "setsockopt(SO_REUSEADDR) failed";
    return false;
  }
  return true;
}

bool Socket::SetNoDelay() {
  int yes = 1;
  if (setsockopt(fd(), IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1) {
    DLOGE(ERROR) << "setsockopt(TCP_NODELAY) failed";
    return false;
  }
  return true;
}

int Socket::ReadyToReadSize() const {
  int size;
  if (ioctl(fd(), FIONREAD, &size) < 0) {
    DLOGE(ERROR) << "ioctl failed";
    return -1;
  }
  return size;
}

unique_ptr<Socket> Socket::AcceptConnection() {
  DCHECK(is_server_);
  int ret = accept(fd(), NULL, NULL);
  if (ret == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      DLOG(WARNING) << "calling accept() would block, returning NULL";
    else
      DLOGE(ERROR) << "accept() failed";
    return NULL;
  }

  unique_ptr<Socket> sock(new Socket(ret));

  if (!sock->SetNonBlocking())
    return NULL;
  if (!sock->SetCloseOnExec())
    return NULL;

  return sock;
}

// static
unique_ptr<Socket> Socket::CreateSocket(const addrinfo& addr) {
  int fd = socket(addr.ai_family, addr.ai_socktype, addr.ai_protocol);
  if (fd == -1) {
    DLOGE(ERROR) << "failed to open socket to " << DNS::ToString(addr);
    return NULL;
  }

  unique_ptr<Socket> sock(new Socket(fd));

  if (!sock->SetNonBlocking())
    return NULL;
  if (!sock->SetCloseOnExec())
    return NULL;

  return sock;
}

#include "base/dns.h"

#include <sstream>

#include <arpa/inet.h>
#include <string.h>

#include "base/bind.h"
#include "base/event_loop.h"
#include "base/logging.h"

namespace {

void Jump(const DNS::Callback& callback, addrinfo* ptr) {
  DNS::unique_addrinfo unique(ptr);
  callback(std::move(unique));
}

}  // namespace

unique_ptr<DNS> DNS::Create() {
  unique_ptr<EventLoop> loop(EventLoop::Create());
  if (!loop.get())
    return NULL;
  return make_unique(new DNS(loop.release()));
}

DNS::~DNS() {
  dns_loop_->QuitSoon();
  dns_thread_.join();
}

void DNS::Resolve(const std::string& host,
                  const std::string& service,
                  const Callback& callback,
                  int family,
                  int socktype,
                  int protocol) {
  DCHECK(EventLoop::Current());
  dns_loop_->Post(Bind(&DNS::ResolveAndReply, this, host, service, callback,
                       family, socktype, protocol, EventLoop::Current()));
}

// static
std::string DNS::GetHost(const addrinfo& addr) {
  void* ptr = NULL;
  if (addr.ai_family == PF_INET) {
    sockaddr_in *sin = (sockaddr_in *) addr.ai_addr;
    ptr = &sin->sin_addr;
  } else if (addr.ai_family == PF_INET6) {
    sockaddr_in6 *sin6 = (sockaddr_in6 *) addr.ai_addr;
    ptr = &sin6->sin6_addr;
  } else {
    DLOG(WARNING) << "Unknown family: " << addr.ai_family;
    return "";
  }

  char buffer[INET6_ADDRSTRLEN];
  return inet_ntop(addr.ai_family, ptr, buffer, sizeof(buffer));
}

// static
std::string DNS::GetPort(const addrinfo& addr) {
  std::stringstream ss;
  if (addr.ai_family == PF_INET) {
    sockaddr_in *sin = (sockaddr_in *) addr.ai_addr;
    ss << ntohs(sin->sin_port);
  } else if (addr.ai_family == PF_INET6) {
    sockaddr_in6 *sin6 = (sockaddr_in6 *) addr.ai_addr;
    ss << ntohs(sin6->sin6_port);
  } else {
    DLOG(WARNING) << "Unknown family: " << addr.ai_family;
  }

  return ss.str();
}

// static
std::string DNS::ToString(const addrinfo& addr) {
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

void DNS::ResolveAndReply(const std::string& host,
                          const std::string& service,
                          const Callback& callback,
                          int family,
                          int socktype,
                          int protocol,
                          EventLoop* origin_loop) {
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = 0;  // AI_ADDRCONFIG, AI_NUMERICHOST, AI_NUMERICSERV?
  hints.ai_family = family;
  hints.ai_socktype = socktype;
  hints.ai_protocol = protocol;

  addrinfo* result;
  if (getaddrinfo(host.empty() ? NULL : host.c_str(), service.c_str(),
                  &hints, &result) != 0) {
    DLOGE(ERROR) << "getaddrinfo failed for " << host << ":" << service;
    result = NULL;
  }

  origin_loop->Post(Bind(Jump, callback, result));
}

// static
void DNS::delete_addrinfo(addrinfo* addr) {
  freeaddrinfo(addr);
}

DNS::DNS(EventLoop* loop)
    : dns_loop_(loop),
      dns_thread_(Bind(&EventLoop::Run, loop)) {}

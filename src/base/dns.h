#ifndef BASE_DNS_H
#define BASE_DNS_H

#include <functional>
#include <thread>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "base/base.h"
#include "base/memory.h"

class EventLoop;

// Objects of this class have their own EventLoop and thread to resolve DNS
// without blocking the caller. This is handy because getaddrinfo(3) is a
// blocking call.
class DNS {
 public:
  struct addrinfo_deleter {
    void operator()(addrinfo* ptr) {
      DNS::delete_addrinfo(ptr);
    }
  };

  typedef unique_ptr<addrinfo, addrinfo_deleter> unique_addrinfo;
  typedef std::function<void(unique_addrinfo)> Callback;

  // Creates a new DNS object and returns it, or NULL if it fails. The DNS
  // object has its own thread where resolutions are performed.
  static unique_ptr<DNS> Create();

  // Waits until all pending resolutions are completed before returning.
  ~DNS();

  EventLoop* loop() { return dns_loop_.get(); }

  // Resolves |host| and |service|, and replies by invoking |callback| on the
  // current EventLoop. The argument to |callback| is the resolved addrinfo
  // (see getaddrinfo(3)), or NULL. |host| and |service| can either be a name
  // to resolve or a numeric value.
  void Resolve(const std::string& host,
               const std::string& service,
               const Callback& callback,
               int family = PF_UNSPEC,
               int socktype = SOCK_STREAM,
               int protocol = IPPROTO_TCP);

  static std::string GetHost(const addrinfo& addr);
  static std::string GetPort(const addrinfo& addr);
  static std::string ToString(const addrinfo& addr);

 private:
  void ResolveAndReply(const std::string& host,
                       const std::string& service,
                       const Callback& callback,
                       int family,
                       int socktype,
                       int protocol,
                       EventLoop* origin_loop);

  static void delete_addrinfo(addrinfo* addr);

  explicit DNS(EventLoop* loop);

  unique_ptr<EventLoop> dns_loop_;
  std::thread dns_thread_;

  DISALLOW_COPY_AND_ASSIGN(DNS);
};

#endif  // BASE_DNS_H

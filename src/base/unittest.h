#ifndef BASE_UNITTEST_H
#define BASE_UNITTEST_H

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#include "gtest/gtest.h"

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "base/base.h"
#include "base/dns.h"
#include "base/memory.h"
#include "base/time.h"

class EventLoop;
class Socket;

// A base class for tests that need an EventLoop. The TestBody runs within
// the |loop_|.
class BaseTest : public testing::Test {
 protected:
  BaseTest();

 public:
  virtual ~BaseTest();

  virtual void SetUp() override;
  virtual void TearDown() override;

  // Runs all currently pending tasks in all the loops before returning.
  void RunAllPending();

  // Runs and processes blocking calls. Some task should eventually be posted
  // to make the main loop quit; otherwise. Returns false after |timeout| ms
  // if nothing was posted to make the loop quit.
  bool Run(const TimeDelta& timeout = TimeDelta(100));

  void QuitSoon();

  void StartTestServer();
  const addrinfo& GetTestServerAddr() const;
  //unique_ptr<URL> GetTestServerURL(const string& path) const;

  // Blocks until it gets a connection:
  unique_ptr<Socket> AcceptTestServerConnection();

 protected:
  unique_ptr<EventLoop> loop_;
  unique_ptr<DNS> dns_;

 private:
  void OnTestServerAddrResolved(DNS::unique_addrinfo& addr);
  void OnTestServerReadReady(bool invalid, bool hangup, bool error);
  void OnTimeout();

  bool running_;
  bool timed_out_;

  unique_ptr<Socket> server_socket_;
  DNS::unique_addrinfo server_addr_;
  unique_ptr<Socket> connection_socket_;

  DISALLOW_COPY_AND_ASSIGN(BaseTest);
};

#endif  // BASE_UNITTEST_H

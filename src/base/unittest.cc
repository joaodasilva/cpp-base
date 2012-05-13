#include "base/unittest.h"

#include "base/event_loop.h"
#include "base/logging.h"
#include "base/socket.h"

BaseTest::BaseTest()
    : running_(false) {}

BaseTest::~BaseTest() {}

void BaseTest::SetUp() {
  loop_ = EventLoop::Create();
  CHECK(loop_);
  dns_ = DNS::Create();
  CHECK(dns_);
  EventLoop::SetCurrent(loop_.get());
}

void BaseTest::TearDown() {
  RunAllPending();
  dns_.reset();
  loop_.reset();
  EventLoop::SetCurrent(NULL);
}

void BaseTest::RunAllPending() {
  EventLoop::SetCurrent(NULL);

  // Post a task to
  // tell the DNS look to post a task to
  // quit the main loop. This ensures that once the main loop quits, all the
  // pending tasks on DNS have been processed.
  auto postquit = Bind(&EventLoop::QuitSoon, loop_.get());
  auto postpostquit = Bind(&EventLoop::Post, dns_->loop(),
                           std::move(postquit));
  loop_->Post(std::move(postpostquit));
  loop_->Run();

  EventLoop::SetCurrent(loop_.get());
}

bool BaseTest::Run(const TimeDelta& timeout) {
  CHECK(!running_);
  running_ = true;
  timed_out_ = false;
  TimeDelta t = timeout;
  // if (FLAGS_valgrind)
  //   t += TimeDelta(2000);
  ScopedWeakPtrFactory<BaseTest> factory(this);
  loop_->PostAfter(Bind(&BaseTest::OnTimeout, factory.GetWeakPtr()), t);
  EventLoop::SetCurrent(NULL);
  loop_->Run();  // Returns until something from the test calls QuitSoon.
  EventLoop::SetCurrent(loop_.get());
  running_ = false;
  return !timed_out_;
}

void BaseTest::QuitSoon() {
  loop_->QuitSoon();
}

void BaseTest::StartTestServer() {
  DCHECK(!server_socket_);
  dns_->Resolve("", "48080",
                Bind(&BaseTest::OnTestServerAddrResolved, this));
  CHECK(Run());  // Returns when OnTestServerAddrResolved is invoked.
  CHECK(server_addr_.get() != NULL);
  server_socket_ = Socket::OpenServerSocket(*server_addr_);
  CHECK(server_socket_.get() != NULL);
}

const addrinfo& BaseTest::GetTestServerAddr() const {
  DCHECK(server_addr_.get() != NULL);
  return *server_addr_;
}

#if 0
unique_ptr<URL> BaseTest::GetTestServerURL(const std::string& path) const {
  DCHECK(server_addr_.get() != NULL);
  std::string host = DNS::GetHost(GetTestServerAddr());
  std::string port = DNS::GetPort(GetTestServerAddr());
  std::string spec = "http://" + host + ":" + port + path;
  return URL::Parse(spec);
}
#endif

unique_ptr<Socket> BaseTest::AcceptTestServerConnection() {
  DCHECK(server_socket_.get() != NULL);
  DCHECK(connection_socket_.get() == NULL);
  loop_->PostWhenReadReady(server_socket_->fd(),
                           Bind(&BaseTest::OnTestServerReadReady, this));
  if (!Run()) {  // Returns when OnTestServerReadReady is invoked.
    DLOG(ERROR) << "Timed out waiting for connection.";
    return NULL;
  }
  return std::move(connection_socket_);
}

void BaseTest::OnTestServerAddrResolved(DNS::unique_addrinfo addr) {
  server_addr_.swap(addr);
  QuitSoon();
}

void BaseTest::OnTestServerReadReady(bool invalid, bool hangup, bool error) {
  CHECK(!invalid);
  CHECK(!hangup);
  CHECK(!error);
  DCHECK(connection_socket_.get() == NULL);
  connection_socket_ = server_socket_->AcceptConnection();
  CHECK(connection_socket_.get() != NULL);
  QuitSoon();
}

void BaseTest::OnTimeout() {
  if (running_) {
    timed_out_ = true;
    loop_->QuitSoon();
  }
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();
  return ret;
}

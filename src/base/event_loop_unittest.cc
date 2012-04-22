#include <thread>

#include <unistd.h>

#include "base/event_loop.h"
#include "base/time.h"
#include "base/unittest.h"
#include "base/weak.h"

namespace {

void increment(int *counter) {
  (*counter)++;
}

class Incrementer {
 public:
  Incrementer(int *counter): counter_(counter) {}
  void increment() {
    (*counter_)++;
  }
 private:
  int* counter_;
};

class WeakIncrementer : public Incrementer,
                        public Weakling<WeakIncrementer> {
 public:
  explicit WeakIncrementer(int* ptr)
      : Incrementer(ptr) {}

  using Weakling::InvalidateAll;
};

void verify_current(EventLoop* expected) {
  EXPECT_EQ(expected, EventLoop::Current());
}

void post_increment_in_current(int *counter) {
  EventLoop* loop = EventLoop::Current();
  ASSERT_TRUE(loop);
  loop->Post(Bind(increment, counter));
}

Time return_current_time(const Time* t) {
  return *t;
}

}  // namespace

class EventLoopTest : public testing::Test {
 public:
  EventLoopTest()
      : loop_(EventLoop::Create()) {}

  void SetUp() override {
    SetNowFunction(Bind(return_current_time, &now_));
    EXPECT_FALSE(EventLoop::Current());
    ASSERT_TRUE(loop_.get());
  }

  void TearDown() override {
    EXPECT_FALSE(EventLoop::Current());
  }

  void SetNow(const Time& t, bool nval, bool hup, bool err) {
    now_ = t;
  }

  void QuitSoon(EventLoop* loop, bool nval, bool hup, bool err) {
    loop->QuitSoon();
  }

  Time now_;
  unique_ptr<EventLoop> loop_;
};

TEST_F(EventLoopTest, QuitAfterAllWorkDone) {
  int counter = 0;
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(verify_current, loop_.get()));
  loop_->Post(Bind(increment, &counter));
  EXPECT_FALSE(EventLoop::Current());
  loop_->Run();
  EXPECT_FALSE(EventLoop::Current());
  EXPECT_EQ(1, counter);
}

TEST_F(EventLoopTest, WakeUpForWork) {
  int counter = 0;
  std::thread other(Bind(&EventLoop::Run, loop_.get()));
  std::this_thread::sleep_for(TimeDelta(1));
  loop_->Post(Bind(increment, &counter));
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  other.join();
  EXPECT_EQ(1, counter);
}

TEST_F(EventLoopTest, WeakPtr) {
  int counter = 0;
  WeakIncrementer incrementer(&counter);
  WeakPtr<Incrementer> ptr;
  
  // Check loop_ is working.
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(&WeakIncrementer::increment, &incrementer));
  loop_->Run();
  EXPECT_EQ(1, counter);

  // Check loop_ can be restarted.
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(&WeakIncrementer::increment, &incrementer));
  loop_->Run();
  EXPECT_EQ(2, counter);

  // Valid WeakPtr.
  ptr = incrementer.GetWeakPtr();
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(&WeakIncrementer::increment, ptr));
  loop_->Run();
  EXPECT_EQ(3, counter);

  // Invalidated WeakPtr.
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(&WeakIncrementer::InvalidateAll, &incrementer));
  loop_->Post(Bind(&WeakIncrementer::increment, ptr));
  loop_->Run();
  EXPECT_EQ(3, counter);
}

TEST_F(EventLoopTest, After) {
  Time start;
  now_ = start;
  int counter = 0;
  int delayedCounterA = 0;
  int delayedCounterB = 0;
  int delayedCounterC = 0;

  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(increment, &counter));
  loop_->PostAfter(Bind(increment, &delayedCounterA), TimeDelta(30));
  loop_->PostAfter(Bind(increment, &delayedCounterB), TimeDelta(10));
  loop_->PostAfter(Bind(increment, &delayedCounterC), TimeDelta(20));
  loop_->Run();
  EXPECT_EQ(1, counter);
  EXPECT_EQ(0, delayedCounterA);
  EXPECT_EQ(0, delayedCounterB);
  EXPECT_EQ(0, delayedCounterC);

  now_ = start + TimeDelta(1);
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(increment, &counter));
  loop_->Run();
  EXPECT_EQ(2, counter);
  EXPECT_EQ(0, delayedCounterA);
  EXPECT_EQ(0, delayedCounterB);
  EXPECT_EQ(0, delayedCounterC);

  now_ = start + TimeDelta(10);
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(increment, &counter));
  loop_->Run();
  EXPECT_EQ(3, counter);
  EXPECT_EQ(0, delayedCounterA);
  EXPECT_EQ(1, delayedCounterB);
  EXPECT_EQ(0, delayedCounterC);

  now_ = start + TimeDelta(25);
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(increment, &counter));
  loop_->Run();
  EXPECT_EQ(4, counter);
  EXPECT_EQ(0, delayedCounterA);
  EXPECT_EQ(1, delayedCounterB);
  EXPECT_EQ(1, delayedCounterC);

  now_ = start + TimeDelta(100);
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Post(Bind(increment, &counter));
  loop_->Run();
  EXPECT_EQ(5, counter);
  EXPECT_EQ(1, delayedCounterA);
  EXPECT_EQ(1, delayedCounterB);
  EXPECT_EQ(1, delayedCounterC);
}

TEST_F(EventLoopTest, ReadWrite) {
  Time start;
  now_ = start;
  Time end = start + TimeDelta(10);

  int fds[2];
  ASSERT_EQ(0, pipe(fds));
  int pipe_read = fds[0];
  int pipe_write = fds[1];

  // Not read ready, but write ready.
  loop_->PostWhenReadReady(pipe_read, Bind(&EventLoopTest::SetNow, this, end));
  loop_->PostWhenWriteReady(pipe_write,
                            Bind(&EventLoopTest::QuitSoon, this, loop_.get()));
  loop_->Run();
  EXPECT_EQ(start, now_);

  // Make it read ready now.
  uint8 byte = 0;
  ASSERT_EQ(1, write(pipe_write, &byte, 1));
  loop_->PostAfter(Bind(&EventLoop::QuitSoon, loop_.get()), TimeDelta(5));
  loop_->Run();
  EXPECT_EQ(end, now_);

  close(fds[0]);
  close(fds[1]);
}

TEST_F(EventLoopTest, ClosedFd) {
  Time start;
  now_ = start;
  Time end = start + TimeDelta(10);

  int fds[2];
  ASSERT_EQ(0, pipe(fds));
  int pipe_read = fds[0];
  int pipe_write = fds[1];

  // Not read ready, but write ready..
  loop_->PostWhenReadReady(pipe_read, Bind(&EventLoopTest::SetNow, this, end));
  loop_->PostWhenWriteReady(pipe_write,
                            Bind(&EventLoopTest::QuitSoon, this, loop_.get()));
  loop_->Run();
  EXPECT_EQ(start, now_);

  // Make it read ready now.
  close(pipe_write);
  loop_->PostAfter(Bind(&EventLoop::QuitSoon, loop_.get()), TimeDelta(5));
  loop_->Run();
  EXPECT_EQ(end, now_);

  close(fds[0]);
  close(fds[1]);
}

TEST_F(EventLoopTest, Current) {
  unique_ptr<EventLoop> loop2(EventLoop::Create());
  ASSERT_TRUE(loop2.get());
  int counter = 0;
  int counter2 = 0;

  loop_->Post(Bind(post_increment_in_current, &counter));
  loop_->Post(Bind(&EventLoop::QuitSoon, loop_.get()));
  loop_->Run();
  EXPECT_EQ(1, counter);

  loop2->Post(Bind(post_increment_in_current, &counter2));
  loop2->Post(Bind(&EventLoop::QuitSoon, loop2.get()));
  std::thread other(Bind(&EventLoop::Run, loop2.get()));
  other.join();
  EXPECT_EQ(1, counter2);
}

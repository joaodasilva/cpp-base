#include "base/weak.h"

#include "base/unittest.h"

// TODO: enable tests with the EventLoop.
#if 0
#include "base/event_loop.h"
#endif

namespace {

class Incrementer {
 public:
  explicit Incrementer(int* ptr): ptr_(ptr) {}
  void increment() { (*ptr_)++; }
  void add(int n) { (*ptr_) += n; }
  void mult_and_add(int m, int a) { (*ptr_) *= m; (*ptr_) += a; }
  int value() { return *ptr_; }

 private:
  int* ptr_;
  DISALLOW_COPY_AND_ASSIGN(Incrementer);
};

class WeakIncrementer : public Incrementer,
                        public Weakling<WeakIncrementer> {
 public:
  explicit WeakIncrementer(int* ptr)
      : Incrementer(ptr) {}

  using Weakling::InvalidateAll;
};

}  // namespace

TEST(Weak, WeakPtr) {
  int counter = 0;
  WeakIncrementer incrementer(&counter);
  EXPECT_FALSE(incrementer.HasWeakPtrs());

  WeakPtr<WeakIncrementer> w0 = incrementer.GetWeakPtr();
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  WeakPtr<WeakIncrementer> w1 = incrementer.GetWeakPtr();
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  EXPECT_TRUE(w0.get());
  EXPECT_EQ(0, w0->value());
  w0.Reset();
  EXPECT_FALSE(w0.get());
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  EXPECT_EQ(0, w1->value());
  counter = 1;
  EXPECT_EQ(1, w1->value());
  w1.Reset();
  EXPECT_FALSE(incrementer.HasWeakPtrs());

  WeakFlag f0 = incrementer.GetWeakPtr().GetWeakFlag();
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  EXPECT_TRUE(f0.IsValid());
  WeakFlag f1 = f0;
  WeakFlag f2;
  f2 = f0;
  EXPECT_TRUE(f1.IsValid());
  EXPECT_TRUE(f2.IsValid());
  f0.Reset();
  EXPECT_FALSE(f0.IsValid());
  EXPECT_TRUE(f1.IsValid());
  EXPECT_TRUE(f2.IsValid());
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  f1.Reset();
  EXPECT_FALSE(f0.IsValid());
  EXPECT_FALSE(f1.IsValid());
  EXPECT_TRUE(f2.IsValid());
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  f2.Reset();
  EXPECT_FALSE(f0.IsValid());
  EXPECT_FALSE(f1.IsValid());
  EXPECT_FALSE(f2.IsValid());
  EXPECT_FALSE(incrementer.HasWeakPtrs());

  w0 = incrementer.GetWeakPtr();
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  EXPECT_TRUE(w0.get());
  f0 = w0.GetWeakFlag();
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  EXPECT_TRUE(f0.IsValid());
  incrementer.InvalidateAll();
  EXPECT_FALSE(incrementer.HasWeakPtrs());
  EXPECT_FALSE(w0.get());
  EXPECT_FALSE(f0.IsValid());

  w0 = incrementer.GetWeakPtr();
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  EXPECT_TRUE(w0.get());
  f0 = w0.GetWeakFlag();
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  EXPECT_TRUE(f0.IsValid());
  w0.Reset();
  EXPECT_FALSE(w0.get());
  EXPECT_TRUE(incrementer.HasWeakPtrs());
  EXPECT_TRUE(f0.IsValid());
  incrementer.InvalidateAll();
  EXPECT_FALSE(incrementer.HasWeakPtrs());
  EXPECT_FALSE(w0.get());
  EXPECT_FALSE(f0.IsValid());

  {
    WeakIncrementer inc(&counter);
    w0 = inc.GetWeakPtr();
    EXPECT_TRUE(inc.HasWeakPtrs());
    EXPECT_TRUE(w0.get());
  }
  EXPECT_FALSE(w0.get());
}

// TODO: port this.
#if 0
void inc(WeakPtr<WeakIncrementer> weak) {
  if (weak.get())
    weak->increment();
  EventLoop::Current()->QuitSoon();
}

void bounce(WeakPtr<WeakIncrementer> weak, EventLoop* reply_loop) {
  reply_loop->Post(bind(inc, weak));
}

TEST(Weak, CopyAcrossThreads) {
  int counter = 0;
  unique_ptr<WeakIncrementer> incrementer(new WeakIncrementer(&counter));
  unique_ptr<EventLoop> main(EventLoop::Create(CreateSteadyClock()));
  ASSERT_TRUE(main.get());
  unique_ptr<EventLoop> other(EventLoop::Create(CreateSteadyClock()));
  ASSERT_TRUE(other.get());
  other->Post(bind(bounce, incrementer->GetWeakPtr(), main.get()));
  std::thread other_thread(bind(&EventLoop::Run, other.get()));
  main->Run();
  other->QuitSoon();
  other_thread.join();
  EXPECT_EQ(1, counter);
}
#endif

TEST(Weak, ScopedWeakPtrFactory) {
  int counter = 0;
  Incrementer incrementer(&counter);
  ScopedWeakPtrFactory<Incrementer> factory(&incrementer);
  EXPECT_FALSE(factory.HasWeakPtrs());

  WeakPtr<Incrementer> w0 = factory.GetWeakPtr();
  EXPECT_TRUE(factory.HasWeakPtrs());
  WeakPtr<Incrementer> w1 = factory.GetWeakPtr();
  EXPECT_TRUE(factory.HasWeakPtrs());
  EXPECT_TRUE(w0.get());
  EXPECT_EQ(0, w0->value());
  w0.Reset();
  EXPECT_FALSE(w0.get());
  EXPECT_TRUE(factory.HasWeakPtrs());
  EXPECT_EQ(0, w1->value());
  counter = 1;
  EXPECT_EQ(1, w1->value());
  w1.Reset();
  EXPECT_FALSE(factory.HasWeakPtrs());

  WeakFlag f0 = factory.GetWeakPtr().GetWeakFlag();
  EXPECT_TRUE(factory.HasWeakPtrs());
  EXPECT_TRUE(f0.IsValid());
  WeakFlag f1 = f0;
  WeakFlag f2;
  f2 = f0;
  EXPECT_TRUE(f1.IsValid());
  EXPECT_TRUE(f2.IsValid());
  f0.Reset();
  EXPECT_FALSE(f0.IsValid());
  EXPECT_TRUE(f1.IsValid());
  EXPECT_TRUE(f2.IsValid());
  EXPECT_TRUE(factory.HasWeakPtrs());
  f1.Reset();
  EXPECT_FALSE(f0.IsValid());
  EXPECT_FALSE(f1.IsValid());
  EXPECT_TRUE(f2.IsValid());
  EXPECT_TRUE(factory.HasWeakPtrs());
  f2.Reset();
  EXPECT_FALSE(f0.IsValid());
  EXPECT_FALSE(f1.IsValid());
  EXPECT_FALSE(f2.IsValid());
  EXPECT_FALSE(factory.HasWeakPtrs());

  w0 = factory.GetWeakPtr();
  EXPECT_TRUE(factory.HasWeakPtrs());
  EXPECT_TRUE(w0.get());
  f0 = w0.GetWeakFlag();
  EXPECT_TRUE(factory.HasWeakPtrs());
  EXPECT_TRUE(f0.IsValid());
  factory.InvalidateAll();
  EXPECT_FALSE(factory.HasWeakPtrs());
  EXPECT_FALSE(w0.get());
  EXPECT_FALSE(f0.IsValid());

  w0 = factory.GetWeakPtr();
  EXPECT_TRUE(factory.HasWeakPtrs());
  EXPECT_TRUE(w0.get());
  f0 = w0.GetWeakFlag();
  EXPECT_TRUE(factory.HasWeakPtrs());
  EXPECT_TRUE(f0.IsValid());
  w0.Reset();
  EXPECT_FALSE(w0.get());
  EXPECT_TRUE(factory.HasWeakPtrs());
  EXPECT_TRUE(f0.IsValid());
  factory.InvalidateAll();
  EXPECT_FALSE(factory.HasWeakPtrs());
  EXPECT_FALSE(w0.get());
  EXPECT_FALSE(f0.IsValid());

  {
    ScopedWeakPtrFactory<Incrementer> factory2(&incrementer);
    w0 = factory2.GetWeakPtr();
    EXPECT_TRUE(factory2.HasWeakPtrs());
    EXPECT_TRUE(w0.get());
  }
  EXPECT_FALSE(w0.get());
}

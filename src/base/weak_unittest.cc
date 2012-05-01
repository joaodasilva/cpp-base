#include "base/weak.h"

#include "base/event_loop.h"
#include "base/unittest.h"

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

TEST(Weak, CopyAcrossThreads) {
  unique_ptr<EventLoop> main(EventLoop::Create());
  ASSERT_TRUE(main.get());
  unique_ptr<EventLoop> other(EventLoop::Create());
  ASSERT_TRUE(other.get());

  int counter = 0;
  unique_ptr<WeakIncrementer> incrementer(new WeakIncrementer(&counter));
  int counter2 = 0;
  unique_ptr<WeakIncrementer> incrementer2(new WeakIncrementer(&counter2));

  auto inc = Bind(&WeakIncrementer::increment, incrementer->GetWeakPtr());
  auto inc2 = Bind(&WeakIncrementer::increment, incrementer2->GetWeakPtr());
  auto invalidate = Bind(&WeakIncrementer::InvalidateAll,
                         incrementer2->GetWeakPtr());

  other->Post(Bind(&EventLoop::Post, main.get(), inc));
  other->Post(Bind(&EventLoop::Post, main.get(), invalidate));
  other->Post(Bind(&EventLoop::Post, main.get(), inc2));
  other->Post(Bind(&EventLoop::QuitSoon, main.get()));
  other->QuitSoon();

  std::thread other_thread(Bind(&EventLoop::Run, other.get()));
  main->Run();
  other_thread.join();
  EXPECT_EQ(1, counter);
  EXPECT_EQ(0, counter2);
}

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

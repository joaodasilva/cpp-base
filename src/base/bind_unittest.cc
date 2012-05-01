#include "base/bind.h"

#include <string>

#include "base/unittest.h"
#include "base/weak.h"

using namespace std::placeholders;

namespace {

class Foo {
 public:
  explicit Foo(std::string arg) : aa(arg) {}
  ~Foo() {}

  std::string Merge(std::string bb, std::string cc) {
    return "(" + aa + ", " + bb + ", " + cc + ")";
  }

  std::string ConstMerge(std::string bb, std::string cc) const {
    return "const (" + aa + ", " + bb + ", " + cc + ")";
  }

  std::string Overloaded(std::string s) {
    return "Overloaded(" + s + aa + ")";
  }

  std::string Overloaded(std::string s) const {
    return "const Overloaded(" + s + aa + ")";
  }

  void Copy(std::string* to) {
    *to = aa;
  }

 private:
  std::string aa;
};

class Zombie {
 public:
  explicit Zombie(bool* alive_flag) : alive_flag_(alive_flag) {}
  ~Zombie() { *alive_flag_ = false; }

  bool alive() { return *alive_flag_; }

 private:
  bool* alive_flag_;
};

class Tracer {
 public:
  Tracer() {
    created_++;
  }

  Tracer(const Tracer& tracer) {
    copied_++;
  }

  Tracer(Tracer&& tracer) {
    moved_++;
  }

  ~Tracer() {
    deleted_++;
  }

  static void Reset() {
    created_ = 0;
    copied_ = 0;
    moved_ = 0;
    deleted_ = 0;
  }

  static void DumpStats() {
    LOG(INFO) << "Created: " << created_;
    LOG(INFO) << "Copied: " << copied_;
    LOG(INFO) << "Moved: " << moved_;
    LOG(INFO) << "Deleted: " << deleted_;
    LOG(INFO) << "";
  }

  static int created_;
  static int copied_;
  static int moved_;
  static int deleted_;
};

int Tracer::created_ = 0;
int Tracer::copied_ = 0;
int Tracer::moved_ = 0;
int Tracer::deleted_ = 0;

void NopConstRef(const Tracer& tracer) {}

void NopValue(Tracer tracer) {}

void NopRef(Tracer& tracer) {}

std::string Merge(std::string aa, std::string bb, std::string cc) {
  return "(" + aa + ", " + bb + ", " + cc + ")";
}

void MergeRef(std::string& to, std::string from) {
  to += from;
}

const char kAA[] = "111";
const char kBB[] = "222";
const char kCC[] = "333";
const char kMerged[] = "(111, 222, 333)";
const char kConstMerged[] = "const (111, 222, 333)";

}  // namespace

TEST(ApplyTest, Function) {
  EXPECT_EQ(kMerged, Apply(Merge, std::make_tuple(kAA, kBB, kCC)));
  EXPECT_EQ(kMerged, Apply(Merge, std::make_tuple(kAA, kBB), kCC));
  EXPECT_EQ(kMerged, Apply(Merge, std::make_tuple(kAA), kBB, kCC));
  EXPECT_EQ(kMerged, Apply(Merge, std::make_tuple(), kAA, kBB, kCC));
}

TEST(ApplyTest, Method) {
  Foo foo(kAA);
  EXPECT_EQ(kMerged, Apply(&Foo::Merge, std::make_tuple(&foo, kBB, kCC)));
  EXPECT_EQ(kMerged, Apply(&Foo::Merge, std::make_tuple(&foo, kBB), kCC));
  EXPECT_EQ(kMerged, Apply(&Foo::Merge, std::make_tuple(&foo), kBB, kCC));
  EXPECT_EQ(kMerged, Apply(&Foo::Merge, std::make_tuple(), &foo, kBB, kCC));
}

TEST(ApplyTest, ConstMethod) {
  Foo foo(kAA);
  const Foo* cfoo = &foo;
  EXPECT_EQ(kConstMerged,
            Apply(&Foo::ConstMerge, std::make_tuple(cfoo, kBB, kCC)));
  EXPECT_EQ(kConstMerged,
            Apply(&Foo::ConstMerge, std::make_tuple(cfoo, kBB), kCC));
  EXPECT_EQ(kConstMerged,
            Apply(&Foo::ConstMerge, std::make_tuple(cfoo), kBB, kCC));
  EXPECT_EQ(kConstMerged,
            Apply(&Foo::ConstMerge, std::make_tuple(), cfoo, kBB, kCC));
}

TEST(ApplyTest, WeakMethod) {
  Foo foo(kAA);
  ScopedWeakPtrFactory<Foo> factory(&foo);
  WeakPtr<Foo> weak = factory.GetWeakPtr();

  std::string copy;
  Apply(&Foo::Copy, std::make_tuple(weak, &copy));
  EXPECT_EQ(kAA, copy);

  copy.clear();
  EXPECT_EQ(std::string(), copy);
  factory.InvalidateAll();
  Apply(&Foo::Copy, std::make_tuple(weak, &copy));
  EXPECT_EQ(std::string(), copy);
}

TEST(ApplyTest, Ref) {
  std::string from("from");
  Apply(MergeRef, std::make_tuple(), from, "to");
  EXPECT_EQ("fromto", from);

  Apply(MergeRef, std::make_tuple(std::ref(from)), "to");
  EXPECT_EQ("fromtoto", from);

  Apply(MergeRef, std::make_tuple(std::ref(from), "to"));
  EXPECT_EQ("fromtototo", from);
}

TEST(ApplyTest, CopiesConstRef) {
  Tracer::Reset();
  Apply(NopConstRef, std::make_tuple(), Tracer());
  Tracer::DumpStats();

  Tracer::Reset();
  {
    Tracer tracer;
    Apply(NopConstRef, std::make_tuple(), tracer);
  }
  Tracer::DumpStats();

  Tracer::Reset();
  Apply(NopConstRef, std::make_tuple(Tracer()));
  Tracer::DumpStats();

  Tracer::Reset();
  {
    Tracer tracer;
    Apply(NopConstRef, std::make_tuple(tracer));
  }
  Tracer::DumpStats();
}

TEST(ApplyTest, CopiesRef) {
  // This is invalid, because NopRef(Tracer()) can't bind an rvalue to a ref.
#if 0
  Tracer::Reset();
  Apply(NopRef, std::make_tuple(), Tracer());
  Tracer::DumpStats();
#endif

  Tracer::Reset();
  {
    Tracer tracer;
    Apply(NopRef, std::make_tuple(), tracer);
  }
  Tracer::DumpStats();

  // Same as above.
#if 0
  Tracer::Reset();
  Apply(NopRef, std::make_tuple(Tracer()));
  Tracer::DumpStats();
#endif

  Tracer::Reset();
  {
    Tracer tracer;
    Apply(NopRef, std::make_tuple(std::ref(tracer)));
  }
  Tracer::DumpStats();
}

TEST(ApplyTest, CopiesValue) {
  Tracer::Reset();
  Apply(NopValue, std::make_tuple(), Tracer());
  Tracer::DumpStats();

  Tracer::Reset();
  {
    Tracer tracer;
    Apply(NopValue, std::make_tuple(), tracer);
  }
  Tracer::DumpStats();

  Tracer::Reset();
  Apply(NopValue, std::make_tuple(Tracer()));
  Tracer::DumpStats();

  Tracer::Reset();
  {
    Tracer tracer;
    Apply(NopValue, std::make_tuple(tracer));
  }
  Tracer::DumpStats();
}

TEST(BindTest, Function) {
  auto cb1 = Bind(Merge, kAA, kBB, kCC);
  EXPECT_EQ(kMerged, cb1());
  auto cb2 = Bind(Merge, kAA, kBB);
  EXPECT_EQ(kMerged, cb2(kCC));
  auto cb3 = Bind(Merge, kAA);
  EXPECT_EQ(kMerged, cb3(kBB, kCC));
  auto cb4 = Bind(Merge);
  EXPECT_EQ(kMerged, cb4(kAA, kBB, kCC));
}

TEST(BindTest, Method) {
  Foo foo(kAA);
  auto cb1 = Bind(&Foo::Merge, &foo, kBB, kCC);
  EXPECT_EQ(kMerged, cb1());
  auto cb2 = Bind(&Foo::Merge, &foo, kBB);
  EXPECT_EQ(kMerged, cb2(kCC));
  auto cb3 = Bind(&Foo::Merge, &foo);
  EXPECT_EQ(kMerged, cb3(kBB, kCC));
  auto cb4 = Bind(&Foo::Merge);
  EXPECT_EQ(kMerged, cb4(&foo, kBB, kCC));
}

TEST(BindTest, ConstMethod) {
  Foo foo(kAA);
  const Foo* cfoo = &foo;
  auto cb1 = Bind(&Foo::ConstMerge, cfoo, kBB, kCC);
  EXPECT_EQ(kConstMerged, cb1());
  auto cb2 = Bind(&Foo::ConstMerge, cfoo, kBB);
  EXPECT_EQ(kConstMerged, cb2(kCC));
  auto cb3 = Bind(&Foo::ConstMerge, cfoo);
  EXPECT_EQ(kConstMerged, cb3(kBB, kCC));
  auto cb4 = Bind(&Foo::ConstMerge);
  EXPECT_EQ(kConstMerged, cb4(cfoo, kBB, kCC));
}

TEST(BindTest, WeakMethod) {
  Foo foo(kAA);
  ScopedWeakPtrFactory<Foo> factory(&foo);
  WeakPtr<Foo> weak = factory.GetWeakPtr();

  std::string copy;
  auto cb1r = Bind(&Foo::Copy, factory.GetWeakPtr(), &copy);
  auto cb1l = Bind(&Foo::Copy, weak, &copy);

  cb1r();
  EXPECT_EQ(kAA, copy);
  copy.clear();
  cb1l();
  EXPECT_EQ(kAA, copy);

  factory.InvalidateAll();
  auto cb2 = Bind(&Foo::Copy, weak, &copy);
  copy.clear();
  cb2();
  EXPECT_EQ(std::string(), copy);
}

TEST(BindTest, UniquePtr) {
  bool alive = true;
  unique_ptr<Zombie> zombie(new Zombie(&alive));
  EXPECT_TRUE(alive);
  zombie.reset();
  EXPECT_FALSE(alive);

  alive = true;
  zombie.reset(new Zombie(&alive));
  EXPECT_TRUE(alive);

  {
    // The callback owns the Zombie.
    auto cb = Bind(&Zombie::alive, std::move(zombie));
    EXPECT_TRUE(alive);
    EXPECT_TRUE(cb());
    EXPECT_TRUE(alive);
  }
  EXPECT_FALSE(alive);
}

TEST(BindTest, BindBind) {
  auto func = std::bind(Merge, _2, "+", _1);
  auto cb = Bind(std::move(func), "123");
  EXPECT_EQ("(456, +, 123)", cb("456"));
}

TEST(BindTest, BindBind2) {
  std::function<std::string(std::string, std::string)> func =
      std::bind(Merge, _2, "+", _1);
  auto cb = Bind(func, "123");
  EXPECT_EQ("(456, +, 123)", cb("456"));
}

TEST(BindTest, BindFunction) {
  std::function<std::string(std::string, std::string, std::string)> f = Merge;
  auto cb = Bind(f, kAA);
  EXPECT_EQ(kMerged, cb(kBB, kCC));
}

TEST(BindTest, StdFunction) {
  std::function<std::string(std::string)> f = Bind(Merge, kAA, kBB);
  EXPECT_EQ(kMerged, f(kCC));
}

TEST(BindTest, CopiesConstRef) {
  Tracer::Reset();
  {
    auto t = Bind(NopConstRef, Tracer());
    t();
  }
  Tracer::DumpStats();

 Tracer::Reset();
  {
    Tracer tracer;
    auto t = Bind(NopConstRef, tracer);
    t();
  }
  Tracer::DumpStats();
}

TEST(BindTest, CopiesRef) {
 Tracer::Reset();
  {
    Tracer tracer;
    auto t = Bind(NopRef, std::ref(tracer));
    t();
  }
  Tracer::DumpStats();
}

TEST(BindTest, CopiesValue) {
  Tracer::Reset();
  {
    auto t = Bind(NopValue, Tracer());
    t();
  }
  Tracer::DumpStats();

 Tracer::Reset();
  {
    Tracer tracer;
    auto t = Bind(NopValue, tracer);
    t();
  }
  Tracer::DumpStats();
}

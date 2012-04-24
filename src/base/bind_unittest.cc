#include "base/bind.h"

#include <string>

#include "base/unittest.h"
#include "base/weak.h"

namespace {

class Foo {
 public:
  explicit Foo(std::string arg) : aa(arg) {}
  ~Foo() {}

  std::string Merge(std::string bb, std::string cc) {
    return "(" + aa + ", " + bb + ", " + cc + ")";
  }

  // TODO: const method can't have the same name.
  // TODO: test other overloadings.
  std::string ConstMerge(std::string bb, std::string cc) const {
    return "const (" + aa + ", " + bb + ", " + cc + ")";
  }

  void Copy(std::string* to) {
    *to = aa;
  }

 private:
  std::string aa;
};

std::string Merge(std::string aa, std::string bb, std::string cc) {
  return "(" + aa + ", " + bb + ", " + cc + ")";
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

// TODO: test Bind() with bind() and function<>
// TODO: test Bind() with unique_ptrs

// TODO: test Apply() with many types of parameters:
//  - (auto, const auto, refs, auto refs, rvalues, const rvalues?) x
//    (int, string, struct, char[], tuples) x
//    (function, method, const method, static method, virtual method, functor) x
//    (return void, return string, return tuple)

// TODO: test same for Bind()

// TODO: make this work:
// std::function<ret_type(arg_type, arg2_type)> f = Bind(...)
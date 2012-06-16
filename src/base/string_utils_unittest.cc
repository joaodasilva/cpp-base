#include "base/string_utils.h"

#include "base/unittest.h"

TEST(StringUtilsTest, SplitString) {
  std::vector<std::string> list;

  SplitString("", ',', &list);
  EXPECT_EQ(0, list.size());

  SplitString("  ", ',', &list);
  EXPECT_EQ(0, list.size());

  SplitString("1", ',', &list);
  EXPECT_EQ(1u, list.size());
  EXPECT_EQ("1", list[0]);

  SplitString("1,2", ',', &list);
  EXPECT_EQ(2u, list.size());
  EXPECT_EQ("1", list[0]);
  EXPECT_EQ("2", list[1]);

  SplitString(",", ',', &list);
  EXPECT_EQ(2u, list.size());
  EXPECT_EQ("", list[0]);
  EXPECT_EQ("", list[1]);

  SplitString("  ,  ", ',', &list);
  EXPECT_EQ(2u, list.size());
  EXPECT_EQ("", list[0]);
  EXPECT_EQ("", list[1]);

  SplitString("1,", ',', &list);
  EXPECT_EQ(2u, list.size());
  EXPECT_EQ("1", list[0]);
  EXPECT_EQ("", list[1]);

  SplitString(",2", ',', &list);
  EXPECT_EQ(2u, list.size());
  EXPECT_EQ("", list[0]);
  EXPECT_EQ("2", list[1]);

  SplitString("  1   ", ',', &list);
  EXPECT_EQ(1u, list.size());
  EXPECT_EQ("1", list[0]);

  SplitString("  1   ,       2,     3          , 4 5 6 ", ',', &list);
  EXPECT_EQ(4u, list.size());
  EXPECT_EQ("1", list[0]);
  EXPECT_EQ("2", list[1]);
  EXPECT_EQ("3", list[2]);
  EXPECT_EQ("4 5 6", list[3]);
}

TEST(StringUtilsTest, StringToUnsigned) {
  uint32 value = 123;
  EXPECT_TRUE(StringToUnsigned("0", &value));
  EXPECT_EQ(0u, value);
  EXPECT_TRUE(StringToUnsigned("1", &value));
  EXPECT_EQ(1u, value);
  EXPECT_TRUE(StringToUnsigned("123", &value));
  EXPECT_EQ(123u, value);
  EXPECT_FALSE(StringToUnsigned("-1", &value));
  EXPECT_FALSE(StringToUnsigned("abc", &value));
  EXPECT_FALSE(StringToUnsigned(" 1", &value));
  EXPECT_FALSE(StringToUnsigned("-0", &value));
  EXPECT_FALSE(StringToUnsigned("1.2", &value));
}

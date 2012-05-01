#include "base/url.h"

#include <string>

#include "base/unittest.h"

TEST(URL, EncodeEmpty) {
  std::string empty;
  std::string result;

  URL::Encode(empty, true, &result);
  EXPECT_EQ(result, std::string());
}

TEST(URL, DecodeEmpty) {
  std::string empty;
  std::string result;

  EXPECT_TRUE(URL::Decode(empty, &result));
  EXPECT_EQ(result, std::string());
}

TEST(URL, Encode) {
  std::string decoded("abc+dd:/?#[]@!$&'()*,;=123 +");
  std::string expected("abc%2Bdd%3A%2F%3F%23%5B%5D%40%21%24%26%27%28%29%2A%2C"
                       "%3B%3D123+%2B");
  std::string expected2("abc%2Bdd%3A%2F%3F%23%5B%5D%40%21%24%26%27%28%29%2A%2C"
                        "%3B%3D123%20%2B");
  std::string result;
  URL::Encode(decoded, true, &result);
  EXPECT_EQ(expected, result);
  URL::Encode(decoded, false, &result);
  EXPECT_EQ(expected2, result);
}

TEST(URL, Decode) {
  std::string encoded("abc%2Bdd%3A%2F%3F%23%5B%5D%40%21%24%26%27%28%29%2A%2C%3B"
                      "%3D123+%20%2B");
  std::string expected("abc+dd:/?#[]@!$&'()*,;=123  +");
  std::string result;
  EXPECT_TRUE(URL::Decode(encoded, &result));
  EXPECT_EQ(expected, result);
}

TEST(URL, DecodeFail) {
  std::string encoded("abc%2Bdd%3A%2F%3F%2-");
  std::string result;
  EXPECT_FALSE(URL::Decode(encoded, &result));
}

TEST(URL, TestAll) {
  std::string all(256, 0);
  for (int i = 0; i < 256; ++i)
    all[i] = i;
  std::string encoded;
  URL::Encode(all, true, &encoded);
  std::string decoded;
  EXPECT_TRUE(URL::Decode(encoded, &decoded));
  EXPECT_EQ(all, decoded);
}

TEST(URL, Parse) {
  static struct {
    const char* url;
    const char* scheme;
    const char* userinfo;
    const char* host;
    const char* port;
    const char* path;
    const char* query;
    const char* fragment;
  } tests[] = {
    { "",
      "", "", "", "", "", "", "" },
    { "://",
      "", "", "", "", "", "", "" },
    { "/",
      "", "", "", "", "/", "", "" },
    { "10",
      "", "", "10", "", "", "", "" },
    { "@",
      "", "", "", "", "", "", "" },
    { "-@",
      "", "-", "", "", "", "", "" },
    { "x://",
      "x", "", "", "", "", "", "" },
    { "#",
      "", "", "", "", "", "", "" },
    { "#section",
      "", "", "", "", "", "", "section" },
    { "/abs/path/current/host",
      "", "", "", "", "/abs/path/current/host", "", "" },
    { "user@",
      "", "user", "", "", "", "", "" },
    { "s://user:pass@",
      "s", "user:pass", "", "", "", "", "" },
    { "host",
      "", "", "host", "", "", "", "" },
    { "?",
      "", "", "", "", "", "", "" },
    { "?query=1&ab=123",
      "", "", "", "", "", "query=1&ab=123", "" },
    { "google.com",
      "", "", "google.com", "", "", "", "" },
    { "http://google.com:",
      "http", "", "google.com", "", "", "", "" },
    { "http://google.com:123",
      "http", "", "google.com", "123", "", "", "" },
    { "http://google.com:/",
      "http", "", "google.com", "", "/", "", "" },
    { "http://google.com",
      "http", "", "google.com", "", "", "", "" },
    { "https://google.com",
      "https", "", "google.com", "", "", "", "" },
    { "https://user:pass@google.com:8443/path/to/stuff?query=1#anchor",
      "https", "user:pass", "google.com", "8443", "/path/to/stuff", "query=1",
      "anchor" },
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(tests); ++i) {
    URL url(tests[i].url);
    std::string error("Failed while parsing url: ");
    error += tests[i].url;
    EXPECT_EQ(tests[i].scheme, url.scheme()) << error;
    EXPECT_EQ(tests[i].userinfo, url.userinfo()) << error;
    EXPECT_EQ(tests[i].host, url.host()) << error;
    EXPECT_EQ(tests[i].port, url.port()) << error;
    EXPECT_EQ(tests[i].path, url.path()) << error;
    EXPECT_EQ(tests[i].query, url.query()) << error;
    EXPECT_EQ(tests[i].fragment, url.fragment()) << error;
  }
}

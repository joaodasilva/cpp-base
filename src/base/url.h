#ifndef BASE_URL_H
#define BASE_URL_H

#include <string>

#include "base/base.h"

class URL {
 public:
  URL();
  explicit URL(const std::string& url);
  URL(const URL& url);
  URL(URL&& url);
  ~URL();

  URL& operator=(const URL& url);
  URL& operator=(URL&& url);

  const std::string& scheme() const { return scheme_; }
  const std::string& userinfo() const { return userinfo_; }
  const std::string& host() const { return host_; }
  const std::string& port() const { return port_; }
  const std::string& path() const { return path_; }
  const std::string& query() const { return query_; }
  const std::string& fragment() const { return fragment_; }

  std::string ToString() const;

  // |space_with_plus| encodes ' ' with '+'. This is required for form encoded
  // data.
  static void Encode(const std::string& decoded,
                     bool space_with_plus,
                     std::string* encoded);

  // Returns false if the data in |encoded| is not valid URL encoded data.
  static bool Decode(const std::string& encoded, std::string* decoded);

 private:
  std::string scheme_;
  std::string userinfo_;
  std::string host_;
  std::string port_;
  std::string path_;
  std::string query_;
  std::string fragment_;
};

#endif  // BASE_URL_H

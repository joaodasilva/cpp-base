#include "base/url.h"

#include <ctype.h>
#include <stdlib.h>

// http://tools.ietf.org/html/rfc1738
// http://tools.ietf.org/html/rfc1808
// http://tools.ietf.org/html/rfc3986

namespace {

unsigned int from_hex(unsigned int n) {
  if (n >= '0' && n <= '9')
    return n - '0';
  if (n >= 'a' && n <= 'f')
    return n - 'a' + 10;
  return n - 'A' + 10;
}

}  // namespace

URL::URL() {}

URL::URL(const std::string& url) {
  size_t size = url.size();
  size_t begin = 0;
  size_t end = size;
  size_t k = url.find(':');
  if (k >= begin && k < end) {
    scheme_ = url.substr(begin, k-begin);
    if (k + 2 >= end || url[k + 1] != '/' || url[k + 2] != '/') {
      // Not an absolute URL.
      return;
    }
    begin = k + 3;
  }

  k = url.rfind('#');
  if (k >= begin && k < end) {
    fragment_ = url.substr(k + 1, end - k - 1);
    end = k;
  }

  k = end > 0 ? url.rfind('?', end - 1) : size;
  if (k >= begin && k < end) {
    query_ = url.substr(k + 1, end - k - 1);
    end = k;
  }

  k = url.find('/', begin);
  if (k >= begin && k < end) {
    path_ = url.substr(k, end - k);
    end = k;
  }

  // [begin, end) contains [user@]host[:port]

  k = url.find('@', begin);
  if (k >= begin && k < end) {
    userinfo_ = url.substr(begin, k - begin);
    begin = k + 1;
  }

  k = end > 0 ? url.rfind(':', end - 1) : size;
  if (k >= begin && k < end) {
    port_ = url.substr(k + 1, end - k - 1);
    end = k;
  }

  host_ = url.substr(begin, end - begin);
}

URL::URL(const URL& url)
    : scheme_(url.scheme_),
      userinfo_(url.userinfo_),
      host_(url.host_),
      port_(url.port_),
      path_(url.path_),
      query_(url.query_),
      fragment_(url.fragment_) {}

URL::URL(URL&& url) {
  scheme_.swap(url.scheme_);
  userinfo_.swap(url.userinfo_);
  host_.swap(url.host_);
  port_.swap(url.port_);
  path_.swap(url.path_);
  query_.swap(url.query_);
  fragment_.swap(url.fragment_);
}

URL::~URL() {}

URL& URL::operator=(const URL& url) {
  scheme_ = url.scheme_;
  userinfo_ = url.userinfo_;
  host_ = url.host_;
  port_ = url.port_;
  path_ = url.path_;
  query_ = url.query_;
  fragment_ = url.fragment_;
  return *this;
}

URL& URL::operator=(URL&& url) {
  scheme_.swap(url.scheme_);
  userinfo_.swap(url.userinfo_);
  host_.swap(url.host_);
  port_.swap(url.port_);
  path_.swap(url.path_);
  query_.swap(url.query_);
  fragment_.swap(url.fragment_);
  return *this;
}

std::string URL::ToString() const {
  std::string result;
  if (!scheme_.empty())
    result.append(scheme_).append("://");
  if (!userinfo_.empty())
    result.append(userinfo_).append("@");
  result.append(host_);
  if (!port_.empty())
    result.append(":").append(port_);
  if (path_.empty())
    result.append("/");
  else
    result.append(path_);
  if (!query_.empty())
    result.append("?").append(query_);
  if (!fragment_.empty())
    result.append("#").append(fragment_);
  return result;
}

// static
void URL::Encode(const std::string& decoded,
                 bool space_with_plus,
                 std::string* encoded) {
  // This implementation encodes more than necessary, but should be compatible
  // with every other.
  static const char* hex = "0123456789ABCDEF";
  const size_t size = decoded.size();
  size_t encoded_size = size;
  for (size_t i = 0; i < size; ++i) {
    if (!isalnum(decoded[i]))
      encoded_size += (decoded[i] == ' ' && space_with_plus) ? 0 : 2;
  }
  encoded->resize(encoded_size);
  for (size_t i = 0, j = 0; i < size; ++i, ++j) {
    unsigned int n = (unsigned int) decoded[i];
    if (isalnum(n)) {
      (*encoded)[j] = n;
    } else {
      if (n == ' ' && space_with_plus) {
        (*encoded)[j] = '+';
      } else {
        (*encoded)[j] = '%';
        (*encoded)[j+1] = hex[(n >> 4) & 0xf];
        (*encoded)[j+2] = hex[n & 0xf];
        j += 2;
      }
    }
  }
}

// static
bool URL::Decode(const std::string& encoded, std::string* decoded) {
  const size_t size = encoded.size();
  decoded->resize(size);
  size_t j = 0;
  for (size_t i = 0; i < size; ++i, ++j) {
    if (encoded[i] == '%') {
      if (i + 2 >= size || !isxdigit(encoded[i+1]) || !isxdigit(encoded[i+2]))
        return false;
      (*decoded)[j] = (from_hex(encoded[i+1]) << 4) | from_hex(encoded[i+2]);
      i += 2;
    } else if (encoded[i] == '+') {
      (*decoded)[j] = ' ';
    } else {
      (*decoded)[j] = encoded[i];
    }
  }
  decoded->resize(j);
  return true;
}

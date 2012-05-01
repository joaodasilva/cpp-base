#include "base/string_utils.h"

size_t SkipSpaces(const std::string& s, size_t begin) {
  while (begin < s.size() && IsSpace(s[begin]))
    ++begin;
  return begin;
}

size_t RSkipSpaces(const std::string& s, size_t rbegin) {
  while (rbegin > 0 && IsSpace(s[rbegin - 1]))
    --rbegin;
  return rbegin;
}

void SplitString(const std::string& s, char sep, std::vector<std::string>* v) {
  v->clear();
  size_t begin = SkipSpaces(s, 0);
  size_t end = s.find_first_of(sep, 0);
  while (end != std::string::npos) {
    size_t end_no_spaces = RSkipSpaces(s, end);
    if (end_no_spaces < begin)
      end_no_spaces = begin;
    v->push_back(s.substr(begin, end_no_spaces - begin));
    begin = SkipSpaces(s, end + 1);
    end = s.find_first_of(sep, begin);
  }
  if (begin < s.size() || !v->empty()) {
    end = RSkipSpaces(s, s.size());
    if (end < begin)
      end = begin;
    v->push_back(s.substr(begin, end - begin));
  }
}

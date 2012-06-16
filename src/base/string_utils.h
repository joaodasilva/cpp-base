#ifndef BASE_STRING_UTILS_H
#define BASE_STRING_UTILS_H

#include <string>
#include <vector>

#include "base/base.h"

inline bool IsSpace(int n) { return n == ' ' || n == '\t'; }

size_t SkipSpaces(const std::string& s, size_t begin);

size_t RSkipSpaces(const std::string& s, size_t rbegin);

void SplitString(const std::string& s, char sep, std::vector<std::string>* v);

bool StringToUnsigned(const std::string& s, uint32* value);

#endif  // BASE_STRING_UTILS_H

//
// Created by yy on 17-8-5.
//
#include <iostream>
#include <string>
#include <vector>

// 注意：当字符串为空时，也会返回一个空字符串
void split(std::string &s, std::string &delim, std::vector<std::string> *ret) {
  size_t last = 0;
  size_t index = s.find_first_of(delim, last);
  while (index != std::string::npos) {
    ret->push_back(s.substr(last, index - last));
    last = index + 1;
    index = s.find_first_of(delim, last);
  }
  if (index - last > 0) {
    ret->push_back(s.substr(last, index - last));
  }
}

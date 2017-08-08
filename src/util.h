//
// Created by yy on 17-8-5.
//

#ifndef LIBRDNS_UTIL_H
#define LIBRDNS_UTIL_H

#include <iostream>
#include <string>
#include <vector>

void split(std::string &s, std::string &delim, std::vector<std::string> *ret);

bool end_with(const std::string &str, const std::string &suffix);

#endif //LIBRDNS_UTIL_H

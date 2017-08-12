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

bool end_with(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void match_dns_rule(std::vector<std::vector<std::string>> &domains, std::string &domain, bool *matched, std::string *dns_server, bool *blocked) {
  for (int i = 0; i < domains.size(); ++i) {
    std::string rule(domains.at(i).at(0).c_str());
    if (rule == "server=" || rule == "domain=") {
      if (domain == domains.at(i).at(1)) {
        *matched = true;
        *dns_server = domains.at(i).at(2);
        break;
      }
    } else if (rule == "domain_keyword=") {
      if (domain.find(domains.at(i).at(1), 0) != std::string::npos) {
        *matched = true;
        *dns_server = domains.at(i).at(2);
        break;
      }
    } else if (rule == "domain_suffix=") {
      if (end_with(domain, domains.at(i).at(1))) {
        *matched = true;
        *dns_server = domains.at(i).at(2);
        break;
      }
    } else if (rule == "block=") {
      std::string block_rule(domains.at(i).at(2).c_str());

      if (block_rule == "domain") {
        if (domain == domains.at(i).at(1)) {
          *blocked = true;
          break;
        }
      } else if (block_rule == "domain_keyword") {
        if (domain.find(domains.at(i).at(1), 0) != std::string::npos) {
          *blocked = true;
          break;
        }
      } else if (block_rule == "domain_suffix") {
        if (end_with(domain, domains.at(i).at(1))) {
          *blocked = true;
          break;
        }
      }
    }
  }
}

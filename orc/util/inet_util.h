#ifndef ORC_UTIL_INET_UTIL_H_
#define ORC_UTIL_INET_UTIL_H_

#include <vector>
#include <string>

namespace orc {

bool GetHostIp(std::vector<std::string>* ips, bool ipv6);

std::string GetHostIp(bool ipv6);

std::string GetHostName();

}  // namespace orc

#endif  // ORC_UTIL_INET_UTIL_H_

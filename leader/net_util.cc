/*!
 * \file net_util.h
 * \brief The net utility
 */
#include "leader/net_util.h"

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <vector>
#include <string>

namespace leader {

bool NetUtil::GetHostName(std::string &hostName) {
  char buf[128];
  if (0 == gethostname(buf, sizeof(buf))) {
    hostName = buf;
    return true;
  }
  return false;
}

bool NetUtil::GetIP(std::vector<std::string> &ips) {
  std::string hostName;
  if (!GetHostName(hostName)) {
    return false;
  }
  struct hostent *hent;
  hent = gethostbyname(hostName.c_str());
  for (uint32_t i = 0; hent->h_addr_list[i]; i++) {
    std::string ip = inet_ntoa(*(struct in_addr *) (hent->h_addr_list[i]));
    ips.push_back(ip);
  }
  return true;
}

bool NetUtil::ValidatePort(const std::string &portStr, int &port) {
  try {
    auto p = std::stoi(portStr);
    if (p > 65535) {
      return false;
    }
    port = p;
    return true;
  } catch (...) {
    return false;
  }
}

}  // namespace leader

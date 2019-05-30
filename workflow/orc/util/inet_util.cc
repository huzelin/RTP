#include "orc/util/inet_util.h"

#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netdb.h>

#include <string>

#include "orc/util/scope_guard.h"

namespace orc {

bool GetHostIp(std::vector<std::string>* ips, bool ipv6) {
  struct ifaddrs* ifaddr;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) {
    return false;
  }
  auto free_guard = MakeScopeGuard([ifaddr](){ freeifaddrs(ifaddr); });

  for (auto ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    int family = ifa->ifa_addr->sa_family;

    if (family == (ipv6 ? AF_INET6 : AF_INET)) {
      if (strncmp(ifa->ifa_name, "lo", 2) == 0) continue;

      if (getnameinfo(ifa->ifa_addr,
                      (ipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)),
                      host, NI_MAXHOST,
                      NULL, 0,
                      NI_NUMERICHOST) == 0)  {
        ips->emplace_back(host);
        return true;
      }
    }
  }

  return false;
}

std::string GetHostIp(bool ipv6) {
  static struct Initializer {
    Initializer() {
      struct ifaddrs* ifaddr;
      char host[NI_MAXHOST];

      if (getifaddrs(&ifaddr) == -1) {
        return;
      }

      auto free_guard = MakeScopeGuard([ifaddr](){ freeifaddrs(ifaddr); });

      for (auto ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        int family = ifa->ifa_addr->sa_family;

        if (family == AF_INET || family == AF_INET6) {
          if (strncmp(ifa->ifa_name, "lo", 2) == 0) continue;

          std::string* d = (family == AF_INET ? &ip : &ipv6);
          size_t s = (family == AF_INET ?
                      sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));

          if (getnameinfo(ifa->ifa_addr, s, host, NI_MAXHOST, NULL, 0,
                          NI_NUMERICHOST) == 0)  {
            d->assign(host);
          }
        }
      }
    }

    std::string ip;
    std::string ipv6;
  } ini;

  return ipv6 ? ini.ipv6 : ini.ip;
}

std::string GetHostName() {
  static struct Initializer {
    Initializer() {
      char buf[1024];
      if (gethostname(buf, sizeof(buf)) == 0) {
        hostname.assign(buf);
      }
    }

    std::string hostname;
  } ini;
  return ini.hostname;
}

}  // namespace orc

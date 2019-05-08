/*!
 * \file net_util.h
 * \brief The net utility
 */
#ifndef LEADER_NET_UTIL_H_
#define LEADER_NET_UTIL_H_

#include <string>
#include <vector>

namespace leader {

/**
 * @brief Pure toolbox class, no instalizing allowed
 */
class NetUtil {
 public:
  NetUtil() = delete;

  ~NetUtil() = delete;

  /**
   * @brief Get the first available hostname of current device
   * @param hostName string to store to
   * @return true if found a hostname
   */
  static bool GetHostName(std::string& hostName);

  /**
   * @brief Get the ip list of current device
   * @param ips
   * @return
   */
  static bool GetIP(std::vector<std::string>& ips);

  //  /**
  //   * @brief validate a ipv4 adress
  //   * @param ip
  //   * @return true if a valid ipv4 address
  //   */
  //  static bool ValidateIPv4(const std::string &ip);

  /**
   * @brief validate and covert a port number
   * @param portStr port string to validate
   * @param port port to store to
   * @return true if a valid port number
   */
  static bool ValidatePort(const std::string& portStr, int& port);
};

}  // namespace leader

#endif  // LEADER_NET_UTIL_H_

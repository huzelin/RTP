/*!
 * \file server_register.h
 * \brief The server register base class
 */
#ifndef LEADER_SERVER_REGISTER_H_
#define LEADER_SERVER_REGISTER_H_

#include <string>
#include <vector>

#include "leader/zkwrapper.h"

namespace leader {

class HeartbeatReporter;

class ServerRegister {
 public:
  ServerRegister();

  virtual ~ServerRegister();

  /**
   * @brief Initializer
   * @param zkHost zk host to report to
   * @param port local service port
   * @param ip local ip address, use first available local ip if not provided
   * @param hbInterval interval in seconds between checks
   * @return true if success
   */
  bool Init(const std::string& zkHost, uint32_t port,
            const std::string& ip = "", int32_t hbInterval = 5);

  /**
   * @brief Initializer
   * @param zk shared ZKWrapper, register does not take ownership of this object
   * @param port local service port
   * @param ip local ip address, use first available local ip if not provided
   * @param hbInterval interval in seconds between checks
   * @return true if success
   */
  bool Init(ZKWrapper* zk, uint32_t port,
            const std::string& ip = "", int32_t hbInterval = 5);

  void AddPath(const std::string& path);

  void RemovePath(const std::string& path, bool blocking = true);

  void RemovePath(const std::vector<std::string>& paths, bool blocking = true);

  virtual bool Start();

  virtual void Close();

 protected:
  ServerRegister(const ServerRegister&);

  ServerRegister& operator=(const ServerRegister&);

  bool AutomaticGetHost();

  bool AutomaticGetIP();

  bool SharedInitializer(uint32_t port,
                         const std::string& ip);
  HeartbeatReporter* heartbeat_reporter_;
  std::string host_;
  std::string ip_;
  uint32_t port_;
};

}  // namespace leader

#endif  // LEADER_SERVER_REGISTER_H_

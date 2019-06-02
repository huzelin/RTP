#include "hello_session.h"

#include "orc/framework/session_factory_mgr.h"
#include "orc/framework/pool_session_factory.h"

namespace orc {
namespace example {
namespace hello {

using HelloSessionFactory = orc::PoolSessionFactory<HelloSession>;

ORC_REGISTER_SESSION_FACTORY(HelloSessionFactory);

}  // namespace hello
}  // namespace example
}  // namespace orc

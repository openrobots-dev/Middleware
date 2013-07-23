
#include <r2p/BaseTransport.hpp>
#include <r2p/Middleware.hpp>

namespace r2p {


BaseTransport::BaseTransport(const char *namep)
:
  Named(namep),
  by_middleware(*this)
{}


BaseTransport::~BaseTransport() {}


} // namespace r2p

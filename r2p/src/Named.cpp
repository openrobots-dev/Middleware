
#include <r2p/Named.hpp>

namespace r2p {


Named::Named(const char *const namep)
:
  namep(namep)
{
  R2P_ASSERT(namep != NULL);
  R2P_ASSERT(namep[0] != 0);
  R2P_ASSERT(is_identifier(namep));
}


bool Named::has_name(const Named &obj, const char *namep) {

  return namep != NULL && 0 == ::strcmp(obj.namep, namep);
}


bool Named::is_identifier(const char *namep) {

  if (namep != NULL && namep != 0) {
    while ((*namep >= 'a' && *namep <= 'z') ||
           (*namep >= 'A' && *namep <= 'Z') ||
           (*namep >= '0' && *namep <= '9') ||
           *namep == '_') {
      ++namep;
    }
    return *namep == 0;
  }
  return false;
}


} // namespace r2p

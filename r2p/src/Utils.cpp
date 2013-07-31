
#include <r2p/Utils.hpp>

namespace r2p {


bool is_identifier(const char *namep) {

  if (namep != 0 && *namep != 0) {
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

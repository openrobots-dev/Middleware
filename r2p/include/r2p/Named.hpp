
#ifndef __R2P__NAMED_HPP__
#define __R2P__NAMED_HPP__

#include <r2p/common.hpp>

namespace r2p {


class Named {
private:
  const char *const namep;

public:
  const char *get_name() const;

protected:
  Named(const char *const namep);

public:
  static bool has_name(const Named &obj, const char *namep);
  static bool is_identifier(const char *namep);
};


inline
const char *Named::get_name() const {

  return namep;
}


} // namespace r2p
#endif // __R2P__NAMED_HPP__

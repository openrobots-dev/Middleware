
#ifndef __R2P__SPINEVENT_HPP__
#define __R2P__SPINEVENT_HPP__

#include <r2p/common.hpp>
#include <r2p/impl/SpinEvent_.hpp>
#include <r2p/Time.hpp>

namespace r2p {


class SpinEvent : private Uncopyable {
public:
  typedef SpinEvent_::Mask Mask;

private:
  SpinEvent_ impl;

public:
  bool signal(unsigned event_index) {
    return impl.signal(event_index);
  }

  Mask wait(const Time &timeout) {
    return impl.wait(timeout);
  }
};


} // namespace r2p
#endif // __R2P__SPINEVENT_HPP__

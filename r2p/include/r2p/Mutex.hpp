#pragma once

#include <r2p/common.hpp>
#include <r2p/impl/Mutex_.hpp>

namespace r2p {


class Mutex : private Uncopyable {
private:
  Mutex_ impl;

public:
  void acquire_unsafe();
  void release_unsafe();

  void acquire();
  void release();
};


inline
void Mutex::acquire_unsafe() {
  impl.acquire_unsafe();
}


inline
void Mutex::release_unsafe() {
  impl.release_unsafe();
}


inline
void Mutex::acquire() {
  impl.acquire();
}


inline
void Mutex::release() {
  impl.release();
}


} // namespace r2p

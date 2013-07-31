#pragma once

#include <r2p/common.hpp>
#include <r2p/impl/SimplePool_.hpp>

namespace r2p {


template<typename Item>
class SimplePool : private Uncopyable {
private:
  SimplePool_ impl;

public:
  Item *alloc_unsafe();
  void free_unsafe(Item *objp);

  Item *alloc();
  void free(Item *objp);
  void grow(Item array[], size_t length);

public:
  SimplePool();
  SimplePool(Item array[], size_t length);
};


template<typename Item> inline
Item *SimplePool<Item>::alloc_unsafe() {

  return impl.alloc_unsafe();
}


template<typename Item> inline
void SimplePool<Item>::free_unsafe(Item *objp) {

  impl.free_unsafe(objp);
}


template<typename Item> inline
Item *SimplePool<Item>::alloc() {

  return impl.alloc();
}


template<typename Item> inline
void SimplePool<Item>::free(Item *objp) {

  impl.free(objp);
}


template<typename Item> inline
void SimplePool<Item>::grow(Item array[], size_t length) {

  impl.grow(reinterpret_cast<void *>(array), length, sizeof(Item));
}


} // namespace r2p


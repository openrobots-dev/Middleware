#pragma once

#include <r2p/common.hpp>
#include <r2p/impl/MemoryPool_.hpp>

namespace r2p {


template<typename Item>
class MemoryPool : private Uncopyable {
private:
  MemoryPool_ impl;

public:
  size_t get_item_size() const;

  template<typename Allocator>
  Allocator get_allocator() const;

  Item *alloc_unsafe();
  void free_unsafe(Item *objp);

  Item *alloc();
  void free(Item *objp);
  void extend(Item array[], size_t length);

public:
  MemoryPool();
  MemoryPool(Item array[], size_t length);

  template<typename Allocator>
  MemoryPool(Allocator allocator);

  template<typename Allocator>
  MemoryPool(Item array[], size_t length, Allocator allocator);
};


template<typename Item>
inline
size_t MemoryPool<Item>::get_item_size() const {

  return impl.get_item_size();
}


template<typename Item>
template<typename Allocator>
inline
Allocator MemoryPool<Item>::get_allocator() const {

  return static_cast<Allocator>(impl.get_allocator());
}


template<typename Item>
inline
Item *MemoryPool<Item>::alloc_unsafe() {

  return reinterpret_cast<Item *>(impl.alloc_unsafe());
}


template<typename Item>
inline
void MemoryPool<Item>::free_unsafe(Item *objp) {

  impl.free_unsafe(reinterpret_cast<void *>(objp));
}


template<typename Item>
inline
Item *MemoryPool<Item>::alloc() {

  return reinterpret_cast<Item *>(impl.alloc());
}


template<typename Item>
inline
void MemoryPool<Item>::free(Item *objp) {

  impl.free(reinterpret_cast<void *>(objp));
}


template<typename Item>
inline
void MemoryPool<Item>::extend(Item array[], size_t length) {

  impl.extend(reinterpret_cast<void *>(array), length);
}


template<typename Item>
inline
MemoryPool<Item>::MemoryPool()
:
  impl(sizeof(Item))
{
  R2P_ASSERT(sizeof(Item) >= sizeof(void *));
}


template<typename Item>
inline
MemoryPool<Item>::MemoryPool(Item array[], size_t length)
:
  impl(reinterpret_cast<void *>(array), length, sizeof(Item))
{
  R2P_ASSERT(sizeof(Item) >= sizeof(void *));
}


template<typename Item>
template<typename Allocator>
inline
MemoryPool<Item>::MemoryPool(Allocator allocator)
:
  impl(sizeof(Item), reinterpret_cast<MemoryPool_::Allocator>(allocator))
{
  R2P_ASSERT(sizeof(Item) >= sizeof(void *));
}


template<typename Item>
template<typename Allocator>
inline
MemoryPool<Item>::MemoryPool(Item array[], size_t length, Allocator allocator)
:
  impl(reinterpret_cast<void *>(array), length, sizeof(Item),
       reinterpret_cast<MemoryPool_::Allocator>(allocator))
{
  R2P_ASSERT(sizeof(Item) >= sizeof(void *));
}


} // namespace r2p

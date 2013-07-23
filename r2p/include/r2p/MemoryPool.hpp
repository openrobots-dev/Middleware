
#ifndef __R2P__MEMORYPOOL_HPP__
#define __R2P__MEMORYPOOL_HPP__

#include <r2p/common.hpp>
#include <r2p/impl/MemoryPool_.hpp>

namespace r2p {


template<typename T>
class MemoryPool : private Uncopyable {
private:
  MemoryPool_ impl;

public:
  size_t get_block_length() const;

  template<typename Allocator>
  Allocator get_allocator() const;

  T *alloc();
  void free(T *objp);
  void grow(T array[], size_t arraylen);

public:
  MemoryPool();
  MemoryPool(T array[], size_t arraylen);

  template<typename Allocator>
  MemoryPool(Allocator allocator);

  template<typename Allocator>
  MemoryPool(T array[], size_t arraylen, Allocator allocator);
};


template<typename T>
inline
size_t MemoryPool<T>::get_block_length() const {

  return impl.get_block_length();
}


template<typename T>
template<typename Allocator>
inline
Allocator MemoryPool<T>::get_allocator() const {

  return static_cast<Allocator>(impl.get_allocator());
}


template<typename T>
inline
T *MemoryPool<T>::alloc() {

  return reinterpret_cast<T *>(impl.alloc());
}


template<typename T>
inline
void MemoryPool<T>::free(T *objp) {

  impl.free(reinterpret_cast<void *>(objp));
}


template<typename T>
inline
void MemoryPool<T>::grow(T array[], size_t arraylen) {

  impl.grow(reinterpret_cast<void *>(array), arraylen);
}


template<typename T>
inline
MemoryPool<T>::MemoryPool()
:
  impl(sizeof(T))
{
  R2P_ASSERT(sizeof(T) >= sizeof(void *));
}


template<typename T>
inline
MemoryPool<T>::MemoryPool(T array[], size_t arraylen)
:
  impl(reinterpret_cast<void *>(array), arraylen, sizeof(T))
{
  R2P_ASSERT(sizeof(T) >= sizeof(void *));
}


template<typename T>
template<typename Allocator>
inline
MemoryPool<T>::MemoryPool(Allocator allocator)
:
  impl(sizeof(T), static_cast<MemoryPool_::Allocator>(allocator))
{
  R2P_ASSERT(sizeof(T) >= sizeof(void *));
}


template<typename T>
template<typename Allocator>
inline
MemoryPool<T>::MemoryPool(T array[], size_t arraylen,
                          Allocator allocator)
:
  impl(reinterpret_cast<void *>(array), arraylen, sizeof(T),
       static_cast<MemoryPool_::Allocator>(allocator))
{
  R2P_ASSERT(sizeof(T) >= sizeof(void *));
}


} // namespace r2p
#endif // __R2P__MEMORYPOOL_HPP__

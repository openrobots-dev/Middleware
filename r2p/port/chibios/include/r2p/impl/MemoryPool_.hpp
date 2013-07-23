
#ifndef __R2P__MEMORYPOOL__HPP__
#define __R2P__MEMORYPOOL__HPP__

#include <r2p/common.hpp>
#include <ch.h>

namespace r2p {


class MemoryPool_ : private Uncopyable {
public:
  typedef ::memgetfunc_t Allocator;

private:
  ::MemoryPool pool;

public:
  operator ::MemoryPool * ();

  size_t get_block_length() const;
  Allocator get_allocator() const;

  void *alloc();
  void free(void *objp);
  void grow(void *arrayp, size_t arraylen);

public:
  MemoryPool_(size_t blocklen);
  MemoryPool_(void *arrayp, size_t arraylen, size_t blocklen);
  MemoryPool_(size_t blocklen, Allocator allocator);
  MemoryPool_(void *arrayp, size_t arraylen,
              size_t blocklen, Allocator allocator);
};


inline
MemoryPool_::operator ::MemoryPool * () {

  return &pool;
}


inline
size_t MemoryPool_::get_block_length() const {

  chSysLock();
  size_t blocklen = pool.mp_object_size;
  chSysUnlock();
  return blocklen;
}


inline
MemoryPool_::Allocator MemoryPool_::get_allocator() const {

  chSysLock();
  Allocator allocator = pool.mp_provider;
  chSysUnlock();
  return allocator;
}


inline
void *MemoryPool_::alloc() {

  return chPoolAlloc(&pool);
}


inline
void MemoryPool_::free(void *objp) {

  if (objp != NULL) {
    chPoolFree(&pool, objp);
  }
}


inline
void MemoryPool_::grow(void *arrayp, size_t arraylen) {

  chPoolLoadArray(&pool, arrayp, arraylen);
}


inline
MemoryPool_::MemoryPool_(size_t blocklen) {

  chPoolInit(&pool, blocklen, reinterpret_cast<Allocator>(NULL));
}


inline
MemoryPool_::MemoryPool_(void *arrayp, size_t arraylen, size_t blocklen) {

  chPoolInit(&pool, blocklen, reinterpret_cast<Allocator>(NULL));
  grow(arrayp, arraylen);
}


inline
MemoryPool_::MemoryPool_(size_t blocklen, Allocator allocator) {

  chPoolInit(&pool, blocklen, reinterpret_cast<Allocator>(allocator));
}


inline
MemoryPool_::MemoryPool_(void *arrayp, size_t arraylen, size_t blocklen,
                         Allocator allocator) {

  chPoolInit(&pool, blocklen, reinterpret_cast<Allocator>(allocator));
  grow(arrayp, arraylen);
}


} // namespace r2p
#endif // __R2P__MEMORYPOOL__HPP__

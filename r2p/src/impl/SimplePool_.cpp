
#include <r2p/impl/SimplePool_.hpp>

namespace r2p {


void *SimplePool_::alloc_unsafe() {

  if (headp != NULL) {
    void *blockp = headp;
    headp = headp->nextp;
    return blockp;
  }
  return NULL;
}


void SimplePool_::free_unsafe(void *objp) {

  if (objp != NULL) {
    reinterpret_cast<Header *>(objp)->nextp = headp;
    headp = reinterpret_cast<Header *>(objp);
  }
}


void *SimplePool_::alloc() {

  SysLock::acquire();
  if (headp != NULL) {
    void *blockp = headp;
    headp = headp->nextp;
    SysLock::release();
    return blockp;
  } else {
    SysLock::release();
    return NULL;
  }
}


void SimplePool_::free(void *objp) {

  if (objp != NULL) {
    SysLock::acquire();
    reinterpret_cast<Header *>(objp)->nextp = headp;
    headp = reinterpret_cast<Header *>(objp);
    SysLock::release();
  }
}


void SimplePool_::grow(void *arrayp, size_t length, size_t blocklen) {

  R2P_ASSERT(arrayp != NULL);
  R2P_ASSERT(length > 0);
  R2P_ASSERT(blocklen >= sizeof(void *));

  for (uint8_t *curp = reinterpret_cast<uint8_t *>(arrayp);
       length-- > 0; curp += blocklen) {
    free(curp);
  }
}


SimplePool_::SimplePool_()
:
  headp(NULL)
{}


SimplePool_::SimplePool_(void *arrayp, size_t length, size_t blocklen)
:
  headp(NULL)
{
  grow(arrayp, length, blocklen);
}


} // namespace r2p

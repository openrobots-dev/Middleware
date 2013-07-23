
#include <ch.h>


void *operator new (size_t size) {

  return chHeapAlloc(NULL, size);
}


void *operator new (size_t size, ::MemoryHeap *heapp) {

  return chHeapAlloc(heapp, size);
}


void *operator new [] (size_t size) {

  return chHeapAlloc(NULL, size);
}


void *operator new [] (size_t size, ::MemoryHeap *heapp) {

  return chHeapAlloc(heapp, size);
}


void operator delete (void *objp) {

  if (objp != NULL) {
    chHeapFree(objp);
  }
}


void operator delete [] (void *objp) {

  if (objp != NULL) {
    chHeapFree(objp);
  }
}

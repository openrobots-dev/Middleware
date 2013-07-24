
#include <ch.h>


void *operator new (size_t size) {

  return chHeapAlloc(NULL, (size > 0) ? size : 1);
}


void *operator new (size_t size, ::MemoryHeap *heapp) {

  return chHeapAlloc(heapp, (size > 0) ? size : 1);
}


void *operator new [] (size_t size) {

  return chHeapAlloc(NULL, (size > 0) ? size : 1);
}


void *operator new [] (size_t size, ::MemoryHeap *heapp) {

  return chHeapAlloc(heapp, (size > 0) ? size : 1);
}


void operator delete (void *objp) {

  if (objp != NULL) {
    chHeapFree(objp);
  }
}


void operator delete (void *objp, ::MemoryHeap *heapp) {

  (void)heapp;
  if (objp != NULL) {
    chHeapFree(objp);
  }
}


void operator delete [] (void *objp) {

  if (objp != NULL) {
    chHeapFree(objp);
  }
}


void operator delete [] (void *objp, ::MemoryHeap *heapp) {

  (void)heapp;
  if (objp != NULL) {
    chHeapFree(objp);
  }
}

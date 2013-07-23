
#ifndef __R2P__ARRAYQUEUE_HPP__
#define __R2P__ARRAYQUEUE_HPP__

#include <r2p/common.hpp>

namespace r2p {


template<typename T>
class ArrayQueue : private Uncopyable {
private:
  T *arrayp;
  size_t length;
  size_t count;
  T *headp;
  T *tailp;

public:
  size_t get_count_unsafe() const;
  bool post_unsafe(T entry);
  bool fetch_unsafe(T &entry);
  bool skip_unsafe();

  size_t get_length() const;
  size_t get_count() const;
  bool post(T entry);
  bool fetch(T &entry);
  bool skip();

public:
  ArrayQueue(T array[], size_t length);
};


// TODO: Low-level implementation


template<typename T> inline
size_t ArrayQueue<T>::get_count_unsafe() const {

  return count;
}


template<typename T> inline
bool ArrayQueue<T>::post_unsafe(T entry) {

  if (count < length) {
    ++count;
    *tailp = entry;
    if (++tailp >= &arrayp[length]) {
      tailp = &arrayp[0];
    }
    return true;
  }
  return false;
}


template<typename T> inline
bool ArrayQueue<T>::fetch_unsafe(T &entry) {

  if (count > 0) {
    --count;
    entry = *headp;
    if (++headp >= &arrayp[length]) {
      headp = &arrayp[0];
    }
    return true;
  }
  return false;
}


template<typename T> inline
bool ArrayQueue<T>::skip_unsafe() {

  if (count > 0) {
    --count;
    if (++headp >= &arrayp[length]) {
      headp = &arrayp[0];
    }
    return true;
  }
  return false;
}


template<typename T> inline
size_t ArrayQueue<T>::get_length() const {

  return length;
}


template<typename T> inline
size_t ArrayQueue<T>::get_count() const {

  SysLock::acquire();
  size_t n = count;
  SysLock::release();
  return n;
}


template<typename T> inline
bool ArrayQueue<T>::post(T entry) {

  SysLock::acquire();
  bool success = post_unsafe(entry);
  SysLock::release();
  return success;
}


template<typename T> inline
bool ArrayQueue<T>::fetch(T &entry) {

  SysLock::acquire();
  bool success = fetch_unsafe(entry);
  SysLock::release();
  return success;
}


template<typename T> inline
bool ArrayQueue<T>::skip() {

  SysLock::acquire();
  bool success = skip_unsafe();
  SysLock::release();
  return success;
}


template<typename T> inline
ArrayQueue<T>::ArrayQueue(T array[], size_t length)
:
  arrayp(array),
  length(length),
  count(0),
  headp(array),
  tailp(array)
{
  R2P_ASSERT(array != NULL);
  R2P_ASSERT(length > 0);
}


} // namespace r2p
#endif // __R2P__ARRAYQUEUE_HPP__

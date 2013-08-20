#pragma once

#include <r2p/common.hpp>

namespace r2p {


template<typename Item>
class ArrayQueue : private Uncopyable {
private:
  Item *arrayp;
  size_t length;
  size_t count;
  Item *headp;
  Item *tailp;

public:
  size_t get_count_unsafe() const;
  bool post_unsafe(Item item);
  bool fetch_unsafe(Item &item);
  bool skip_unsafe();

  size_t get_length() const;
  size_t get_count() const;
  bool post(Item item);
  bool fetch(Item &item);
  bool skip();

public:
  ArrayQueue(Item array[], size_t length);
};


// TODO: Low-level implementation


template<typename Item> inline
size_t ArrayQueue<Item>::get_count_unsafe() const {

  return count;
}


template<typename Item> inline
bool ArrayQueue<Item>::post_unsafe(Item item) {

  if (count < length) {
    ++count;
    *tailp = item;
    if (++tailp >= &arrayp[length]) {
      tailp = &arrayp[0];
    }
    return true;
  }
  return false;
}


template<typename Item> inline
bool ArrayQueue<Item>::fetch_unsafe(Item &item) {

  if (count > 0) {
    --count;
    item = *headp;
    if (++headp >= &arrayp[length]) {
      headp = &arrayp[0];
    }
    return true;
  }
  return false;
}


template<typename Item> inline
bool ArrayQueue<Item>::skip_unsafe() {

  if (count > 0) {
    --count;
    if (++headp >= &arrayp[length]) {
      headp = &arrayp[0];
    }
    return true;
  }
  return false;
}


template<typename Item> inline
size_t ArrayQueue<Item>::get_length() const {

  return length;
}


template<typename Item> inline
size_t ArrayQueue<Item>::get_count() const {

  return count;
}


template<typename Item> inline
bool ArrayQueue<Item>::post(Item item) {

  return safeguard(post_unsafe(item));
}


template<typename Item> inline
bool ArrayQueue<Item>::fetch(Item &item) {

  return safeguard(fetch_unsafe(item));
}


template<typename Item> inline
bool ArrayQueue<Item>::skip() {

  return safeguard(skip_unsafe());
}


template<typename Item> inline
ArrayQueue<Item>::ArrayQueue(Item array[], size_t length)
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

#pragma once

#include <r2p/common.hpp>

namespace r2p {


class SimplePool_ : private Uncopyable {
public:
  struct Header {
    Header *nextp;
  };

private:
  Header *headp;

public:
  void *alloc_unsafe();
  void free_unsafe(void *objp);

  void *alloc();
  void free(void *objp);
  void grow(void *arrayp, size_t length, size_t blocklen);

public:
  SimplePool_();
  SimplePool_(void *arrayp, size_t length, size_t blocklen);
};


} // namespace r2p


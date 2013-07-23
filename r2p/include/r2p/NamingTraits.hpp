
#ifndef __R2P__NAMING_TRAITS_HPP__
#define __R2P__NAMING_TRAITS_HPP__

#include <r2p/common.hpp>

namespace r2p {

class Middleware;
class Node;
class Topic;


template<typename T>
struct NamingTraits {};


template<>
struct NamingTraits<Middleware> {
  enum { MAX_LENGTH = 7 };
};


template<>
struct NamingTraits<Node> {
  enum { MAX_LENGTH = 8 };
};


template<>
struct NamingTraits<Topic> {
  enum { MAX_LENGTH = 16 };
};


} // namespace r2p
#endif // __R2P__NAMING_TRAITS_HPP__

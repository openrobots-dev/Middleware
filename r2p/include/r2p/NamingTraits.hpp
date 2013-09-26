#pragma once

#include <r2p/common.hpp>

namespace r2p {

class Middleware;
class Node;
class Topic;
class Transport;


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


template<>
struct NamingTraits<Transport> {
  enum { MAX_LENGTH = 8 };
};


} // namespace r2p

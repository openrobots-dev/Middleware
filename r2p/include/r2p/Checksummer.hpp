#pragma once

#include <r2p/common.hpp>

namespace r2p {


class Checksummer : private Uncopyable {
private:
  template<uintmax_t value>
  struct Static {
    enum {
      ACCUMULATOR = (value + Static<(value >> 8)>::ACCUMULATOR) & 0xFF,
      CHECKSUM = (0x100 - ACCUMULATOR) & 0xFF,
    };
  };

private:
  uint8_t accumulator;

public:
  uint8_t get_accumulator() const;
  uint8_t compute_checksum() const;
  bool check(uint8_t expected) const;

  void reset();
  void add(const char value);
  void add(const uint8_t value);
  void add(const uint8_t *chunkp, size_t length);
  void add(const void *chunkp, size_t length);
  template<typename T> void add(const T &value);
  template<uintmax_t value> void add();

public:
  operator uint8_t () const;

  Checksummer &operator += (const char value);
  Checksummer &operator += (const uint8_t value);
  template<typename T> Checksummer &operator += (const T &value);

public:
  Checksummer();
  Checksummer(const char value);
  Checksummer(const uint8_t value);
  Checksummer(const uint8_t *chunkp, size_t length);
  Checksummer(const void *chunkp, size_t length);
  template<typename T> Checksummer(const T &value);

public:
  template<uintmax_t value>
  static uint8_t accumulate();

  template<uintmax_t value>
  static uint8_t compute_checksum();
};


template<>
struct Checksummer::Static<0> {
  enum {
    ACCUMULATOR = 0,
    CHECKSUM = 0,
  };
};


inline
uint8_t Checksummer::get_accumulator() const {

  return accumulator;
}


inline
uint8_t Checksummer::compute_checksum() const {

  return ~accumulator + 1;
}


inline
bool Checksummer::check(uint8_t expected) const {

  // XXX return compute_checksum() == expected;
  (void)expected; return true;
}


inline
void Checksummer::reset() {

  accumulator = 0;
}


inline
void Checksummer::add(const char value) {

  add(static_cast<const uint8_t>(value));
}


inline
void Checksummer::add(const uint8_t value) {

  accumulator += value;
}


inline
void Checksummer::add(const void *chunkp, size_t length) {

  add(reinterpret_cast<const uint8_t *>(chunkp), length);
}


template<typename T> inline
void Checksummer::add(const T &value) {

  add(reinterpret_cast<const uint8_t *>(&value), sizeof(T));
}


inline
Checksummer::operator uint8_t () const {

  return compute_checksum();
}


inline
Checksummer &Checksummer::operator += (const char value) {

  add(value);
  return *this;
}


inline
Checksummer &Checksummer::operator += (const uint8_t value) {

  add(value);
  return *this;
}


template<typename T> inline
Checksummer &Checksummer::operator += (const T &value) {

  add(value);
  return *this;
}


inline
Checksummer::Checksummer()
:
  accumulator(0)
{}


inline
Checksummer::Checksummer(const uint8_t value)
:
  accumulator(value)
{}


inline
Checksummer::Checksummer(const char value)
:
  accumulator(static_cast<const uint8_t>(value))
{}


inline
Checksummer::Checksummer(const uint8_t *chunkp, size_t length)
:
  accumulator(0)
{
  add(chunkp, length);
}


inline
Checksummer::Checksummer(const void *chunkp, size_t length)
:
  accumulator(0)
{
  add(chunkp, length);
}


template<typename T> inline
Checksummer::Checksummer(const T &value)
:
  accumulator(0)
{
  add(value);
}


template<uintmax_t value> inline
uint8_t Checksummer::accumulate() {

  return static_cast<uint8_t>(Static<value>::ACCUMULATOR);
}


template<uintmax_t value> inline
uint8_t Checksummer::compute_checksum() {

  return static_cast<uint8_t>(Static<value>::CHECKSUM);
}


} // namespace r2p

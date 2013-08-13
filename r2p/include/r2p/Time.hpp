#pragma once

#include <r2p/common.hpp>

namespace r2p {


class Time { // TODO: Fix infinite algebra
public:
  typedef int32_t Type;

  enum {
    MIN_US =  (1 << (8 * sizeof(Type) - 1)) + 1,
    MAX_US = ~(1 << (8 * sizeof(Type) - 1)) - 1,

    MIN_MS = MIN_US / 1000,
    MAX_MS = MAX_US / 1000,

    MIN_S = MIN_US / 1000000,
    MAX_S = MAX_US / 1000000,

    MIN_M = MAX_US / 60000000,
    MAX_M = MAX_US / 60000000,
  };

public:
  Type raw;

public:
  Type to_us_raw() const;
  Type to_ms_raw() const;
  Type to_s_raw() const;
  Type to_m_raw() const;

  double to_us() const;
  double to_ms() const;
  double to_s() const;
  double to_m() const;

  Time &operator = (const Time &rhs);
  Time &operator += (const Time &rhs);
  Time &operator -= (const Time &rhs);

public:
  Time();
  template<typename T> Time(T microseconds);
  explicit Time(double seconds);
  Time(const Time &rhs);

public:
  static Time us(const Type microseconds);
  static Time ms(const Type milliseconds);
  static Time s(const Type seconds);
  static Time m(const Type minutes);
  static Time hz(const double hertz);
  static Time now();

public:
  static const Time IMMEDIATE;
  static const Time INFINITE;
  static const Time INFINITEN;
};

bool operator == (const Time &lhs, const Time &rhs);
bool operator != (const Time &lhs, const Time &rhs);
bool operator >  (const Time &lhs, const Time &rhs);
bool operator >= (const Time &lhs, const Time &rhs);
bool operator <  (const Time &lhs, const Time &rhs);
bool operator <= (const Time &lhs, const Time &rhs);
const Time operator + (const Time &lhs, const Time &rhs);
const Time operator - (const Time &lhs, const Time &rhs);


inline
Time::Type Time::to_us_raw() const {

  return raw;
}


inline
Time::Type Time::to_ms_raw() const {

  return raw / 1000;
}


inline
Time::Type Time::to_s_raw() const {

  return raw / 1000000;
}


inline
Time::Type Time::to_m_raw() const {

  return raw / 60000000;
}


inline
double Time::to_us() const {

  return raw;
}


inline
double Time::to_ms() const {

  return raw / 1000.0;
}


inline
double Time::to_s() const {

  return raw / 1000000.0;
}


inline
double Time::to_m() const {

  return raw / 60000000.0;
}


inline
Time &Time::operator = (const Time &rhs) {

  raw = rhs.raw;
  return *this;
}


inline
Time &Time::operator += (const Time &rhs) {

  raw += rhs.raw;
  return *this;
}


inline
Time &Time::operator -= (const Time &rhs) {

  raw += rhs.raw;
  return *this;
}


inline
Time::Time() : raw() {}


template<typename T> inline
Time::Time(T microseconds)
:
  raw(static_cast<Type>(microseconds))
{}


inline
Time::Time(double seconds)
:
  raw(static_cast<Type>((seconds + 0.5) / 1000000.0))
{}


inline
Time::Time(const Time &rhs) : raw(rhs.raw) {}


inline
Time Time::us(const Type microseconds) {

  return Time(microseconds);
}


inline
Time Time::ms(const Type milliseconds) {

  return Time(milliseconds * 1000);
}


inline
Time Time::s(const Type seconds) {

  return Time(seconds * 1000000);
}


inline
Time Time::m(const Type minutes) {

  return Time(minutes * 60000000);
}


inline
Time Time::hz(const double hertz) {

  if (hertz < 1.0 / MAX_US) return Time(MAX_US);
  else if (hertz > 1000000.0) return Time(1);
  else return Time(static_cast<Type>(1000000.0 / hertz));
}


inline
bool operator == (const Time &lhs, const Time &rhs) {

  return lhs.raw == rhs.raw;
}


inline
bool operator != (const Time &lhs, const Time &rhs) {

  return lhs.raw != rhs.raw;
}


inline
bool operator > (const Time &lhs, const Time &rhs) {

  return lhs.raw > rhs.raw;
}


inline
bool operator >= (const Time &lhs, const Time &rhs) {

  return lhs.raw >= rhs.raw;
}


inline
bool operator < (const Time &lhs, const Time &rhs) {

  return lhs.raw < rhs.raw;
}


inline
bool operator <= (const Time &lhs, const Time &rhs) {

  return lhs.raw <= rhs.raw;
}


inline
const Time operator + (const Time &lhs, const Time &rhs) {

  return Time(lhs.raw + rhs.raw);
}


inline
const Time operator - (const Time &lhs, const Time &rhs) {

  return Time(lhs.raw - rhs.raw);
}


}; // namespace r2p

#pragma once

namespace r2p {


class Uncopyable {
private:
  // Must not be defined
  Uncopyable(const Uncopyable &);

  // Must not be defined
  template<typename T>
  Uncopyable(const T &);

  // Must not be defined
  template<typename T>
  Uncopyable &operator = (const T &);

protected:
  Uncopyable() {}
  ~Uncopyable() {}
};


} // namespace r2p

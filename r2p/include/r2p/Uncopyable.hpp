#pragma once

namespace r2p {


class Uncopyable {
private:
  template<typename T>
  Uncopyable(const T &);

  template<typename T>
  Uncopyable &operator = (const T &);

protected:
  Uncopyable() {}
  ~Uncopyable() {}
};


} // namespace r2p

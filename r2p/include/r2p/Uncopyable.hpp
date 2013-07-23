
#ifndef __R2P__UNCOPYABLE_HPP__
#define __R2P__UNCOPYABLE_HPP__

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
#endif // __R2P__UNCOPYABLE_HPP__

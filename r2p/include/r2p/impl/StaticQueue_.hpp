
#ifndef __R2P__STATICQUEUE__HPP__
#define __R2P__STATICQUEUE__HPP__

#include <r2p/common.hpp>

namespace r2p {


class StaticQueue_ : private Uncopyable {
public:
  struct Link {
    Link *nextp;
    void *datap;

    Link(void *datap) :
      nextp(NULL),
      datap(datap)
    {}
  };

private:
  Link *headp;
  Link *tailp;

public:
  const Link *get_head_unsafe() const;
  const Link *get_tail_unsafe() const;
  bool is_empty_unsafe() const;
  void post_unsafe(Link &link);
  bool peek_unsafe(const Link *&linkp) const;
  bool fetch_unsafe(Link &link);
  bool skip_unsafe();

  const Link *get_head() const;
  const Link *get_tail() const;
  bool is_empty() const;
  void post(Link &link);
  bool peek(const Link *&linkp) const;
  bool fetch(Link &link);
  bool skip();

public:
  StaticQueue_();
};


inline
bool StaticQueue_::is_empty_unsafe() const {

  return headp == NULL;
}


} // namespace r2p
#endif // __R2P__STATICQUEUE__HPP__

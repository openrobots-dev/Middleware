#pragma once

#include <r2p/common.hpp>

namespace r2p {


class StaticList_ : private Uncopyable {
public:
  struct Link {
    Link *nextp;
    void *itemp;

    Link(void *itemp) :
      nextp(NULL),
      itemp(itemp)
    {}
  };

  typedef bool (*Predicate)(const void *itemp);
  typedef bool (*Matches)(const void *itemp, const void *featuresp);

private:
  Link *headp;

public:
  const Link *get_head_unsafe() const;
  bool is_empty_unsafe() const;
  size_t get_count_unsafe() const;
  void link_unsafe(Link &link);
  bool unlink_unsafe(Link &link);
  int index_of_unsafe(const Link &link) const;
  const Link *find_first_unsafe(Predicate pred_func) const;
  const Link *find_first_unsafe(Matches match_func, const void *featuresp) const;

  const Link *get_head() const;
  bool is_empty() const;
  size_t get_count() const;
  void link(Link &link);
  bool unlink(Link &link);
  int index_of(const Link &link) const;
  const Link *find_first(Predicate pred_func) const;
  const Link *find_first(Matches match_func, const void *featuresp) const;

public:
  StaticList_();
};


inline
const StaticList_::Link *StaticList_::get_head_unsafe() const {

  return headp;
}


inline
bool StaticList_::is_empty_unsafe() const {

  return headp == NULL;
}


} // namespace r2p

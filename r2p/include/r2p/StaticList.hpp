#pragma once

#include <r2p/common.hpp>
#include <r2p/impl/StaticList_.hpp>

namespace r2p {


#if !defined(R2P_FORCE_STATICLIST_IMPL) || defined(__DOXYGEN__)
#define R2P_FORCE_STATICLIST_IMPL   0
#endif


template<typename Item>
class StaticList : private Uncopyable {
public:
  struct Link {
    Link *nextp;
    Item *const itemp;

    Link(Item &item) : nextp(NULL), itemp(&item) {}
  };

  struct ConstLink {
    ConstLink *nextp;
    const Item *const itemp;

    ConstLink(const Item &item) : nextp(NULL), itemp(&item) {}
    ConstLink(const Link &link) : nextp(link.nextp), itemp(link.itemp) {}
  };

  class IteratorUnsafe {
    friend class StaticList;

  private:
    const Link *curp;

  public:
    IteratorUnsafe(const IteratorUnsafe &rhs) : curp(rhs.curp) {}

  private:
    IteratorUnsafe(const Link *beginp) : curp(beginp) {}

  private:
    IteratorUnsafe &operator = (const IteratorUnsafe &rhs) {
      curp = rhs.curp;
      return *this;
    }

  public:
    Item *operator -> () const {
      return curp->itemp;
    }

    Item &operator * () const {
      return *curp->itemp;
    }

    IteratorUnsafe &operator ++ () {
      curp = curp->nextp;
      return *this;
    }

    const IteratorUnsafe operator ++ (int) {
      IteratorUnsafe old(*this);
      curp = curp->nextp;
      return old;
    }

    bool operator == (const IteratorUnsafe &rhs) const {
      return this->curp == rhs.curp;
    }

    bool operator != (const IteratorUnsafe &rhs) const {
      return this->curp != rhs.curp;
    }
  };

  class ConstIteratorUnsafe {
    friend class StaticList;

  private:
    const ConstLink *curp;

  public:
    ConstIteratorUnsafe(const ConstIteratorUnsafe &rhs) : curp(rhs.curp) {}

  private:
    ConstIteratorUnsafe(const ConstLink *beginp) : curp(beginp) {}

  private:
    ConstIteratorUnsafe &operator = (const ConstIteratorUnsafe &rhs) {
      curp = rhs.curp;
      return *this;
    }

  public:
    const Item *operator -> () const {
      return curp->itemp;
    }

    const Item &operator * () const {
      return *curp->itemp;
    }

    ConstIteratorUnsafe &operator ++ () {
      curp = curp->nextp;
      return *this;
    }

    const ConstIteratorUnsafe operator ++ (int) {
      ConstIteratorUnsafe old(*this);
      curp = curp->nextp;
      return old;
    }

    bool operator == (const ConstIteratorUnsafe &rhs) const {
      return this->curp == rhs.curp;
    }

    bool operator != (const ConstIteratorUnsafe &rhs) const {
      return this->curp != rhs.curp;
    }
  };

  class Iterator {
    friend class StaticList;

  private:
    const Link *curp;

  public:
    Iterator(const Iterator &rhs) : curp(rhs.curp) {}

  private:
    Iterator(const Link *beginp) : curp(beginp) {}

  private:
    Iterator &operator = (const Iterator &rhs) {
      curp = rhs.curp;
      return *this;
    }

  public:
    Item *operator -> () const {
      return curp->itemp;
    }

    Item &operator * () const {
      return *curp->itemp;
    }

    Iterator &operator ++ () {
      SysLock::acquire();
      curp = curp->nextp;
      SysLock::release();
      return *this;
    }

    const Iterator operator ++ (int) {
      SysLock::acquire();
      Iterator old(*this);
      curp = curp->nextp;
      SysLock::release();
      return old;
    }

    bool operator == (const Iterator &rhs) const {
      return this->curp == rhs.curp;
    }

    bool operator != (const Iterator &rhs) const {
      return this->curp != rhs.curp;
    }
  };

  class ConstIterator {
    friend class StaticList;

  private:
    const ConstLink *curp;

  public:
    ConstIterator(const ConstIterator &rhs) : curp(rhs.curp) {}

  private:
    ConstIterator(const ConstLink *beginp) : curp(beginp) {}

  private:
    ConstIterator &operator = (const ConstIterator &rhs) {
      curp = rhs.curp;
      return *this;
    }

  public:
    const Item *operator -> () const {
      return curp->itemp;
    }

    const Item &operator * () const {
      return *curp->itemp;
    }

    ConstIterator &operator ++ () {
      SysLock::acquire();
      curp = curp->nextp;
      SysLock::release();
      return *this;
    }

    const ConstIterator operator ++ (int) {
      SysLock::acquire();
      ConstIterator old(*this);
      curp = curp->nextp;
      SysLock::release();
      return old;
    }

    bool operator == (const ConstIterator &rhs) const {
      return this->curp == rhs.curp;
    }

    bool operator != (const ConstIterator &rhs) const {
      return this->curp != rhs.curp;
    }
  };

#if R2P_FORCE_STATICLIST_IMPL

private:
  typedef StaticList_::Link Node_impl;

private:
  StaticList_ impl;

public:
  bool is_empty_unsafe() const {
    return impl.is_empty_unsafe();
  }

  size_t count_unsafe() const {
    return impl.count_unsafe();
  }

  void link_unsafe(Link &link) {
    impl.link_unsafe(reinterpret_cast<Node_impl &>(link));
  }

  bool unlink_unsafe(Link &link) {
    return impl.unlink_unsafe(reinterpret_cast<Node_impl &>(link));
  }

  int index_of_unsafe(const Item &item) const {
    return impl.index_of_unsafe(reinterpret_cast<const void *>(&item));
  }

  bool contains_unsafe(const Item &item) const {
    return impl.contains_unsafe(reinterpret_cast<const void *>(&item));
  }

  template<typename Predicate>
  Item *find_first_unsafe(Predicate pred_func) const {
    return reinterpret_cast<Item *>(impl.find_first_unsafe(
      reinterpret_cast<StaticList_::Predicate>(pred_func)
    ));
  }

  template<typename Matches, typename Reference>
  Item *find_first_unsafe(Matches match_func, const Reference &reference)
  const {
    return reinterpret_cast<Item *>(impl.find_first_unsafe(
      reinterpret_cast<StaticList_::Matches>(match_func), reference
    ));
  }

  IteratorUnsafe begin_unsafe() {
    return IteratorUnsafe(
      reinterpret_cast<const Link *>(impl.get_head_unsafe())
    );
  }

  IteratorUnsafe end_unsafe() {
    return IteratorUnsafe(NULL);
  }

  void restart_unsafe(IteratorUnsafe &iterator) {
    iterator = begin_unsafe();
  }

  const ConstIteratorUnsafe begin_unsafe() const {
    return ConstIteratorUnsafe(
      reinterpret_cast<const ConstLink *>(impl.get_head_unsafe())
    );
  }

  const ConstIteratorUnsafe end_unsafe() const {
    return ConstIteratorUnsafe(NULL);
  }

  void restart_unsafe(ConstIteratorUnsafe &iterator) const {
    iterator = begin_unsafe();
  }

  bool is_empty() const {
    return impl.is_empty();
  }

  size_t count() const {
    return impl.count();
  }

  void link(Link &link) {
    impl.link(reinterpret_cast<Node_impl &>(link));
  }

  bool unlink(Link &link) {
    return impl.unlink(reinterpret_cast<Node_impl &>(link));
  }

  int index_of(const Item &item) const {
    return impl.index_of(reinterpret_cast<const void *>(&item));
  }

  bool contains(const Item &item) const {
    return impl.contains(reinterpret_cast<const void *>(&item));
  }

  template<typename Predicate>
  Item *find_first(Predicate pred_func) const {
    return reinterpret_cast<Item *>(
      impl.find_first(reinterpret_cast<StaticList_::Predicate>(pred_func))
    );
  }

  template<typename Matches, typename Reference>
  Item *find_first(Matches match_func, const Reference &reference) const {
    return reinterpret_cast<Item *>(
      impl.find_first(reinterpret_cast<StaticList_::Matches>(match_func),
                      reference)
    );
  }

  const Iterator begin() {
    return Iterator(reinterpret_cast<const Link *>(impl.get_head()));
  }

  const Iterator end() {
    return Iterator(NULL);
  }

  void restart(Iterator &iterator) {
    iterator = begin();
  }

  const ConstIterator begin() const {
    return ConstIterator(reinterpret_cast<const ConstLink *>(impl.get_head()));
  }

  const ConstIterator end() const {
    return ConstIterator(NULL);
  }

  void restart(ConstIterator &iterator) const {
    iterator = begin();
  }

#else // !R2P_FORCE_STATICLIST_IMPL

private:
  typedef bool (*Predicate)(const Item &item);
  typedef bool (*Matches)(const Item &item, const void *featuresp);

private:
  Link *headp;

public:
  const Link *get_head_unsafe() const {
    return headp;
  }

  bool is_empty_unsafe() const {
    return headp != NULL;
  }

  size_t count_unsafe() const;

  void link_unsafe(Link &link) {
    R2P_ASSERT(link.nextp == NULL);
    R2P_ASSERT(!unlink_unsafe(link));

    link.nextp = headp;
    headp = &link;
  }

  bool unlink_unsafe(Link &link);
  int index_of_unsafe(const Item &item) const;
  bool contains_unsafe(const Item &item) const;
  Item *find_first_unsafe(Predicate pred_func) const;
  Item *find_first_unsafe(Matches match_func, const void *featuresp) const;

  const IteratorUnsafe begin_unsafe() {
    return IteratorUnsafe(get_head_unsafe());
  }

  const IteratorUnsafe end_unsafe() {
    return IteratorUnsafe(NULL);
  }

  void restart_unsafe(IteratorUnsafe &iterator) {
    iterator = begin_unsafe();
  }

  const ConstIteratorUnsafe begin_unsafe() const {
    return ConstIteratorUnsafe(get_head_unsafe());
  }

  const ConstIteratorUnsafe end_unsafe() const {
    return ConstIteratorUnsafe(NULL);
  }

  void restart_unsafe(ConstIteratorUnsafe &iterator) const {
    iterator = begin_unsafe();
  }

  const Link *get_head() const;
  bool is_empty() const;
  size_t count() const;
  void link(Link &link);
  bool unlink(Link &link);
  int index_of(const Item &item) const;
  bool contains(const Item &item) const;
  Item *find_first(Predicate pred_func) const;
  Item *find_first(Matches match_func, const void *featuresp) const;

  template<typename Predicate>
  Item *find_first(Predicate pred_func) const {
    return find_first(reinterpret_cast<StaticList::Predicate>(pred_func));
  }

  template<typename Matches, typename Features>
  Item *find_first(Matches match_func, const Features *featuresp) const {
    return find_first(reinterpret_cast<StaticList::Matches>(match_func),
                      reinterpret_cast<const void *>(featuresp));
  }

  const Iterator begin() {
    return Iterator(get_head());
  }

  const Iterator end() {
    return Iterator(NULL);
  }

  void restart(Iterator &iterator) {
    iterator = begin();
  }

  const ConstIterator begin() const {
    return ConstIterator(get_head_unsafe());
  }

  const ConstIterator end() const {
    return ConstIterator(NULL);
  }

  void restart(ConstIterator &iterator) const {
    iterator = begin();
  }

  StaticList() : headp(NULL) {}

#endif // R2P_FORCE_STATICLIST_IMPL
};


#if !R2P_FORCE_STATICLIST_IMPL

template<typename Item> inline
size_t StaticList<Item>::count_unsafe() const {

  size_t count = 0;
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    ++count;
  }
  return count;
}


template<typename Item> inline
bool StaticList<Item>::unlink_unsafe(Link &link) {

  for (Link *curp = headp, *prevp = NULL;
       curp != NULL;
       prevp = curp, curp = curp->nextp) {
    if (curp == &link) {
      if (prevp != NULL) prevp->nextp = curp->nextp;
      else headp = curp->nextp;
      curp->nextp = NULL;
      return true;
    }
  }
  return false;
}


template<typename Item> inline
int StaticList<Item>::index_of_unsafe(const Item &item) const {

  int i = 0;
  for (const Link *linkp = headp; linkp != NULL; ++i, linkp = linkp->nextp) {
    if (linkp->itemp == &item) {
      return i;
    }
  }
  return -1;
}


template<typename Item> inline
bool StaticList<Item>::contains_unsafe(const Item &item) const {

  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (linkp->itemp == &item) {
      return true;
    }
  }
  return false;
}


template<typename Item> inline
Item *StaticList<Item>::find_first_unsafe(Predicate pred_func) const {

  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (pred_func(linkp->itemp)) {
      return linkp->itemp;
    }
  }
  return NULL;
}


template<typename Item> inline
Item *StaticList<Item>::find_first_unsafe(Matches match_func,
                                          const void *featuresp) const {

  R2P_ASSERT(featuresp != NULL);

  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (match_func(linkp->itemp, featuresp)) {
      return linkp->itemp;
    }
  }
  return NULL;
}


template<typename Item> inline
const typename StaticList<Item>::Link *StaticList<Item>::get_head() const {

  SysLock::acquire();
  const Link *headp = get_head_unsafe();
  SysLock::release();
  return headp;
}


template<typename Item> inline
bool StaticList<Item>::is_empty() const {

  SysLock::acquire();
  bool success = is_empty_unsafe();
  SysLock::release();
  return success;
}


template<typename Item> inline
size_t StaticList<Item>::count() const {

  size_t count = 0;
  SysLock::acquire();
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    SysLock::release();
    ++count;
    SysLock::acquire();
  }
  SysLock::release();
  return count;
}


template<typename Item> inline
void StaticList<Item>::link(Link &link) {

  SysLock::acquire();
  link_unsafe(link);
  SysLock::release();
}


template<typename Item> inline
bool StaticList<Item>::unlink(Link &link) {

  SysLock::acquire();
  bool success = unlink_unsafe(link);
  SysLock::release();
  return success;
}


template<typename Item> inline
int StaticList<Item>::index_of(const Item &item) const {

  int i = 0;
  SysLock::acquire();
  for (const Link *linkp = headp; linkp != NULL; ++i, linkp = linkp->nextp) {
    SysLock::release();
    if (linkp->itemp == &item) {
      return i;
    }
    SysLock::acquire();
  }
  SysLock::release();
  return -1;
}


template<typename Item> inline
bool StaticList<Item>::contains(const Item &item) const {

  SysLock::acquire();
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (linkp->itemp == &item) {
      SysLock::release();
      return true;
    }
    SysLock::release();
    SysLock::acquire();
  }
  SysLock::release();
  return false;
}


template<typename Item> inline
Item *StaticList<Item>::find_first(Predicate pred_func) const {

  SysLock::acquire();
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    SysLock::release();
    if (pred_func(linkp->itemp)) {
      return linkp->itemp;
    }
    SysLock::acquire();
  }
  SysLock::release();
  return NULL;
}


template<typename Item> inline
Item *StaticList<Item>::find_first(Matches match_func,
                                   const void *featuresp) const {

  R2P_ASSERT(featuresp != NULL);

  SysLock::acquire();
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    SysLock::release();
    if (match_func(*linkp->itemp, featuresp)) {
      return linkp->itemp;
    }
    SysLock::acquire();
  }
  SysLock::release();
  return NULL;
}


#endif // !R2P_FORCE_STATICLIST_IMPL

} // namespace r2p


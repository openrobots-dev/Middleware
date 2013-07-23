
#ifndef __R2P__STATICLIST_HPP__
#define __R2P__STATICLIST_HPP__

#include <r2p/common.hpp>
#include <r2p/impl/StaticList_.hpp>

namespace r2p {


template<typename T>
class StaticList : private Uncopyable {
public:
  struct Link {
    Link *nextp;
    T *const datap;

    Link(T &data) : nextp(NULL), datap(&data) {}
  };

  struct ConstLink {
    ConstLink *nextp;
    const T *const datap;

    ConstLink(const T &data) : nextp(NULL), datap(&data) {}
    ConstLink(const Link &link) : nextp(link.nextp), datap(link.datap) {}
  };

  class IteratorUnsafe {
    friend class StaticList<T>;

  private:
    const Link *curp;

  public:
    IteratorUnsafe(const IteratorUnsafe &rhs) : curp(rhs.curp) {}

  private:
    IteratorUnsafe(const Link *beginp) : curp(beginp) {}

  private:
    IteratorUnsafe &operator = (const IteratorUnsafe &rhs);

  public:
    const Link *operator -> () const {
      return curp;
    }

    const Link &operator * () const {
      return *curp;
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
    friend class StaticList<T>;

  private:
    const ConstLink *curp;

  public:
    ConstIteratorUnsafe(const ConstIteratorUnsafe &rhs) : curp(rhs.curp) {}

  private:
    ConstIteratorUnsafe(const ConstLink *beginp) : curp(beginp) {}

  private:
    ConstIteratorUnsafe &operator = (const ConstIteratorUnsafe &rhs);

  public:
    const ConstLink *operator -> () const {
      return curp;
    }

    const ConstLink &operator * () const {
      return *curp;
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
    friend class StaticList<T>;

  private:
    const Link *curp;

  public:
    Iterator(const Iterator &rhs) : curp(rhs.curp) {}

  private:
    Iterator(const Link *beginp) : curp(beginp) {}

  private:
    Iterator &operator = (const Iterator &rhs);

  public:
    const Link *operator -> () const {
      return curp;
    }

    const Link &operator * () const {
      return *curp;
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
    friend class StaticList<T>;

  private:
    const ConstLink *curp;

  public:
    ConstIterator(const ConstIterator &rhs) : curp(rhs.curp) {}

  private:
    ConstIterator(const ConstLink *beginp) : curp(beginp) {}

  private:
    ConstIterator &operator = (const ConstIterator &rhs);

  public:
    const ConstLink *operator -> () const {
      return curp;
    }

    const ConstLink &operator * () const {
      return *curp;
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

private:
  typedef StaticList_::Link Node_impl;

private:
  StaticList_ impl;

public:
  bool is_empty_unsafe() const {
    return impl.is_empty_unsafe();
  }

  size_t get_count_unsafe() const {
    return impl.get_count_unsafe();
  }

  void link_unsafe(Link &link) {
    impl.link_unsafe(reinterpret_cast<Node_impl &>(link));
  }

  bool unlink_unsafe(Link &link) {
    return impl.unlink_unsafe(reinterpret_cast<Node_impl &>(link));
  }

  int index_of_unsafe(const Link &link) const {
    return impl.index_of_unsafe(
      reinterpret_cast<const Node_impl &>(link)
    );
  }

  template<typename Predicate>
  const Link *find_first_unsafe(Predicate pred_func) const {
    return reinterpret_cast<const Link *>(impl.find_first_unsafe(
        reinterpret_cast<StaticList_::Predicate>(pred_func)
    ));
  }

  template<typename Matches, typename Reference>
  const Link *find_first_unsafe(Matches match_func, const Reference &reference)
  const {
    return reinterpret_cast<const Link *>(impl.find_first_unsafe(
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

  const ConstIteratorUnsafe begin_unsafe() const {
    return ConstIteratorUnsafe(
      reinterpret_cast<const ConstLink *>(impl.get_head_unsafe())
    );
  }

  const ConstIteratorUnsafe end_unsafe() const {
    return ConstIteratorUnsafe(NULL);
  }

  bool is_empty() const {
    return impl.is_empty();
  }

  size_t get_count() const {
    return impl.get_count();
  }

  void link(Link &link) {
    impl.link(reinterpret_cast<Node_impl &>(link));
  }

  bool unlink(Link &link) {
    return impl.unlink(reinterpret_cast<Node_impl &>(link));
  }

  int index_of(const Link &link) const {
    return impl.index_of(reinterpret_cast<const Node_impl &>(link));
  }

  int index_of(const ConstLink &link) const {
    return impl.index_of(reinterpret_cast<const Node_impl &>(link));
  }

  template<typename Predicate>
  const Link *find_first(Predicate pred_func) {
    return reinterpret_cast<const Link *>(
      impl.find_first(reinterpret_cast<StaticList_::Predicate>(pred_func))
    );
  }

  template<typename Matches, typename Reference>
  const Link *find_first(Matches match_func, const Reference &reference) {
    return reinterpret_cast<const Link *>(
      impl.find_first(reinterpret_cast<StaticList_::Matches>(match_func),
                      reference)
    );
  }

  template<typename Predicate>
  const ConstLink *find_first(Predicate pred_func) const {
    return reinterpret_cast<const ConstLink *>(
      impl.find_first(reinterpret_cast<StaticList_::Predicate>(pred_func))
    );
  }

  template<typename Matches, typename Reference>
  const ConstLink *find_first(Matches match_func, const Reference &reference)
  const {
    return reinterpret_cast<const ConstLink *>(
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

  const ConstIterator begin() const {
    return ConstIterator(reinterpret_cast<const ConstLink *>(impl.get_head()));
  }

  const ConstIterator end() const {
    return ConstIterator(NULL);
  }
};


} // namespace r2p
#endif // __R2P__STATICLIST_HPP__


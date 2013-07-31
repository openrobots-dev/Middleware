
#include <r2p/impl/StaticList_.hpp>

namespace r2p {


size_t StaticList_::get_count_unsafe() const {

  size_t count = 0;
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    ++count;
  }
  return count;
}


void StaticList_::link_unsafe(Link &link) {

  R2P_ASSERT(link.nextp == NULL);
  R2P_ASSERT(!unlink_unsafe(link));

  link.nextp = headp;
  headp = &link;
}


bool StaticList_::unlink_unsafe(Link &link) {

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


int StaticList_::index_of_unsafe(const Link &link) const {

  int i = 0;
  for (Link *linkp = headp; linkp != NULL; ++i, linkp = linkp->nextp) {
    if (linkp == &link) {
      return i;
    }
  }
  return -1;
}


const StaticList_::Link *StaticList_::find_first_unsafe(
  Predicate pred_func) const {

  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (pred_func(linkp->itemp)) {
      return linkp;
    }
  }
  return NULL;
}


const StaticList_::Link *StaticList_::find_first_unsafe(
  Matches match_func, const void *featuresp) const {

  R2P_ASSERT(featuresp != NULL);

  for (Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (match_func(linkp->itemp, featuresp)) {
      return linkp;
    }
  }
  return NULL;
}


const StaticList_::Link *StaticList_::get_head() const {

  SysLock::acquire();
  const Link *linkp = get_head_unsafe();
  SysLock::release();
  return linkp;
}


bool StaticList_::is_empty() const {

  SysLock::acquire();
  bool empty = is_empty_unsafe();
  SysLock::release();
  return empty;
}


size_t StaticList_::get_count() const {

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


void StaticList_::link(Link &link) {

  SysLock::acquire();
  link_unsafe(link);
  SysLock::release();
}


bool StaticList_::unlink(Link &link) {

  SysLock::acquire();
  bool success = unlink_unsafe(link);
  SysLock::release();
  return success;
}


int StaticList_::index_of(const Link &link) const {

  int i = 0;
  SysLock::acquire();
  for (Link *linkp = headp; linkp != NULL; ++i, linkp = linkp->nextp) {
    SysLock::release();
    if (linkp == &link) {
      return i;
    }
    SysLock::acquire();
  }
  SysLock::release();
  return -1;
}


const StaticList_::Link *StaticList_::find_first(Predicate pred_func)
  const {

  SysLock::acquire();
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    SysLock::release();
    if (pred_func(linkp->itemp)) {
      return linkp;
    }
    SysLock::acquire();
  }
  SysLock::release();
  return NULL;
}


const StaticList_::Link *StaticList_::find_first(
  Matches match_func, const void *featuresp) const {

  R2P_ASSERT(featuresp != NULL);

  SysLock::acquire();
  for (Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    SysLock::release();
    if (match_func(linkp->itemp, featuresp)) {
      return linkp;
    }
    SysLock::acquire();
  }
  SysLock::release();
  return NULL;
}


StaticList_::StaticList_()
:
  headp(NULL)
{}


} // namespace r2p

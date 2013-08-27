
#include <r2p/impl/StaticList_.hpp>

namespace r2p {


size_t StaticList_::count_unsafe() const {

  size_t count = 0;
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    ++count;
  }
  return count;
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


int StaticList_::index_of_unsafe(const void *itemp) const {

  if (itemp == NULL) return -1;

  int i = 0;
  for (const Link *linkp = headp; linkp != NULL; ++i, linkp = linkp->nextp) {
    if (linkp->itemp == itemp) {
      return i;
    }
  }
  return -1;
}


bool StaticList_::contains_unsafe(const void *itemp) const {

  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (linkp->itemp == itemp) {
      return true;
    }
  }
  return false;
}


void *StaticList_::find_first_unsafe(Predicate pred_func) const {

  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (pred_func(linkp->itemp)) {
      return linkp->itemp;
    }
  }
  return NULL;
}


void *StaticList_::find_first_unsafe(Matches match_func,
                                     const void *featuresp) const {

  R2P_ASSERT(featuresp != NULL);

  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (match_func(linkp->itemp, featuresp)) {
      return linkp->itemp;
    }
  }
  return NULL;
}


const StaticList_::Link *StaticList_::get_head() const {

  SysLock::acquire();
  const Link *headp = get_head_unsafe();
  SysLock::release();
  return headp;
}


bool StaticList_::is_empty() const {

  SysLock::acquire();
  bool success = is_empty_unsafe();
  SysLock::release();
  return success;
}


size_t StaticList_::count() const {

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


int StaticList_::index_of(const void *itemp) const {

  if (itemp == NULL) return -1;

  int i = 0;
  SysLock::acquire();
  for (const Link *linkp = headp; linkp != NULL; ++i, linkp = linkp->nextp) {
    SysLock::release();
    if (linkp->itemp == itemp) {
      return i;
    }
    SysLock::acquire();
  }
  SysLock::release();
  return -1;
}


bool StaticList_::contains(const void *itemp) const {

  SysLock::acquire();
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    if (linkp->itemp == itemp) {
      SysLock::release();
      return true;
    }
    SysLock::release();
    SysLock::acquire();
  }
  SysLock::release();
  return false;
}


void *StaticList_::find_first(Predicate pred_func) const {

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


void *StaticList_::find_first(Matches match_func,
                              const void *featuresp) const {

  R2P_ASSERT(featuresp != NULL);

  SysLock::acquire();
  for (const Link *linkp = headp; linkp != NULL; linkp = linkp->nextp) {
    SysLock::release();
    if (match_func(linkp->itemp, featuresp)) {
      return linkp->itemp;
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

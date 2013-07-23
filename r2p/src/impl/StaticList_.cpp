
#include <r2p/impl/StaticList_.hpp>

namespace r2p {


size_t StaticList_::get_count_unsafe() const {

  size_t count = 0;
  for (const Link *ep = headp; ep != NULL; ep = ep->nextp) {
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
  for (Link *ep = headp; ep != NULL; ++i, ep = ep->nextp) {
    if (ep == &link) {
      return i;
    }
  }
  return -1;
}


const StaticList_::Link *StaticList_::find_first_unsafe(
  Predicate pred_func) const {

  for (const Link *ep = headp; ep != NULL; ep = ep->nextp) {
    if (pred_func(ep->datap)) {
      return ep;
    }
  }
  return NULL;
}


const StaticList_::Link *StaticList_::find_first_unsafe(
  Matches match_func, const void *featuresp) const {

  R2P_ASSERT(featuresp != NULL);

  for (Link *ep = headp; ep != NULL; ep = ep->nextp) {
    if (match_func(ep->datap, featuresp)) {
      return ep;
    }
  }
  return NULL;
}


const StaticList_::Link *StaticList_::get_head() const {

  SysLock::acquire();
  const Link *entryp = get_head_unsafe();
  SysLock::release();
  return entryp;
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
  for (const Link *ep = headp; ep != NULL; ep = ep->nextp) {
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
    if (pred_func(linkp->datap)) {
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
    if (match_func(linkp->datap, featuresp)) {
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

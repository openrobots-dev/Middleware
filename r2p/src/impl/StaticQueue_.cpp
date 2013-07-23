
#include <r2p/impl/StaticQueue_.hpp>

namespace r2p {


const StaticQueue_::Link *StaticQueue_::get_head_unsafe() const {

  return headp;
}


const StaticQueue_::Link *StaticQueue_::get_tail_unsafe() const {

  return tailp;
}


void StaticQueue_::post_unsafe(Link &link) {

  R2P_ASSERT((headp != NULL) == (tailp != NULL));
  R2P_ASSERT(link.nextp == NULL);

  if (tailp != NULL) {
    R2P_ASSERT(tailp->nextp == NULL);
    tailp->nextp = &link;
    tailp = &link;
  } else {
    headp = tailp = &link;
  }
}


bool StaticQueue_::peek_unsafe(const Link *&linkp) const {

  R2P_ASSERT((headp != NULL) == (tailp != NULL));

  if (headp != NULL) {
    linkp = headp;
    return true;
  }
  return false;
}


bool StaticQueue_::fetch_unsafe(Link &link) {

  R2P_ASSERT((headp != NULL) == (tailp != NULL));

  if (headp != NULL) {
    register Link *nextp = headp->nextp;
    headp->nextp = NULL;
    link = *headp;
    headp = nextp;
    if (headp == NULL) {
      tailp = NULL;
    }
    return true;
  }
  return false;
}


bool StaticQueue_::skip_unsafe() {

  R2P_ASSERT((headp != NULL) == (tailp != NULL));

  if (headp != NULL) {
    register Link *nextp = headp->nextp;
    headp->nextp = NULL;
    headp = nextp;
    if (headp == NULL) {
      tailp = NULL;
    }
    return true;
  }
  return false;
}


const StaticQueue_::Link *StaticQueue_::get_head() const {

  SysLock::acquire();
  Link *linkp = headp;
  SysLock::release();
  return linkp;
}


const StaticQueue_::Link *StaticQueue_::get_tail() const {

  SysLock::acquire();
  Link *linkp = tailp;
  SysLock::release();
  return linkp;
}


bool StaticQueue_::is_empty() const {

  SysLock::acquire();
  bool empty = is_empty_unsafe();
  SysLock::release();
  return empty;
}


void StaticQueue_::post(Link &link) {

  SysLock::acquire();
  post_unsafe(link);
  SysLock::release();
}


bool StaticQueue_::peek(const Link *&linkp) const {

  SysLock::acquire();
  bool success = peek_unsafe(linkp);
  SysLock::release();
  return success;
}


bool StaticQueue_::fetch(Link &link) {

  SysLock::acquire();
  bool success = fetch_unsafe(link);
  SysLock::release();
  return success;
}


bool StaticQueue_::skip() {

  SysLock::acquire();
  bool success = skip_unsafe();
  SysLock::release();
  return success;
}


StaticQueue_::StaticQueue_()
:
    headp(NULL),
    tailp(NULL)
{}


} // namespace r2p

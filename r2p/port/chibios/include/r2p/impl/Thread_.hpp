#pragma once

#include <r2p/common.hpp>
#include <ch.h>

namespace r2p {

class MemoryPool_;
class Time;


class Thread_ {
private:
  ::Thread impl;

public:
  enum PriorityEnum {
    READY       = NOPRIO,
    IDLE        = IDLEPRIO,
    LOWEST      = LOWPRIO,
    NORMAL      = NORMALPRIO,
    HIGHEST     = HIGHPRIO - 1,
    INTERRUPT   = HIGHPRIO
  };

  enum { OK = CH_SUCCESS };

  typedef tprio_t Priority;
  typedef ::tfunc_t Function;
  typedef msg_t Return;
  typedef void *Argument;

public:
  const char *get_name() const;

  ::Thread &get_impl();

private:
  Thread_();

public:
  static size_t compute_stack_size(size_t userlen);
  static Thread_ *create_static(void *stackp, size_t stacklen,
                                Priority priority,
                                Function threadf, void *argp,
                                const char *namep = NULL);
  static Thread_ *create_heap(void *heapp, size_t stacklen, Priority priority,
                              Function threadf, void *argp,
                              const char *namep = NULL);
  static Thread_ *create_pool(MemoryPool_ &mempool, Priority priority,
                              Function threadf, void *argp,
                              const char *namep = NULL);
  static Thread_ &self();
  static Priority get_priority();
  static void set_priority(Priority priority);
  static void yield();
  static void sleep(const Time &delay);
  static bool join(Thread_ &thread);
};


} // namespace r2p

#include <r2p/impl/MemoryPool_.hpp>
#include <r2p/Time.hpp>

namespace r2p {


inline
const char *Thread_::get_name() const {

#if CH_USE_REGISTRY
  return chRegGetThreadName(&impl);
#else
  return NULL;
#endif
}


inline
::Thread &Thread_::get_impl() {

  return impl;
}


inline
size_t Thread_::compute_stack_size(size_t userlen) {

  return THD_WA_SIZE(userlen);
}


inline
Thread_ *Thread_::create_static(void *stackp, size_t stacklen,
                                Priority priority,
                                Function threadf, void *argp,
                                const char *namep) {

  Thread_ *threadp = reinterpret_cast<Thread_ *>(
    chThdCreateStatic(stackp, stacklen, static_cast<tprio_t>(priority),
                      threadf, argp)
  );
#if CH_USE_REGISTRY
  if (threadp != NULL) {
    threadp->impl.p_name = namep;
  }
#else
  (void)namep;
#endif
  return threadp;
}


inline
Thread_ *Thread_::create_heap(void *heapp, size_t stacklen, Priority priority,
                              Function threadf, void *argp,
                              const char *namep) {

  Thread_ *threadp = reinterpret_cast<Thread_ *>(
    chThdCreateFromHeap(reinterpret_cast<MemoryHeap *>(heapp), stacklen,
                        priority, threadf, argp)
  );
#if CH_USE_REGISTRY
  if (threadp != NULL) {
    threadp->impl.p_name = namep;
  }
#else
  (void)namep;
#endif
  return threadp;
}


inline
Thread_ *Thread_::create_pool(MemoryPool_ &mempool, Priority priority,
                              Function threadf, void *argp,
                              const char *namep) {

  Thread_ *threadp = reinterpret_cast<Thread_ *>(
    chThdCreateFromMemoryPool(&mempool.get_impl(), priority, threadf, argp)
  );
#if CH_USE_REGISTRY
  if (threadp != NULL) {
    threadp->impl.p_name = namep;
  }
#else
  (void)namep;
#endif
  return threadp;
}


inline
Thread_ &Thread_::self() {

  return reinterpret_cast<Thread_ &>(*chThdSelf());
}


inline
Thread_::Priority Thread_::get_priority() {

  return chThdGetPriority();
}


inline
void Thread_::set_priority(Priority priority) {

  chThdSetPriority(priority);
}


inline
void Thread_::yield() {

  chThdYield();
}


inline
void Thread_::sleep(const Time &delay) {

  if (delay.raw > 0) {
    if (delay.raw < 1000000) {
      chThdSleepMicroseconds(delay.raw);
    } else {
      chThdSleepSeconds(delay.to_s_raw());
    }
  } else {
    chThdYield();
  }
}


inline
bool Thread_::join(Thread_ &thread) {

  return chThdWait(&thread.impl) == CH_SUCCESS;
}


} // namespace r2p

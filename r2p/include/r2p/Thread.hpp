
#ifndef __R2P__THREAD_HPP__
#define __R2P__THREAD_HPP__

#include <r2p/common.hpp>
#include <r2p/impl/Thread_.hpp>
#include <r2p/MemoryPool.hpp>

namespace r2p {

class Time;


class Thread : private Uncopyable {
public:
  enum PriorityEnum {
    READY       = Thread_::READY,
    IDLE        = Thread_::IDLE,
    LOWEST      = Thread_::LOWEST,
    NORMAL      = Thread_::NORMAL,
    HIGHEST     = Thread_::HIGHEST,
    INTERRUPT   = Thread_::INTERRUPT
  };

  typedef Thread_::Priority Priority;
  typedef Thread_::Function Function;
  typedef Thread_::ReturnType ReturnType;
  typedef Thread_::ArgumentType ArgumentType;

private:
  Thread_ impl;

public:

  const char *get_name() const;

private:
  Thread();

public:
  static size_t compute_stack_size(size_t userlen);
  static Thread *create_static(void *stackp, size_t stacklen,
                               Priority priority,
                               Function threadf, void *argp,
                               const char *namep = NULL);
  static Thread *create_heap(void *heapp, size_t stacklen,
                             Priority priority,
                             Function threadf, void *argp,
                             const char *namep = NULL);

  template<typename T>
  static Thread *create_pool(MemoryPool<T> &mempool,
                             Priority priority,
                             Function threadf, void *argp,
                             const char *namep = NULL);

  static Thread &self();
  static Priority get_priority();
  static void set_priority(Priority priority);
  static void yield();
  static void sleep(const Time &delay);
  static bool join(Thread &thread);
};


inline
const char *Thread::get_name() const {

  return impl.get_name();
}


inline
size_t compute_stack_size(size_t userlen) {

  return Thread_::compute_stack_size(userlen);
}


inline
Thread *Thread::create_static(void *stackp, size_t stacklen,
                              Priority priority,
                              Function threadf, void *argp,
                              const char *namep) {

  return reinterpret_cast<Thread *>(
    Thread_::create_static(stackp, stacklen, priority, threadf, argp,
                               namep)
  );
}


inline
Thread *Thread::create_heap(void *heapp, size_t stacklen,
                            Priority priority,
                            Function threadf, void *argp, const char *namep) {

  return reinterpret_cast<Thread *>(
    Thread_::create_heap(heapp, stacklen, priority, threadf, argp, namep)
  );
}


template<typename T> inline
Thread *Thread::create_pool(MemoryPool<T> &mempool,
                            Priority priority,
                            Function threadf, void *argp, const char *namep) {

  return reinterpret_cast<Thread *>(
    Thread_::create_pool(mempool, priority, threadf, argp, namep)
  );
}


inline
Thread &Thread::self() {

  return reinterpret_cast<Thread &>(Thread_::self());
}


inline
Thread::Priority Thread::get_priority() {

  return Thread_::get_priority();
}


inline
void Thread::set_priority(Priority priority) {

  Thread_::set_priority(priority);
}


inline
void Thread::yield() {

  Thread_::yield();
}


inline
void Thread::sleep(const Time &delay) {

  Thread_::sleep(delay);
}


inline
bool Thread::join(Thread &thread) {

  return Thread_::join(thread.impl);
}


} // namespace r2p
#endif // __R2P__THREAD_HPP__

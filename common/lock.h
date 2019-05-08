/*!
 * \file lock.h
 * \brief The read/write lock
 */
#ifndef COMMON_LOCK_H_
#define COMMON_LOCK_H_

#include <assert.h>
#include <pthread.h>

namespace common {

class ReadWriteLock {
 private:
  ReadWriteLock(const ReadWriteLock&);
  ReadWriteLock& operator = (const ReadWriteLock&);
 
 public:
  enum Mode {
    PREFER_READER,
    PREFER_WRITER
  };

  ReadWriteLock(Mode mode = PREFER_READER) {
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    switch (mode) {
      case PREFER_WRITER:
        pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
        break;
      case PREFER_READER:
        pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_READER_NP);
        break;
      default:
        break;
    }
    pthread_rwlock_init(&_lock, &attr);
  }

  ~ReadWriteLock() {
    pthread_rwlock_destroy(&_lock);
  }

  int rdlock() {
    return pthread_rwlock_rdlock(&_lock);
  }

  int wrlock() {
    return pthread_rwlock_wrlock(&_lock);
  }

  int tryrdlock() {
    return pthread_rwlock_tryrdlock(&_lock);
  }

  int trywrlock() {
    return pthread_rwlock_trywrlock(&_lock);
  }

  int unlock() {
    return pthread_rwlock_unlock(&_lock);
  }

 protected:
  pthread_rwlock_t _lock;
};

class ScopedReadLock {
 private:
  ReadWriteLock &_lock;

  ScopedReadLock(const ScopedReadLock&);
  ScopedReadLock& operator = (const ScopedReadLock&);

 public:
  explicit ScopedReadLock(ReadWriteLock &lock)
      : _lock(lock) {
    int ret = _lock.rdlock();
    assert(ret == 0); (void) ret;
  }

  ~ScopedReadLock() {
    int ret = _lock.unlock();
    assert(ret == 0); (void) ret;
  }
};

class ScopedWriteLock {
 private:
  ReadWriteLock &_lock;

  ScopedWriteLock(const ScopedWriteLock&);
  ScopedWriteLock& operator = (const ScopedWriteLock&);

 public:
  explicit ScopedWriteLock(ReadWriteLock &lock)
      : _lock(lock) {
    int ret = _lock.wrlock();
    assert(ret == 0); (void) ret;
  }

  ~ScopedWriteLock() {
    int ret = _lock.unlock();
    assert(ret == 0); (void) ret;
  }
};

class ScopedReadWriteLock {
 private:
  ReadWriteLock& _lock;
  char _mode;

 private:
  ScopedReadWriteLock(const ScopedReadWriteLock&);
  ScopedReadWriteLock& operator = (const ScopedReadWriteLock&);

 public:
  explicit ScopedReadWriteLock(ReadWriteLock& lock, const char mode)
      : _lock(lock), _mode(mode) {
    if (_mode == 'r' || _mode == 'R') {
      int ret = _lock.rdlock();
      assert(ret == 0); (void) ret;
    } else if (_mode == 'w' || _mode == 'W') {
      int ret = _lock.wrlock();
      assert(ret == 0); (void) ret;
    }
  }

  ~ScopedReadWriteLock() {
    if (_mode == 'r' || _mode == 'R'
        || _mode == 'w' || _mode == 'W') {
      int ret = _lock.unlock();
      assert(ret == 0); (void) ret;
    }
  }
};

}  // namespace common

#endif  // COMMON_LOCK_H_

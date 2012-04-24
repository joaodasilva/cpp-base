#include "base/event_loop.h"

#include <unordered_map>
#include <utility>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>

#include "base/lock.h"
#include "base/logging.h"

namespace {

// TODO: Redo this using thread_local once clang++ supports it.
pthread_once_t current_key_once = PTHREAD_ONCE_INIT;
pthread_key_t current_key;

void create_current_key() {
  int ret;
  if ((ret = pthread_key_create(&current_key, NULL)) != 0)
    DLOG(FATAL) << "pthread_key_create failed: " << ret;
}

bool current_key_is_valid() {
  int ret;
  if ((ret = pthread_once(&current_key_once, create_current_key)) != 0) {
    DLOG(FATAL) << "pthread_once failed: " << ret;
    return false;
  }
  return true;
}

}  // namespace

struct EventLoop::Task {
  Task(Callback&& f) : callback(std::forward<Callback>(f)) {}

  Callback callback;
};

struct EventLoop::PollTask {
  PollTask(PollCallback&& f, int fd, int events)
      : callback(std::forward<PollCallback>(f)), fd(fd), events(events) {}

  PollCallback callback;
  int fd;
  int events;
};

struct EventLoop::DelayedTask {
  DelayedTask(Task* task, Time when)
      : task(task), when(when) {}

  Task* task;
  Time when;
  bool operator<(const DelayedTask& t) const { return when > t.when; }
};

EventLoop::EventLoop()
    : quit_soon_(false) {
#ifndef NDEBUG
  running_ = false;
#endif
}

EventLoop::~EventLoop() {
  close(pipe_read_);
  close(pipe_write_);
  if (!pending_.empty())
    DLOG(ERROR) << "Deleting EventLoop with pending_ tasks";
  for (auto i: pending_) delete i;
  while (!pending_delayed_.empty()) {
    delete pending_delayed_.top().task;
    pending_delayed_.pop();
  }
  if (!pending_poll_.empty())
    DLOG(ERROR) << "Deleting EventLoop with pending_poll_ tasks";
  for (auto i: pending_poll_) delete i;
}

// static
unique_ptr<EventLoop> EventLoop::Create() {
  int fds[2];
  if (pipe(fds) != 0) {
    DLOGE(ERROR) << "pipe failed";
    return NULL;
  }
  if (fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL, 0) | O_NONBLOCK) == -1) {
    DLOGE(ERROR) << "fcntl failed";
    close(fds[0]);
    close(fds[1]);
    return NULL;
  }

  unique_ptr<EventLoop> loop(new EventLoop);
  loop->pipe_read_ = fds[0];
  loop->pipe_write_ = fds[1];
  return loop;
}

// static
EventLoop* EventLoop::Current() {
  if (!current_key_is_valid())
    return NULL;
  return (EventLoop*) pthread_getspecific(current_key);
}

void EventLoop::Post(Callback&& f) {
  Task* task = new Task(std::forward<Callback>(f));
  bool ping_pipe = false;
  {
    ScopedLock lock(pending_lock_);
    ping_pipe = pending_.empty();
    pending_.push_back(task);
  }
  if (ping_pipe)
    PingPipe();
}

void EventLoop::PostAfter(Callback&& f, const TimeDelta& delay) {
  Task* task = new Task(std::forward<Callback>(f));
  Time when = Now() + delay;
  bool ping_pipe = false;
  {
    ScopedLock lock(pending_lock_);
    ping_pipe = pending_delayed_.empty() ||
                pending_delayed_.top().when > when;
    pending_delayed_.push(DelayedTask(task, when));
  }
  if (ping_pipe)
    PingPipe();
}

void EventLoop::PostWhenReadReady(int fd, PollCallback&& f) {
  PollTask* task = new PollTask(std::forward<PollCallback>(f), fd, POLLIN);
  {
    ScopedLock lock(pending_lock_);
    pending_poll_.push_back(task);
  }
  PingPipe();
}

void EventLoop::PostWhenWriteReady(int fd, PollCallback&& f) {
  PollTask* task = new PollTask(std::forward<PollCallback>(f), fd, POLLOUT);
  {
    ScopedLock lock(pending_lock_);
    pending_poll_.push_back(task);
  }
  PingPipe();
}

void EventLoop::EventLoop::CancelDescriptor(int fd) {
  static PollCallback kEmptyFunction;
  {
    ScopedLock lock(pending_lock_);
    pending_poll_.push_back(
        new PollTask(std::forward<PollCallback>(kEmptyFunction), fd, 0));
  }
  PingPipe();
}

void EventLoop::Run() {
  std::vector<Task*> pending;
  std::vector<PollTask*> pending_poll;
  std::unordered_map<int, PollTask*> fd_to_poll_task;
  uint8 buffer[1024];
  std::vector<pollfd> poll_array(1);
  int ret;

#ifndef NDEBUG
  DCHECK(!running_);
  running_ = true;
#endif

  DCHECK(!Current());
  if (!SetCurrent(this))
    return;

  poll_array[0].fd = pipe_read_;
  poll_array[0].events = POLLIN;
  poll_array[0].revents = 0;

  for (;;) {
    bool did_work = false;
    int milliseconds_to_next_delayed = -1;

    // This loop executes all work immediately available.
    do {
      // Flush the pipe.
      do {
        ret = read(pipe_read_, buffer, arraysize(buffer));
      } while (ret > 0);
      if (ret == 0 || (ret == -1 && errno != EAGAIN)) {
        DLOG(FATAL) << "reading from pipe_read_ failed, errno: " << errno;
        return;
      }
      // If more work arrives now, |pipe_read_| will become readable and make
      // poll() return immediately.

      Time now = Now();
      milliseconds_to_next_delayed = -1;
      {
        ScopedLock lock(pending_lock_);
        pending.swap(pending_);
        pending_poll.swap(pending_poll_);

        while (!pending_delayed_.empty() &&
               pending_delayed_.top().when <= now) {
          pending.push_back(pending_delayed_.top().task);
          pending_delayed_.pop();
        }

        if (!pending_delayed_.empty()) {
          milliseconds_to_next_delayed =
              ToTimeDelta(pending_delayed_.top().when - now).count();
        }
      }

      for (PollTask* t: pending_poll) {
        if (t->events) {
          DCHECK(fd_to_poll_task.find(t->fd) == fd_to_poll_task.end());
          fd_to_poll_task[t->fd] = t;
          poll_array.push_back(pollfd());
          poll_array.back().fd = t->fd;
          poll_array.back().events = t->events;
          poll_array.back().revents = 0;
        } else {
          for (size_t i = 1; i < poll_array.size(); ++i) {
            if (poll_array[i].fd == t->fd) {
              poll_array[i] = poll_array.back();
              poll_array.pop_back();
              break;
            }
          }
          if (fd_to_poll_task.find(t->fd) != fd_to_poll_task.end()) {
            delete fd_to_poll_task[t->fd];
            fd_to_poll_task.erase(t->fd);
          }
          delete t;
        }
      }
      pending_poll.clear();

      did_work = !pending.empty();
      DLOG(DEBUG) << "doing work, #tasks: " << pending.size();
      for (Task* t: pending)
        HandleAndDelete(t);
      pending.clear();
    } while (did_work);

    if (quit_soon_)
      break;

    // Didn't do any work in the last iteration; poll for more.
    DLOG(DEBUG) << "polling for " << milliseconds_to_next_delayed << "ms with "
                << poll_array.size() << " fds...";
    if (poll(&poll_array.front(),
             poll_array.size(),
             milliseconds_to_next_delayed) == -1 && errno != EAGAIN) {
      DLOGE(FATAL) << "poll failed";
      return;
    }
    DLOG(DEBUG) << "poll woke up";

    for (size_t i = 1; i < poll_array.size(); ) {
      pollfd& pfd = poll_array[i];
      if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL | pfd.events)) {
        DLOG(VERBOSE) << "fd ready: " << pfd.fd << ", revents: " << pfd.revents;
        DCHECK(fd_to_poll_task.find(pfd.fd) != fd_to_poll_task.end());
        HandleAndDeletePolled(fd_to_poll_task[pfd.fd], pfd.revents);
        fd_to_poll_task.erase(pfd.fd);
        poll_array[i] = poll_array.back();
        poll_array.pop_back();
      } else {
        ++i;
      }
    }
  }

  if (!fd_to_poll_task.empty()) {
    DLOG(WARNING) << "EventLoop still has PollTasks, moving to pending_poll_";
    {
      ScopedLock lock(pending_lock_);
      for (auto i: fd_to_poll_task)
        pending_poll_.push_back(i.second);
    }
  }

  SetCurrent(NULL);
  quit_soon_ = false;

#ifndef NDEBUG
  running_ = false;
#endif
}

void EventLoop::QuitSoon() {
  quit_soon_ = true;
  PingPipe();
}

// static
bool EventLoop::SetCurrent(EventLoop* loop) {
  if (!current_key_is_valid())
    return false;
  int ret;
  if ((ret = pthread_setspecific(current_key, loop)) != 0) {
    DLOG(FATAL) << "pthread_set_specific failed: " << ret;
    return false;
  }
  return true;
}

void EventLoop::PingPipe() {
  static uint8 byte = 0;
  // This can block if the pipe buffer is full, which also means the loop is
  // taking too long on some iteration.
  if (write(pipe_write_, &byte, 1) != 1)
    DLOGE(FATAL) << "writing to pipe_write_ failed";
}

void EventLoop::HandleAndDelete(Task* task) {
  task->callback();
  delete task;
}

void EventLoop::HandleAndDeletePolled(PollTask* task, int revents) {
  task->callback(revents & POLLNVAL, revents & POLLHUP, revents & POLLERR);
  delete task;
}

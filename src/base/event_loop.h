#ifndef BASE_EVENT_LOOP_H
#define BASE_EVENT_LOOP_H

#include <functional>
#include <queue>
#include <vector>

#include "base/base.h"
#include "base/bind.h"
#include "base/memory.h"
#include "base/time.h"

class BaseTest;

class EventLoop {
 public:
  typedef std::function<void()> Callback;
  typedef std::function<void(bool nval, bool hup, bool err)> PollCallback;

  ~EventLoop();

  static unique_ptr<EventLoop> Create();

  static EventLoop* Current();
  bool IsCurrent() const { return Current() == this; }

  void Post(Callback&& f);
  void PostAfter(Callback&& f, const TimeDelta& delay);
  void PostWhenReadReady(int fd, PollCallback&& f);
  void PostWhenWriteReady(int fd, PollCallback&& f);

  // TODO: Bind() can't bind functors, but std::bind() can. This is because
  // CallableTraits<> can't take a struct with operator().
  template<typename T>
  void DeleteSoon(T* ptr) {
    Post(std::bind(std::default_delete<T>(), ptr));
  }

  // Cancels a task that is waiting for |fd|, if any. Such a task can still be
  // invoked after |CancelDescriptor| returns; if the |fd| can be closed in
  // another thread, the task should be protected with a WeakFlag.
  void CancelDescriptor(int fd);

  // Keeps running the loop until QuitSoon is invoked.
  void Run();

  // The loop will quit once all immediately ready tasks have been processed.
  // It will keep any delayed tasks and PollTasks in their queues, which can be
  // resumed by calling Run() again.
  void QuitSoon();

 private:
  friend class BaseTest;

  struct DelayedTask;
  struct PollTask;
  struct Task;

  EventLoop();

  static bool SetCurrent(EventLoop* loop);
  void PingPipe();
  void HandleAndDelete(Task* task);
  void HandleAndDeletePolled(PollTask* task, int revents);

  // These are protected by |pending_lock_|.
  std::vector<Task*> pending_;
  std::priority_queue<DelayedTask> pending_delayed_;
  std::vector<PollTask*> pending_poll_;

  Lock pending_lock_;
  int pipe_read_;
  int pipe_write_;

  volatile bool quit_soon_;
#ifndef NDEBUG
  bool running_;
#endif

  DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

#endif  // BASE_EVENT_LOOP_H

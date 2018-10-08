#include "thread_pool.h"

void thread_pool::worker_thread(const size_t worker_id)
{
  while(_active)
  {
    task_t task;
    {
      std::unique_lock<std::mutex> task_lock{_task_mutex};
      if(!_task_cv.wait_for(task_lock, _cv_wait_time, [this] { return _task_is_available; }))
      {
        if(_active)
          continue;
        else
          break;
      }
      task = std::move(_tasks.top());
      _tasks.pop();
      _task_is_available = !_tasks.empty();
    }
    _workers_idle_status[worker_id] = false;
    task(worker_id);
    _workers_idle_status[worker_id] = true;
  }
}

bool thread_pool::all_workers_idle() const
{
  bool result = true;
  for(size_t i = 0; i < _workers.size(); ++i)
    result = result && _workers_idle_status[i];
  return result;
}

void thread_pool::await_tasks_completion()
{
  for(;;)
  {
    std::this_thread::sleep_for(_cv_wait_time);
    if(idle())
      return;
  }
}

thread_pool::thread_pool(size_t workers_count, std::chrono::duration<long long> cv_wait_time)
  : _cv_wait_time{cv_wait_time}, _workers_idle_status(std::make_unique<std::atomic_bool[]>(workers_count))
{
  for(size_t i = 0; i < workers_count; ++i)
  {
    _workers_idle_status[i] = true;
    _workers.emplace_back(&thread_pool::worker_thread, this, i);
  }
}

void thread_pool::submit(thread_pool::task_t task)
{
  std::lock_guard<std::mutex> task_lock{_task_mutex};
  _tasks.emplace(std::move(task));
  _task_is_available = true;
  _task_cv.notify_one();
}

void thread_pool::submit(std::vector<task_t> tasks)
{
  const size_t                notify_count = std::min(_workers.size(), tasks.size());
  std::lock_guard<std::mutex> task_lock{_task_mutex};
  for(size_t i = 0; i < tasks.size(); ++i)
    _tasks.emplace(std::move(tasks[i]));
  _task_is_available = true;

  for(size_t i = 0; i < notify_count; ++i)
    _task_cv.notify_one();
}

bool thread_pool::idle() const { return !has_pending_tasks() && all_workers_idle(); }

thread_pool::~thread_pool()
{
  _active            = false;
  _task_is_available = false;
  _task_cv.notify_all();
  for(auto & w : _workers)
    if(w.joinable())
      w.join();
}

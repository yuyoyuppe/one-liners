#include <iostream>
#include <future>
#include <thread>
#include <vector>
#include <chrono>
#include <stack>
#include <cassert>

class thread_pool final
{
  using task_t = std::function<void(void)>;

  const std::chrono::duration<long long> _cv_wait_time;

  std::vector<std::thread> _workers;
  std::stack<task_t>       _tasks;
  bool                     _task_is_available = false;
  bool                     _active            = true;
  std::mutex               _task_mutex;
  std::condition_variable  _task_cv;

  void worker_thread()
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
        task = _tasks.top();
        _tasks.pop();
        _task_is_available = _tasks.size();
      }
      task();
    }
  }

public:
  void await_tasks_completion()
  {
    for(;;)
    {
      std::this_thread::sleep_for(_cv_wait_time);
      std::unique_lock<std::mutex> task_lock{_task_mutex};
      if(_tasks.empty())
        return;
    }
  }

  thread_pool(size_t                           workers_count = std::thread::hardware_concurrency(),
              std::chrono::duration<long long> cv_wait_time  = std::chrono::seconds{1})
    : _cv_wait_time{cv_wait_time}
  {
    assert(workers_count <= 1024);
    for(size_t i = 0; i < workers_count; ++i)
      _workers.emplace_back(&thread_pool::worker_thread, this);
  }

  void submit(task_t task)
  {
    std::lock_guard<std::mutex> task_lock{_task_mutex};
    _tasks.emplace(std::move(task));
    _task_is_available = true;
    _task_cv.notify_one();
  }

  ~thread_pool()
  {
    _active            = false;
    _task_is_available = false;
    _task_cv.notify_all();
    for(auto & w : _workers)
      if(w.joinable())
        w.join();
  }
};


int main()
{
  size_t tasks = 1e6;

  thread_pool p;
  while(tasks-- != 0)
  {
    using namespace std::chrono_literals;
    p.submit([]() {
      std::this_thread::sleep_for(10ms);
      std::cout << "this is " << std::this_thread::get_id() << " speaking!\n";
    });
    std::this_thread::sleep_for(100ms);
  }
  p.await_tasks_completion();
  return 0;
}
#pragma once

#include <chrono>
#include <future>
#include <thread>
#include <stack>
#include <vector>

class thread_pool final
{
public:
    using task_t = std::function<void(void)>;
private:

    const std::chrono::duration<long long> _cv_wait_time;

    std::vector<std::thread> _workers;
    std::unique_ptr<std::atomic_bool[]> _workers_idle_status;
    std::stack<task_t>       _tasks;
    bool                     _task_is_available = false;
    bool                     _active = true;
    std::mutex               _task_mutex;
    std::condition_variable  _task_cv;

    void worker_thread(const size_t worker_id);
    bool all_workers_idle() const;
    inline bool has_pending_tasks() const { return !_tasks.empty(); }
public:
    ~thread_pool();

    void await_tasks_completion();

    thread_pool(size_t workers_count = std::thread::hardware_concurrency(),
        std::chrono::duration<long long> cv_wait_time = std::chrono::seconds{ 1 });

    void submit(task_t task);
    void submit(std::vector<task_t> tasks);

    bool idle() const;
};

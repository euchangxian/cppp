#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

const unsigned int MAX_THREADS = std::thread::hardware_concurrency();

/**
 * ThreadPool allows reuse of threads
 * Exposes APIs like submit.
 */
class ThreadPool {
 private:
  // shoudStop is a flag indicating whether the Pool should still accept work
  std::atomic<bool> shouldStop;

  // workers store the worker threads.
  std::vector<std::thread> workers;

  // runqueue stores the tasks that are submitted and to be done.
  std::queue<std::function<void()>> runqueue;

  // queueLock synchronizes access to the queue.
  std::mutex queueLock;

  // hasTask indicates whether the queue has tasks to run. Avoids spinning
  std::condition_variable hasTask;

  void work() {
    while (true) {
      std::function<void()> task;

      // Scope the critical section
      {
        // Acquire the queue lock
        std::unique_lock<std::mutex> lock(this->queueLock);

        // Check if there are tasks to run
        this->hasTask.wait(lock, [this] {
          return this->shouldStop || !this->runqueue.empty();
        });

        // After waking, check whether this Thread should stop
        if (this->shouldStop && this->runqueue.empty()) {
          return;
        }

        if (!this->runqueue.empty()) {
          // Move the task in the queue
          task = std::move(this->runqueue.front());
          this->runqueue.pop();
        }
      }

      if (task) {
        // Add error handling to prevent a faulty task from causing the OS to
        // kill the thread
        try {
          task();
        } catch (const std::exception& e) {
          std::cerr << "exception in thread " << std::this_thread::get_id()
                    << ": " << e.what() << '\n';
        } catch (...) {
          // Catch all other exceptions
          std::cerr << "unknown exception in thread "
                    << std::this_thread::get_id() << '\n';
        }
      }
    }
  }

 public:
  // Create a ThreadPool instance with worker threads that will invoke
  // ThreadPool::work on the current instance of the ThreadPool
  ThreadPool(size_t concurrency) : shouldStop(false) {
    workers.reserve(concurrency);
    for (size_t i = 0; i < concurrency; ++i) {
      workers.emplace_back(&ThreadPool::work, this);
    }
  }

  ~ThreadPool() {
    this->shouldStop = true;

    // Wake all sleeping threads
    hasTask.notify_all();

    // Wait for all running threads.
    for (std::thread& worker : workers) {
      worker.join();
    }
  }

  // && doesnt mean the function takes in an rvalue reference. It is a
  // universal reference which can bind to both lvalues and rvalues, depending
  // on the template parameter.
  template <class FunctionType>
  void submitTask(FunctionType&& f) {
    {
      std::unique_lock<std::mutex> lock(this->queueLock);
      // std::forward preserves the value category of the original argument
      this->runqueue.emplace(std::forward<FunctionType>(f));
    }
    // wake a sleeping thread
    hasTask.notify_one();
  }

  void waitForAll() {
    this->shouldStop = true;

    hasTask.notify_all();
    for (std::thread& worker : workers) {
      worker.join();
    }
  }
};

int main() {
  {
    ThreadPool pool(8);
    std::mutex coutMutex;

    for (int i = 1; i <= 100; ++i) {
      // Capture i by value
      pool.submitTask([i, &coutMutex] {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "ThreadID: " << std::this_thread::get_id() << ", i = " << i
                  << '\n';
      });
    }
    // pool.waitForAll();
    // std::cout << "ThreadPool::waitForAll completed\n";

  }  // Scope of ThreadPool ends here
  return 0;
}

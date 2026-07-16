#include <onex/util/thread_pool.h>

#include <atomic>
#include <thread>
#include <vector>

namespace onex::util {

  ThreadPool::ThreadPool(size_t num_threads) {
    if (num_threads == 0) {
      num_threads = std::thread::hardware_concurrency();
    }
    // At least one worker even when hardware_concurrency returns 0.
    num_threads_ = (num_threads > 0) ? num_threads : 1;
  }

  size_t ThreadPool::parallel_for(size_t count, const std::function<bool(size_t)>& fn) {
    if (count == 0 || num_threads_ == 0) {
      return 0;
    }

    const size_t n_workers = std::min(count, num_threads_);
    std::atomic<size_t> next{0};
    std::atomic<size_t> errors{0};

    auto worker = [&]() {
      while (true) {
        const auto idx = next.fetch_add(1, std::memory_order_relaxed);
        if (idx >= count) {
          break;
        }
        if (!fn(idx)) {
          errors.fetch_add(1, std::memory_order_relaxed);
        }
      }
    };

    std::vector<std::thread> workers;
    workers.reserve(n_workers);
    for (size_t i = 0; i < n_workers; ++i) {
      workers.emplace_back(worker);
    }
    for (auto& w : workers) {
      w.join();
    }

    return errors.load();
  }

}  // namespace onex::util

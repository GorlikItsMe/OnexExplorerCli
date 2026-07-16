#include <onex/util/thread_pool.h>

#include <atomic>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

namespace onex::util {

  ThreadPool::ThreadPool(size_t num_threads) : num_threads_(0) {
    if (num_threads == 0) {
      num_threads = std::thread::hardware_concurrency();
    }
    num_threads_ = (num_threads > 0) ? num_threads : 1;
  }

  std::vector<size_t> ThreadPool::parallel_for(size_t count,
                                               const std::function<bool(size_t)>& fn) {
    std::vector<size_t> failed;
    if (count == 0) {
      return failed;
    }

    const size_t n_workers = std::min(count, num_threads_);
    std::atomic<size_t> next{0};
    std::mutex error_mutex;

    auto worker = [&]() {
      while (true) {
        const auto idx = next.fetch_add(1, std::memory_order_relaxed);
        if (idx >= count) {
          break;
        }

        bool ok = false;
        try {
          ok = fn(idx);
        } catch (...) {
          ok = false;
        }

        if (!ok) {
          std::lock_guard lk(error_mutex);
          failed.push_back(idx);
        }
      }
    };

    std::vector<std::thread> workers;
    workers.reserve(n_workers);

    // RAII scope guard: if emplace_back throws, join any threads already
    // created so we don't hit std::terminate in ~thread.
    struct JoinGuard {
      std::vector<std::thread>& ws;
      ~JoinGuard() noexcept {
        for (auto& w : ws) {
          if (w.joinable()) {
            w.join();
          }
        }
      }
    } guard{workers};

    for (size_t i = 0; i < n_workers; ++i) {
      workers.emplace_back(worker);
    }

    // All threads created successfully — disarm the guard.
    for (auto& w : workers) {
      w.join();
    }

    return failed;
  }

}  // namespace onex::util

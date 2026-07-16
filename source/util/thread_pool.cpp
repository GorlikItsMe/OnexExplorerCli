#include <onex/util/thread_pool.h>

#include <atomic>
#include <exception>
#include <thread>
#include <vector>

namespace onex::util {

  ThreadPool::ThreadPool(size_t num_threads) : num_threads_(0) {
    if (num_threads == 0) {
      num_threads = std::thread::hardware_concurrency();
    }
    if (num_threads == 0) {
      num_threads = 1;
    }
    num_threads_ = num_threads;
  }

  size_t ThreadPool::parallel_for(size_t count, const std::function<bool(size_t)>& fn) {
    if (count == 0) {
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
        // If the callback throws, treat it as an error rather than letting
        // the exception escape into std::thread which would call terminate().
        try {
          if (!fn(idx)) {
            errors.fetch_add(1, std::memory_order_relaxed);
          }
        } catch (...) {
          errors.fetch_add(1, std::memory_order_relaxed);
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
    // workers is empty after join (moved-from), so the guard joins nothing.

    return errors.load();
  }

}  // namespace onex::util

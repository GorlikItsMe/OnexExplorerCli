#pragma once

#include <functional>

namespace onex::util {

  /// A simple thread pool that distributes independent work items across a fixed
  /// set of worker threads.  Each call to @c parallel_for creates threads,
  /// distributes work via an atomic counter, waits for completion, then joins.
  ///
  /// Thread safety: multiple parallel_for calls from the same pool are
  /// serialised (external synchronisation inside the method).  The pool object
  /// itself is not safe for concurrent access from multiple caller threads.
  class ThreadPool {
  public:
    /// Construct a pool with @p num_threads worker threads.
    /// If @p num_threads == 0, uses std::thread::hardware_concurrency().
    explicit ThreadPool(size_t num_threads = 0);

    /// Defaulted destructor — threads are joined inside parallel_for.
    ~ThreadPool() = default;

    // Not copyable or movable — ownership semantics are unclear.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /// Invoke @p fn(i) for i in [0, count) in parallel.
    ///
    /// @p fn must be callable as @c bool(size_t) and return @c true on success,
    /// @c false on error.  The return value is the total number of calls that
    /// returned @c false.
    ///
    /// @note This method blocks until all work is done and threads are joined.
    [[nodiscard]] size_t parallel_for(size_t count, const std::function<bool(size_t)>& fn);

    /// Number of worker threads in this pool.
    [[nodiscard]] size_t num_threads() const noexcept { return num_threads_; }

  private:
    size_t num_threads_;
  };

}  // namespace onex::util

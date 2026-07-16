#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace onex::util {

  /// A simple thread pool that distributes independent work items across a fixed
  /// set of worker threads.  Each call to @c parallel_for creates threads,
  /// distributes work via an atomic counter, waits for completion, then joins.
  ///
  /// @warning This pool is @b not safe for concurrent calls from multiple
  /// threads — parallel_for serialises internally only for exception safety
  /// (joining already-spawned threads if creation fails), but concurrent
  /// invocations from different caller threads would oversubscribe cores and
  /// produce undefined behaviour for the shared @c fn arguments.
  class ThreadPool {
  public:
    /// Construct a pool with @p num_threads worker threads.
    /// If @p num_threads == 0, uses std::thread::hardware_concurrency().
    explicit ThreadPool(size_t num_threads = 0);

    ~ThreadPool() = default;

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /// Invoke @p fn(i) for i in [0, count) in parallel.
    ///
    /// @p fn must be callable as @c bool(size_t) and return @c true on success,
    /// @c false on error.  Exceptions thrown by @p fn are caught and returned as
    /// errors — they never propagate out of @c parallel_for.
    ///
    /// @return A vector of indices for which @p fn returned @c false or threw.
    ///         Empty vector means all calls succeeded.
    ///
    /// @note This method blocks until all work is done and threads are joined.
    /// If thread creation fails partway through, already-spawned threads are
    /// joined before the exception propagates (no @c std::terminate).
    [[nodiscard]] std::vector<size_t> parallel_for(size_t count,
                                                   const std::function<bool(size_t)>& fn);

    /// Number of worker threads in this pool.
    [[nodiscard]] size_t num_threads() const noexcept { return num_threads_; }

  private:
    size_t num_threads_;
  };

}  // namespace onex::util

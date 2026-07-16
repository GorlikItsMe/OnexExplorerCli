#pragma once

#include <onex/core/error.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace onex::downloader {

  /// Signature: (dltotal, dlnow, ultotal, ulnow) — called during transfer.
  using ProgressCallback
      = std::function<void(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow)>;

  class HttpClient;

  /// Factory for creating HttpClient instances (enables DI in tests).
  using HttpClientFactory = std::function<std::unique_ptr<HttpClient>()>;

  /// Result of an HTTP request.
  struct HttpResponse {
    int status_code = 0;     ///< HTTP status code (e.g. 200, 404).
    std::vector<byte> body;  ///< Response body bytes.
  };

  /// Abstract HTTP client interface.
  ///
  /// Allows swapping the real cURL implementation with a fake client in tests.
  class HttpClient {
  public:
    virtual ~HttpClient() = default;

    /// Perform a GET request and return the response.
    virtual auto get(const std::string& url) -> HttpResponse = 0;

    /// Download a URL directly to a file on disk.
    virtual auto download(const std::string& url, const std::string& path) -> HttpResponse = 0;

    /// Register a progress callback for the next transfer(s).
    virtual void set_progress_callback(ProgressCallback cb) = 0;
  };

  /// Real HTTP client backed by libcurl.
  class CurlHttpClient : public HttpClient {
  public:
    CurlHttpClient();
    ~CurlHttpClient() override;
    auto get(const std::string& url) -> HttpResponse override;
    auto download(const std::string& url, const std::string& path) -> HttpResponse override;
    void set_progress_callback(ProgressCallback cb) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

}  // namespace onex::downloader

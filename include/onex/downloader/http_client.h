#pragma once

#include <onex/core/error.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace onex::downloader {

  /// Signature: (dltotal, dlnow, ultotal, ulnow) — called during transfer.
  using ProgressCallback = std::function<void(int64_t dltotal, int64_t dlnow,
                                              int64_t ultotal, int64_t ulnow)>;

  struct HttpResponse {
    int status_code = 0;
    std::vector<byte> body;
  };

  class HttpClient {
  public:
    virtual ~HttpClient() = default;
    virtual auto get(const std::string& url) -> HttpResponse = 0;
    virtual auto download(const std::string& url, const std::string& path) -> HttpResponse = 0;
    virtual void set_progress_callback(ProgressCallback cb) = 0;
  };

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

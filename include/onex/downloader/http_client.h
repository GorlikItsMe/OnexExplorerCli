#pragma once

#include <onex/core/error.h>

#include <memory>
#include <string>
#include <vector>

namespace onex::downloader {

  struct HttpResponse {
    int status_code = 0;
    std::vector<byte> body;
  };

  class HttpClient {
  public:
    virtual ~HttpClient() = default;
    virtual auto get(const std::string& url) -> HttpResponse = 0;
    virtual auto download(const std::string& url, const std::string& path) -> HttpResponse = 0;
    virtual auto clone() -> std::unique_ptr<HttpClient> = 0;
  };

  class CurlHttpClient : public HttpClient {
  public:
    CurlHttpClient();
    ~CurlHttpClient() override;
    auto get(const std::string& url) -> HttpResponse override;
    auto download(const std::string& url, const std::string& path) -> HttpResponse override;
    auto clone() -> std::unique_ptr<HttpClient> override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

}  // namespace onex::downloader

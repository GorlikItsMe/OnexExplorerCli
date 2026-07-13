#include <curl/curl.h>
#include <onex/downloader/http_client.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace onex::downloader {

  namespace {

    auto write_to_buffer(void* contents, std::size_t size, std::size_t nmemb, void* userp)
        -> std::size_t {
      auto total = size * nmemb;
      auto& buf = *static_cast<std::vector<byte>*>(userp);
      auto* data = static_cast<const std::uint8_t*>(contents);
      buf.insert(buf.end(), data, data + total);
      return total;
    }

    auto write_to_file(void* contents, std::size_t size, std::size_t nmemb, void* userp)
        -> std::size_t {
      auto total = size * nmemb;
      auto& file = *static_cast<std::ofstream*>(userp);
      file.write(static_cast<const char*>(contents), static_cast<std::streamsize>(total));
      return file.good() ? total : 0;
    }

    auto apply_common(CURL* curl, const std::string& url, curl_slist*& headers) -> void {
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
      curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "GameforgeClient/2.8.5");
      headers = curl_slist_append(headers, "Origin: spark://www.gameforge.com");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Thread-safe reference-counted global init/cleanup so multiple
    // CurlHttpClient instances can be created across threads.
    std::mutex g_curl_global_mutex;
    int g_curl_refcount = 0;

    void curl_global_acquire() {
      std::lock_guard<std::mutex> lock(g_curl_global_mutex);
      if (g_curl_refcount++ == 0) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
      }
    }

    void curl_global_release() {
      std::lock_guard<std::mutex> lock(g_curl_global_mutex);
      if (--g_curl_refcount == 0) {
        curl_global_cleanup();
      }
    }

  }  // anonymous namespace

  struct CurlHttpClient::Impl {
    CURL* curl = nullptr;
  };

  CurlHttpClient::CurlHttpClient() : impl_(std::make_unique<Impl>()) {
    curl_global_acquire();
    impl_->curl = curl_easy_init();
  }

  CurlHttpClient::~CurlHttpClient() {
    if (impl_->curl) {
      curl_easy_cleanup(impl_->curl);
    }
    curl_global_release();
  }

  auto CurlHttpClient::get(const std::string& url) -> HttpResponse {
    HttpResponse resp;

    if (!impl_->curl) {
      resp.status_code = -1;
      return resp;
    }

    curl_easy_reset(impl_->curl);
    curl_slist* headers = nullptr;
    apply_common(impl_->curl, url, headers);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEDATA, &resp.body);

    auto res = curl_easy_perform(impl_->curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
      resp.status_code = -1;
      return resp;
    }

    curl_easy_getinfo(impl_->curl, CURLINFO_RESPONSE_CODE, &resp.status_code);
    return resp;
  }

  auto CurlHttpClient::download(const std::string& url, const std::string& path) -> HttpResponse {
    HttpResponse resp;

    if (!impl_->curl) {
      resp.status_code = -1;
      return resp;
    }

    std::ofstream file(path, std::ios::binary);
    if (!file) {
      resp.status_code = -1;
      return resp;
    }

    curl_easy_reset(impl_->curl);
    curl_slist* headers = nullptr;
    apply_common(impl_->curl, url, headers);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEFUNCTION, write_to_file);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEDATA, &file);

    auto res = curl_easy_perform(impl_->curl);
    curl_slist_free_all(headers);
    file.close();

    if (res != CURLE_OK) {
      resp.status_code = -1;
      return resp;
    }

    curl_easy_getinfo(impl_->curl, CURLINFO_RESPONSE_CODE, &resp.status_code);
    return resp;
  }

}  // namespace onex::downloader

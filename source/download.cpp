#include <curl/curl.h>
#include <onex/sha1.h>

#include <filesystem>
#include <format>
#include <iostream>
#include <nlohmann/json.hpp>
#include <onex/download.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace onex {

  namespace {

    constexpr std::string_view patches_base_url = "http://patches.gameforge.com";

    auto string_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
      auto& buf = *static_cast<std::string*>(userdata);
      buf.append(ptr, size * nmemb);
      return size * nmemb;
    }

    auto file_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
      auto* file = static_cast<FILE*>(userdata);
      return fwrite(ptr, size, nmemb, file);
    }

    auto perform_request(std::string_view url) -> std::string {
      auto* handle = curl_easy_init();
      if (!handle) throw std::runtime_error("failed to initialize curl handle");

      std::string data;
      auto url_str = std::string(url);
      curl_easy_setopt(handle, CURLOPT_URL, url_str.c_str());
      curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, string_write_callback);
      curl_easy_setopt(handle, CURLOPT_WRITEDATA, &data);
      curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);

      CURLcode res = curl_easy_perform(handle);
      long http_code = 0;
      curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_code);
      curl_easy_cleanup(handle);

      if (res != CURLE_OK) {
        throw std::runtime_error(
            std::format("HTTP request failed ({}): {}", http_code, curl_easy_strerror(res)));
      }

      return data;
    }

    auto sha1_matches(const ManifestEntry& entry, const fs::path& filepath) -> bool {
      if (!fs::exists(filepath)) return false;
      auto file_sha1 = Sha1::file_hex_digest(filepath.string());
      return file_sha1 == entry.sha1;
    }

    void download_file(const ManifestEntry& entry, const fs::path& output_path) {
      auto url = std::format("{}{}", patches_base_url, entry.path);

      auto* handle = curl_easy_init();
      if (!handle) throw std::runtime_error("failed to initialize curl handle");

      auto* file = fopen(output_path.string().c_str(), "wb");
      if (!file) {
        curl_easy_cleanup(handle);
        throw std::runtime_error(
            std::format("failed to open output file '{}'", output_path.string()));
      }

      auto url_str = std::string(url);
      curl_easy_setopt(handle, CURLOPT_URL, url_str.c_str());
      curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, file_write_callback);
      curl_easy_setopt(handle, CURLOPT_WRITEDATA, file);
      curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);

      CURLcode res = curl_easy_perform(handle);
      fclose(file);

      long http_code = 0;
      curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_code);
      curl_easy_cleanup(handle);

      if (res != CURLE_OK) {
        fs::remove(output_path);
        throw std::runtime_error(
            std::format("download failed ({}): {}", http_code, curl_easy_strerror(res)));
      }

      if (!sha1_matches(entry, output_path)) {
        fs::remove(output_path);
        throw std::runtime_error(
            std::format("SHA1 mismatch for '{}': downloaded file is corrupted", entry.file));
      }
    }

  }  // namespace

  std::string basename(std::string_view path) {
    auto pos = path.find_last_of("/\\");
    if (pos == std::string_view::npos) return std::string(path);
    return std::string(path.substr(pos + 1));
  }

  std::vector<ManifestEntry> fetch_manifest(std::string_view build_id) {
    auto url = std::format(
        "https://spark.gameforge.com/api/v1/patching/download/{}/nostale/default", build_id);

    auto response = perform_request(url);

    auto doc = nlohmann::json::parse(response);
    auto entries = doc["entries"];
    std::vector<ManifestEntry> result;
    result.reserve(entries.size());
    for (const auto& entry : entries) {
      if (!entry.contains("path")) continue;
      result.push_back({.file = entry["file"].get<std::string>(),
                        .path = entry["path"].get<std::string>(),
                        .size = entry["size"].get<int64_t>(),
                        .sha1 = entry["sha1"].get<std::string>()});
    }
    return result;
  }

  const ManifestEntry* match_archive(const std::vector<ManifestEntry>& manifest,
                                     std::string_view name) {
    std::vector<const ManifestEntry*> matches;

    for (const auto& entry : manifest) {
      if (entry.file == name) {
        matches.push_back(&entry);
      }
    }

    if (matches.size() > 1) {
      throw std::runtime_error(
          std::format("ambiguous archive name '{}': {} entries match", name, matches.size()));
    }

    if (matches.size() == 1) {
      return matches[0];
    }

    auto name_basename = basename(name);
    for (const auto& entry : manifest) {
      if (basename(entry.file) == name_basename) {
        matches.push_back(&entry);
      }
    }

    if (matches.size() > 1) {
      throw std::runtime_error(std::format(
          "ambiguous archive name '{}': {} entries match by filename", name, matches.size()));
    }

    if (matches.empty()) {
      throw std::runtime_error(std::format("archive '{}' not found in manifest", name));
    }

    return matches[0];
  }

  int run_download(const DownloadOptions& opts) {
    try {
      auto manifest = fetch_manifest(opts.build_id);

      fs::create_directories(opts.output_dir);

      for (const auto& name : opts.archive_names) {
        auto* entry = match_archive(manifest, name);

        auto output_path = fs::path(opts.output_dir) / basename(entry->file);

        if (sha1_matches(*entry, output_path)) {
          std::cout << std::format("SKIP {}: file exists with matching SHA1", entry->file)
                    << std::endl;
          continue;
        }

        download_file(*entry, output_path);
        std::cout << std::format("OK {}", entry->file) << std::endl;
      }

      return 0;
    } catch (const std::exception& e) {
      std::cerr << "OnexExplorerCli: error: " << e.what() << std::endl;
      return 1;
    }
  }

}  // namespace onex

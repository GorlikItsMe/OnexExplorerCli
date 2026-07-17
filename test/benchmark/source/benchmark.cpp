#include <fcntl.h>
#include <fpng.h>
#include <onex/archive/archive_format.h>
#include <onex/archive/codec.h>
#include <onex/archive/image/converter.h>
#include <onex/archive/image/entry_image.h>
#include <onex/archive/image/pixel_format.h>
#include <onex/archive/nos_archive.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <vector>

// ============================================================================
// Timing utilities
// ============================================================================

using clock_type = std::chrono::high_resolution_clock;

struct Phase {
  const char* name;
  double total_ms = 0;
  int64_t count = 0;
  double min_ms = std::numeric_limits<double>::max();
  double max_ms = 0;

  void record(double ms) {
    total_ms += ms;
    ++count;
    if (ms < min_ms) min_ms = ms;
    if (ms > max_ms) max_ms = ms;
  }

  void print() const {
    if (count == 0) return;
    std::printf("  %-28s cnt=%6jd  total=%9.2fms  avg=%9.3fms  min=%9.3fms  max=%9.3fms\n", name,
                static_cast<intmax_t>(count), total_ms, total_ms / count, min_ms, max_ms);
  }
};

class Timer {
public:
  Timer(Phase& p) : phase_(p), start_(clock_type::now()) {}
  ~Timer() { phase_.record(elapsed()); }
  double elapsed() const {
    return std::chrono::duration<double, std::milli>(clock_type::now() - start_).count();
  }

private:
  Phase& phase_;
  clock_type::time_point start_;
};

// ============================================================================
// Benchmark: open/parse a NOS file with sub-phase timing
//
// NOTE: This duplicates the open/parse flow from the production code
// (standalone/main.cpp and library internals). If you change how
// the library opens archives, update this function too.
// ============================================================================

struct OpenTiming {
  Phase header{"header read"};
  Phase detect{"detect format"};
  Phase parse{"parse entry table"};
  Phase mmap{"mmap"};
  Phase madvise{"madvise"};
  Phase total{"open total"};
};

static bool bench_open(const std::filesystem::path& path, OpenTiming& t) {
  Timer total(t.total);

  if (!std::filesystem::exists(path)) return false;

  std::ifstream file{path, std::ios::binary};
  if (!file.is_open()) return false;

  using Header = std::array<uint8_t, 16>;
  Header header{};
  {
    Timer timer(t.header);
    if (!file.read(reinterpret_cast<char*>(header.data()), header.size())) return false;
  }

  std::unique_ptr<onex::archive::ArchiveFormat> format;
  {
    Timer timer(t.detect);
    std::span<const uint8_t> hdr(header.data(), header.size());
    format = onex::archive::ArchiveFormat::detect(hdr);
    if (!format) return false;
  }

  onex::Result<std::vector<onex::archive::EntryInfo>> entries;
  {
    Timer timer(t.parse);
    std::span<const uint8_t> hdr(header.data(), header.size());
    entries = format->parse_entry_table(hdr, file);
    if (!entries) return false;
  }

  file.seekg(0, std::ios::end);
  auto file_size = file.tellg();
  if (file_size == -1) return false;
  auto size = static_cast<size_t>(file_size);

  {
    Timer timer(t.mmap);
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd == -1) return false;
    void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (addr == MAP_FAILED) return false;
    ::munmap(addr, size);
  }

  {
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd != -1) {
      void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      if (addr != MAP_FAILED) {
        Timer timer(t.madvise);
        ::posix_madvise(addr, size, POSIX_MADV_WILLNEED);
        ::posix_madvise(addr, size, POSIX_MADV_SEQUENTIAL);
        ::munmap(addr, size);
      }
      ::close(fd);
    }
  }

  return true;
}

// ============================================================================
// Benchmark: extract entries from a NOS file with sub-phase timing
// ============================================================================

struct ExtractTiming {
  Phase read_entry{"read_entry"};
  Phase parse_plan{"img: parse_image_plan"};
  Phase decode_px{"img: decode_pixels"};
  Phase fpng_enc{"img: fpng::encode_image"};
  Phase img_total{"img: decode_entry_to_png"};
  Phase file_write{"file write"};
  Phase entry_total{"entry total"};
};

static bool bench_extract(const std::filesystem::path& path, ExtractTiming& t,
                          uint64_t& total_bytes, int64_t max_entries = -1) {
  auto result = onex::archive::NosArchive::open(path);
  if (!result) return false;

  auto& archive = result.value;
  auto entries = archive.entries();
  int64_t limit = (max_entries < 0)
                      ? static_cast<int64_t>(entries.size())
                      : std::min<int64_t>(max_entries, static_cast<int64_t>(entries.size()));

  std::filesystem::create_directories("/tmp/onex_cpp_bench");

  for (int64_t i = 0; i < limit; ++i) {
    Timer entry_timer(t.entry_total);

    const auto& entry = entries[static_cast<size_t>(i)];

    std::vector<uint8_t> data;
    {
      Timer timer(t.read_entry);
      auto r = archive.read_entry(static_cast<size_t>(i));
      if (!r) continue;
      data = std::move(r.value);
    }

    const bool is_image = entry.type == onex::archive::EntryType::Texture
                          || entry.type == onex::archive::EntryType::Icon
                          || entry.type == onex::archive::EntryType::Image4B
                          || entry.type == onex::archive::EntryType::TileGrid;

    std::vector<uint8_t> output;
    const uint8_t* write_ptr = data.data();
    size_t write_size = data.size();

    if (is_image) {
      Timer total_img(t.img_total);

      int width = 0, height = 0;
      size_t pixel_offset = 0;
      auto pix_fmt = onex::archive::PixelFormat::kGbar4444;
      {
        Timer timer(t.parse_plan);
        // NOTE: Mirror of parse_image_plan() in source/archive/image/entry_image.cpp
        // If that function changes, update this switch.
        switch (entry.type) {
          case onex::archive::EntryType::Texture: {
            if (data.size() < 8) continue;
            width = static_cast<int>(static_cast<uint16_t>(data[0])
                                     | static_cast<uint16_t>(data[1]) << 8);
            height = static_cast<int>(static_cast<uint16_t>(data[2])
                                      | static_cast<uint16_t>(data[3]) << 8);
            pixel_offset = 8;
            switch (data[4]) {
              case 0:
                pix_fmt = onex::archive::PixelFormat::kGbar4444;
                break;
              case 1:
                pix_fmt = onex::archive::PixelFormat::kArgb555;
                break;
              case 2:
                pix_fmt = onex::archive::PixelFormat::kBgra8888;
                break;
              case 3:
              case 4:
                pix_fmt = onex::archive::PixelFormat::kGrayscale;
                break;
              default:
                continue;
            }
            break;
          }
          case onex::archive::EntryType::TileGrid: {
            if (data.size() < 4) continue;
            width = static_cast<int>(static_cast<uint16_t>(data[0])
                                     | static_cast<uint16_t>(data[1]) << 8);
            height = static_cast<int>(static_cast<uint16_t>(data[2])
                                      | static_cast<uint16_t>(data[3]) << 8);
            pixel_offset = 4;
            pix_fmt = onex::archive::PixelFormat::kNstc;
            break;
          }
          case onex::archive::EntryType::Icon: {
            if (data.size() < 13) continue;
            width = static_cast<int>(static_cast<uint16_t>(data[1])
                                     | static_cast<uint16_t>(data[2]) << 8);
            height = static_cast<int>(static_cast<uint16_t>(data[3])
                                      | static_cast<uint16_t>(data[4]) << 8);
            pixel_offset = 13;
            pix_fmt = onex::archive::PixelFormat::kGbar4444;
            break;
          }
          case onex::archive::EntryType::Image4B: {
            if (data.size() < 4) continue;
            width = static_cast<int>(static_cast<uint16_t>(data[0])
                                     | static_cast<uint16_t>(data[1]) << 8);
            height = static_cast<int>(static_cast<uint16_t>(data[2])
                                      | static_cast<uint16_t>(data[3]) << 8);
            pixel_offset = 4;
            pix_fmt = onex::archive::PixelFormat::kBgra8888Interlaced;
            break;
          }
          default:
            continue;
        }
      }

      if (width <= 0 || height <= 0) continue;

      std::vector<uint8_t> rgba;
      {
        Timer timer(t.decode_px);
        std::span<const uint8_t> pixel_span(data.data() + pixel_offset, data.size() - pixel_offset);
        // NOTE: Mirror of decode_pixels() in source/archive/image/entry_image.cpp
        // If new pixel formats are added or the switch changes, update this.
        switch (pix_fmt) {
          case onex::archive::PixelFormat::kGbar4444:
            rgba = onex::archive::decode_gbar4444_to_rgba(pixel_span, width, height);
            break;
          case onex::archive::PixelFormat::kBgra8888:
            rgba = onex::archive::decode_bgra8888_to_rgba(pixel_span, width, height);
            break;
          case onex::archive::PixelFormat::kArgb555:
            rgba = onex::archive::decode_argb555_to_rgba(pixel_span, width, height);
            break;
          case onex::archive::PixelFormat::kNstc:
            rgba = onex::archive::decode_nstc_to_rgba(pixel_span, width, height);
            break;
          case onex::archive::PixelFormat::kBgra8888Interlaced:
            rgba = onex::archive::decode_bgra8888_interlaced_to_rgba(pixel_span, width, height);
            break;
          case onex::archive::PixelFormat::kBarg4444:
            rgba = onex::archive::decode_barg4444_to_rgba(pixel_span, width, height);
            break;
          case onex::archive::PixelFormat::kGrayscale:
            rgba = onex::archive::decode_grayscale_to_rgba(pixel_span, width, height);
            break;
        }
      }

      if (rgba.empty()) continue;

      {
        Timer timer(t.fpng_enc);
        // Using FPNG_ENCODE_SLOWER (two-pass) — see source/archive/image/entry_image.cpp
        // for rationale. Benchmark results: test/benchmark/RESULTS_fpng.md
        auto ok = fpng::fpng_encode_image_to_memory(rgba.data(), static_cast<uint32_t>(width),
                                                    static_cast<uint32_t>(height), 4, output,
                                                    fpng::FPNG_ENCODE_SLOWER);
        if (!ok) continue;
      }

      write_ptr = output.data();
      write_size = output.size();
    }

    total_bytes += write_size;

    {
      Timer timer(t.file_write);
      auto out_name = path.filename().string() + "." + std::to_string(entry.id);
      auto out_path = std::filesystem::path("/tmp/onex_cpp_bench") / out_name;
      std::ofstream out{out_path.string(), std::ios::binary};
      if (out) {
        out.write(reinterpret_cast<const char*>(write_ptr),
                  static_cast<std::streamsize>(write_size));
      }
      std::error_code ec;
      std::filesystem::remove(out_path, ec);
    }
  }

  return true;
}

// ============================================================================
// Main
// ============================================================================

struct BenchFile {
  const char* name;
  const char* path;
};

static constexpr BenchFile kDefaultFiles[] = {
    {"NS4BbData", "temp/nostale/NostaleData/NS4BbData.NOS"},
    {"NStpData01", "temp/nostale/NostaleData/NStpData01.NOS"},
    {"NSmnData", "temp/nostale/NostaleData/NSmnData.NOS"},
    {"NSgtdData", "temp/nostale/NostaleData/NSgtdData.NOS"},
    {"NSmpData04", "temp/downloads/NostaleData/NSmpData04.NOS"},
};

static void run_open_bench(const std::filesystem::path& path, const char* name) {
  OpenTiming t;
  constexpr int kIterations = 5;
  for (int i = 0; i < kIterations; ++i) {
    bench_open(path, t);
  }

  std::printf("\n--- open: %s ---\n", name);
  t.header.print();
  t.detect.print();
  t.parse.print();
  t.mmap.print();
  t.madvise.print();
  t.total.print();
}

static void run_extract_bench(const std::filesystem::path& path, const char* name) {
  ExtractTiming t;
  uint64_t total_bytes = 0;
  constexpr int kIterations = 3;
  for (int i = 0; i < kIterations; ++i) {
    uint64_t bytes = 0;
    bench_extract(path, t, bytes);
    if (total_bytes == 0) total_bytes = bytes;
  }

  std::printf("--- extract: %s ---\n", name);
  t.read_entry.print();
  t.parse_plan.print();
  t.decode_px.print();
  t.fpng_enc.print();
  t.img_total.print();
  t.file_write.print();
  t.entry_total.print();
  std::printf("  %-28s %s %zu bytes\n", "total output size", "", static_cast<size_t>(total_bytes));
}

int main(int argc, char** argv) {
  fpng::fpng_init();

  std::vector<BenchFile> files;
  if (argc > 1) {
    for (int i = 1; i < argc; ++i) files.push_back({argv[i], argv[i]});
  } else {
    for (auto& f : kDefaultFiles) files.push_back(f);
  }

  for (auto& f : files) {
    std::printf("\n========================================\n");
    std::printf("=== %s ===\n", f.name);
    std::printf("========================================\n");

    auto path = std::filesystem::path(f.path);
    if (!std::filesystem::exists(path)) {
      std::printf("  SKIP: not found\n");
      continue;
    }

    run_open_bench(path, f.name);
    run_extract_bench(path, f.name);
  }

  std::printf("\nDone.\n");
  return 0;
}

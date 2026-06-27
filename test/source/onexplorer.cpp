// clang-format off
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <onexexplorer/stb_image_write.h>
#pragma GCC diagnostic pop
// clang-format on

#include <doctest/doctest.h>
#include <onexexplorer/version.h>

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("OnexExplorer version") {
  static_assert(std::string_view(ONEXEXPLORER_VERSION) == std::string_view("1.0"));
  CHECK(std::string(ONEXEXPLORER_VERSION) == std::string("1.0"));
}

namespace {

  struct WriteSink {
    std::vector<unsigned char> bytes;
  };

  void append_pixels(void* context, void* data, int size) {
    auto* sink = static_cast<WriteSink*>(context);
    auto* p = static_cast<unsigned char*>(data);
    sink->bytes.insert(sink->bytes.end(), p, p + size);
  }

}  // namespace

TEST_CASE("stb_image_write can produce a PNG in memory") {
  WriteSink sink;

  // 2x2 RGBA image: red, green, blue, white
  unsigned char pixels[] = {
      255, 0,   0,   255,  // top-left     red
      0,   255, 0,   255,  // top-right   green
      0,   0,   255, 255,  // bottom-left  blue
      255, 255, 255, 255,  // bottom-right white
  };

  int result = stbi_write_png_to_func(append_pixels, &sink, 2, 2, 4, pixels, 2 * 4);
  CHECK(result != 0);               // success code
  CHECK_FALSE(sink.bytes.empty());  // produced output
  CHECK(sink.bytes.size() >= 8);    // at least PNG header
  CHECK(sink.bytes[0] == 0x89);     // PNG magic byte 1
  CHECK(sink.bytes[1] == 0x50);     // PNG magic byte 2 'P'
  CHECK(sink.bytes[2] == 0x4E);     // PNG magic byte 3 'N'
  CHECK(sink.bytes[3] == 0x47);     // PNG magic byte 4 'G'
}

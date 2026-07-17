#include <fpng.h>
#include <onex/archive/image/converter.h>
#include <onex/archive/image/entry_image.h>
#include <onex/archive/image/pixel_format.h>

#include <cstddef>
#include <cstdint>

namespace {

  struct FpngInit {
    FpngInit() { fpng::fpng_init(); }
  };
  const FpngInit init_fpng;

}  // namespace

namespace onex::archive {

  namespace {

    // Layout of a decoded image entry: parsed dimensions, the pixel format and
    // where the raw pixel bytes start within the decompressed buffer.
    struct ImagePlan {
      int width;
      int height;
      PixelFormat format;
      size_t pixel_offset;
    };

    auto read_u16_le(const uint8_t* p) -> int {
      return static_cast<int>(static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(p[1]) << 8);
    }

    auto bytes_per_pixel(PixelFormat format) -> size_t {
      switch (format) {
        case PixelFormat::kBgra8888:
        case PixelFormat::kBgra8888Interlaced:
          return 4;
        case PixelFormat::kGbar4444:
        case PixelFormat::kArgb555:
        case PixelFormat::kBarg4444:
          return 2;
        case PixelFormat::kNstc:
        case PixelFormat::kGrayscale:
          return 1;
      }
      return 0;
    }

    auto decode_pixels(std::span<const uint8_t> pixels, const ImagePlan& plan)
        -> std::vector<uint8_t> {
      switch (plan.format) {
        case PixelFormat::kGbar4444:
          return decode_gbar4444_to_rgba(pixels, plan.width, plan.height);
        case PixelFormat::kBgra8888:
          return decode_bgra8888_to_rgba(pixels, plan.width, plan.height);
        case PixelFormat::kArgb555:
          return decode_argb555_to_rgba(pixels, plan.width, plan.height);
        case PixelFormat::kNstc:
          return decode_nstc_to_rgba(pixels, plan.width, plan.height);
        case PixelFormat::kBgra8888Interlaced:
          return decode_bgra8888_interlaced_to_rgba(pixels, plan.width, plan.height);
        case PixelFormat::kBarg4444:
          return decode_barg4444_to_rgba(pixels, plan.width, plan.height);
        case PixelFormat::kGrayscale:
          return decode_grayscale_to_rgba(pixels, plan.width, plan.height);
      }
      return {};
    }

    // Parses the entry-specific header. Returns an empty plan on failure, but
    // the caller maps that to kInvalidFormat.
    auto parse_image_plan(std::span<const uint8_t> data, EntryType type) -> Result<ImagePlan> {
      switch (type) {
        case EntryType::Texture: {
          // 8-byte header: width(2), height(2), format(1), smooth(1), flag(1),
          // mipmaps(1). Pixel data (the base frame) starts at offset 8.
          if (data.size() < 8) {
            return {{}, Error::kInvalidFormat};
          }
          ImagePlan plan{read_u16_le(data.data()), read_u16_le(data.data() + 2),
                         PixelFormat::kGbar4444, 8};
          switch (data[4]) {
            case 0:
              plan.format = PixelFormat::kGbar4444;
              break;
            case 1:
              plan.format = PixelFormat::kArgb555;
              break;
            case 2:
              plan.format = PixelFormat::kBgra8888;
              break;
            case 3:
            case 4:
              plan.format = PixelFormat::kGrayscale;
              break;
            default:
              return {{}, Error::kInvalidFormat};
          }
          return {plan};
        }
        case EntryType::TileGrid: {
          // 4-byte header: width(2), height(2). NSTC palette-indexed pixels.
          if (data.size() < 4) {
            return {{}, Error::kInvalidFormat};
          }
          return {ImagePlan{read_u16_le(data.data()), read_u16_le(data.data() + 2),
                            PixelFormat::kNstc, 4}};
        }
        case EntryType::Icon: {
          // 13-byte header: type(1), width(2)@1, height(2)@3, center(4),
          // unknown(4). GBAR4444 pixels.
          if (data.size() < 13) {
            return {{}, Error::kInvalidFormat};
          }
          return {ImagePlan{read_u16_le(data.data() + 1), read_u16_le(data.data() + 3),
                            PixelFormat::kGbar4444, 13}};
        }
        case EntryType::Image4B: {
          // 4-byte header: width(2), height(2). Interlaced BGRA8888 pixels.
          if (data.size() < 4) {
            return {{}, Error::kInvalidFormat};
          }
          return {ImagePlan{read_u16_le(data.data()), read_u16_le(data.data() + 2),
                            PixelFormat::kBgra8888Interlaced, 4}};
        }
        case EntryType::TextDat:
        case EntryType::TextLst:
        case EntryType::Unknown:
          return {{}, Error::kInvalidFormat};
      }
      return {{}, Error::kInvalidFormat};
    }

  }  // namespace

  auto decode_entry_to_png(std::span<const uint8_t> decompressed_data, EntryType type)
      -> Result<std::vector<uint8_t>> {
    auto plan = parse_image_plan(decompressed_data, type);
    if (!plan) {
      return {{}, plan.error};
    }

    if (plan.value.width <= 0 || plan.value.height <= 0) {
      return {{}, Error::kInvalidFormat};
    }

    const auto pixel_count
        = static_cast<size_t>(plan.value.width) * static_cast<size_t>(plan.value.height);
    const auto pixel_bytes = pixel_count * bytes_per_pixel(plan.value.format);
    if (decompressed_data.size() < plan.value.pixel_offset + pixel_bytes) {
      return {{}, Error::kInvalidFormat};
    }

    std::span<const uint8_t> pixels(decompressed_data.data() + plan.value.pixel_offset,
                                    pixel_bytes);
    auto rgba = decode_pixels(pixels, plan.value);
    if (rgba.empty()) {
      return {{}, Error::kInvalidFormat};
    }

    std::vector<uint8_t> png;
    if (!fpng::fpng_encode_image_to_memory(rgba.data(), static_cast<uint32_t>(plan.value.width),
                                           static_cast<uint32_t>(plan.value.height), 4, png,
                                           fpng::FPNG_ENCODE_SLOWER)) {  // two-pass chosen over
                                                                         // default one-pass —
                                                                         // see RESULTS_fpng.md
      return {{}, Error::kInvalidFormat};
    }
    return {std::move(png)};
  }

}  // namespace onex::archive

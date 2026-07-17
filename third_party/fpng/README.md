# fpng — Super fast C++ PNG writer/reader

- **Source**: https://github.com/richgel999/fpng
- **License**: Public Domain (Unlicense)
- **Files**: `fpng.h`, `fpng.cpp` (single-header + implementation)
- **Version**: commit `main` branch, fetched 2026-07-17

## Notes

- Only the encoder is used (`fpng_encode_image_to_memory`). The decoder is
  unused — we use lodepng for decoding in tests.
- SSE 4.1 + PCLMUL flags are enabled on x86/x64 only (see CMakeLists.txt).
- To update: re-download from
  https://raw.githubusercontent.com/richgel999/fpng/main/src/fpng.h and
  https://raw.githubusercontent.com/richgel999/fpng/main/src/fpng.cpp

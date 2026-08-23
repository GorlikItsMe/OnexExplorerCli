#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <onex/archive/image/entry_image.h>
#include <onex/archive/nos_archive.h>

#include <string>
#include <vector>

using namespace emscripten;
using namespace onex::archive;

class Archive {
public:
  Archive() = default;

  bool open(const std::string& filepath) {
    last_error_ = 0;
    auto result = NosArchive::open(filepath);
    if (!result) {
      last_error_ = static_cast<int>(result.error);
      return false;
    }
    archive_ = std::make_unique<NosArchive>(std::move(result.value));
    return true;
  }

  bool isOpen() const { return archive_ && archive_->is_open(); }

  std::string filepath() const { return archive_ ? archive_->filepath() : ""; }

  int entryCount() const {
    if (!archive_) return 0;
    return static_cast<int>(archive_->entries().size());
  }

  std::vector<uint8_t> readEntry(int index) {
    if (!archive_) return {};
    auto result = archive_->read_entry(static_cast<size_t>(index));
    if (!result) return {};
    return std::move(result.value);
  }

  val entryAt(int index) const {
    if (!archive_) return val::null();
    auto span = archive_->entries();
    if (index < 0 || static_cast<size_t>(index) >= span.size()) return val::null();
    const auto& e = span[static_cast<size_t>(index)];
    auto obj = val::object();
    obj.set("id", e.id);
    obj.set("name", e.name);
    obj.set("creationDate", e.creation_date);
    obj.set("compressed", e.compressed);
    obj.set("type", static_cast<int>(e.type));
    obj.set("offset", static_cast<double>(e.offset));
    obj.set("compressedSize", static_cast<double>(e.compressed_size));
    obj.set("uncompressedSize", static_cast<double>(e.uncompressed_size));
    return obj;
  }

  val entries() const {
    if (!archive_) return val::array();
    auto span = archive_->entries();
    auto arr = val::array();
    for (size_t i = 0; i < span.size(); ++i) {
      const auto& e = span[i];
      auto obj = val::object();
      obj.set("id", e.id);
      obj.set("name", e.name);
      obj.set("creationDate", e.creation_date);
      obj.set("compressed", e.compressed);
      obj.set("type", static_cast<int>(e.type));
      obj.set("offset", static_cast<double>(e.offset));
      obj.set("compressedSize", static_cast<double>(e.compressed_size));
      obj.set("uncompressedSize", static_cast<double>(e.uncompressed_size));
      arr.set(i, obj);
    }
    return arr;
  }

  int lastError() const { return last_error_; }

private:
  std::unique_ptr<NosArchive> archive_;
  int last_error_ = 0;
};

std::vector<uint8_t> decodeEntryToPng(std::vector<uint8_t> data, int type) {
  auto result = decode_entry_to_png(data, static_cast<EntryType>(type));
  if (!result) return {};
  return std::move(result.value);
}

EMSCRIPTEN_BINDINGS(onex_explorer) {
  class_<Archive>("Archive")
      .constructor<>()
      .function("open", &Archive::open)
      .function("isOpen", &Archive::isOpen)
      .function("filepath", &Archive::filepath)
      .function("entryCount", &Archive::entryCount)
      .function("readEntry", &Archive::readEntry)
      .function("entryAt", &Archive::entryAt)
      .function("entries", &Archive::entries)
      .function("lastError", &Archive::lastError);

  function("decodeEntryToPng", &decodeEntryToPng);

  // EntryType constants
  constant("ENTRY_TYPE_TEXTURE", static_cast<int>(EntryType::Texture));
  constant("ENTRY_TYPE_ICON", static_cast<int>(EntryType::Icon));
  constant("ENTRY_TYPE_IMAGE4B", static_cast<int>(EntryType::Image4B));
  constant("ENTRY_TYPE_TILE_GRID", static_cast<int>(EntryType::TileGrid));
  constant("ENTRY_TYPE_TEXT_DAT", static_cast<int>(EntryType::TextDat));
  constant("ENTRY_TYPE_TEXT_LST", static_cast<int>(EntryType::TextLst));
  constant("ENTRY_TYPE_UNKNOWN", static_cast<int>(EntryType::Unknown));
}

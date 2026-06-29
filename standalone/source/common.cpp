#include "common.h"

#include <onex/core/error.h>

namespace onex::cli {

  auto error_text(onex::Error e) -> std::string {
    switch (e) {
      case onex::Error::kNetworkError:
        return "network error";
      case onex::Error::kEntryNotFound:
        return "archive not found";
      case onex::Error::kAmbiguousMatch:
        return "ambiguous archive name (multiple manifest entries match)";
      case onex::Error::kFileNotFound:
        return "file not found";
      case onex::Error::kReadError:
        return "read error";
      case onex::Error::kInvalidHeader:
        return "invalid manifest";
      case onex::Error::kInvalidFormat:
        return "invalid or unsupported archive format";
      case onex::Error::kIoError:
        return "I/O error";
      default:
        return "unknown error";
    }
  }

  auto entry_type_name(onex::archive::EntryType type) -> const char* {
    switch (type) {
      case onex::archive::EntryType::Texture:
        return "Texture";
      case onex::archive::EntryType::Icon:
        return "Icon";
      case onex::archive::EntryType::Image4B:
        return "Image4B";
      case onex::archive::EntryType::TileGrid:
        return "TileGrid";
      case onex::archive::EntryType::TextDat:
        return "TextDat";
      case onex::archive::EntryType::TextLst:
        return "TextLst";
      case onex::archive::EntryType::Unknown:
        return "Unknown";
    }
    return "Unknown";
  }

  auto entry_to_json(const onex::archive::EntryInfo& entry) -> nlohmann::json {
    return {
        {"id", entry.id},
        {"name", entry.name},
        {"type", entry_type_name(entry.type)},
        {"creation_date", entry.creation_date},
        {"compressed", entry.compressed},
        {"offset", entry.offset},
        {"compressed_size", entry.compressed_size},
        {"uncompressed_size", entry.uncompressed_size},
    };
  }

}  // namespace onex::cli

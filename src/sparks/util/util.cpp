#include "sparks/util/util.h"

#include "grassland/grassland.h"

namespace sparks {
std::string PathToFilename(const std::string &file_path) {
  std::wstring wide_file_path =
      grassland::util::U8StringToWideString(file_path);
  std::wstring short_name;
  for (auto c : wide_file_path) {
    if (c == L'/' || c == L'\\') {
      short_name.clear();
    } else {
      short_name += c;
    }
  }
  return grassland::util::WideStringToU8String(short_name);
}
}  // namespace sparks

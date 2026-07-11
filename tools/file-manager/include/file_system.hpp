#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace turtle::file_manager {

// File metadata is collected independently of GTK so navigation and previews
// remain testable and can later be reused by non-GUI application services.
enum class EntryKind { Directory, Image, Text, Video, Other };

struct FileEntry {
    std::filesystem::path path;
    std::string name;
    EntryKind kind = EntryKind::Other;
    std::uintmax_t size = 0;
};

class FileSystem {
public:
    bool is_directory_readable(const std::filesystem::path& path, std::string& error) const;
    std::vector<FileEntry> read_directory(const std::filesystem::path& path, std::string& error) const;
    FileEntry describe(const std::filesystem::path& path) const;
    static std::filesystem::path home_directory();
    static std::filesystem::path trash_directory();
};

}  // namespace turtle::file_manager

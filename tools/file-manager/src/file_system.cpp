#include "file_system.hpp"

#include <algorithm>
#include <cstdlib>
#include <system_error>

namespace turtle::file_manager {
namespace {

EntryKind classify(const std::filesystem::path& path) {
    std::error_code error;
    if (std::filesystem::is_directory(path, error)) return EntryKind::Directory;
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".gif" || extension == ".webp") return EntryKind::Image;
    if (extension == ".txt" || extension == ".md" || extension == ".cpp" || extension == ".hpp" || extension == ".c" || extension == ".h" || extension == ".json" || extension == ".xml") return EntryKind::Text;
    if (extension == ".mp4" || extension == ".mkv" || extension == ".webm" || extension == ".avi") return EntryKind::Video;
    return EntryKind::Other;
}

}  // namespace

bool FileSystem::is_directory_readable(const std::filesystem::path& path, std::string& error) const {
    std::error_code ec;
    if (!std::filesystem::is_directory(path, ec)) {
        error = ec ? ec.message() : "The location is not a directory.";
        return false;
    }
    std::filesystem::directory_iterator iterator(path, ec);
    if (ec) { error = ec.message(); return false; }
    return true;
}

std::vector<FileEntry> FileSystem::read_directory(const std::filesystem::path& path, std::string& error) const {
    std::vector<FileEntry> entries;
    std::error_code ec;
    std::filesystem::directory_iterator iterator(path, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec) { error = ec.message(); return entries; }
    for (const auto& item : iterator) entries.push_back(describe(item.path()));
    std::sort(entries.begin(), entries.end(), [](const FileEntry& left, const FileEntry& right) {
        if ((left.kind == EntryKind::Directory) != (right.kind == EntryKind::Directory)) return left.kind == EntryKind::Directory;
        return left.name < right.name;
    });
    return entries;
}

FileEntry FileSystem::describe(const std::filesystem::path& path) const {
    std::error_code ec;
    FileEntry entry{path, path.filename().string(), classify(path), 0};
    if (entry.name.empty()) entry.name = path.string();
    if (std::filesystem::is_regular_file(path, ec)) entry.size = std::filesystem::file_size(path, ec);
    return entry;
}

std::filesystem::path FileSystem::home_directory() {
    const char* home = std::getenv("HOME");
    return home ? std::filesystem::path(home) : std::filesystem::path("/");
}

std::filesystem::path FileSystem::trash_directory() { return home_directory() / ".local/share/Trash/files"; }

}  // namespace turtle::file_manager

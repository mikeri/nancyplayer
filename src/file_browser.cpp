#include "file_browser.h"
#include <algorithm>
#include <iostream>

FileBrowser::FileBrowser() : selected_index(0) {
    current_path = std::filesystem::current_path().string();
    scanDirectory();
}

void FileBrowser::setDirectory(const std::string& path) {
    try {
        std::filesystem::path new_path = std::filesystem::canonical(path);
        current_path = new_path.string();
        selected_index = 0;
        scanDirectory();
    } catch (const std::filesystem::filesystem_error&) {
    }
}

void FileBrowser::refresh() {
    scanDirectory();
}

void FileBrowser::moveUp() {
    if (selected_index > 0) {
        selected_index--;
    }
}

void FileBrowser::moveDown() {
    if (selected_index < static_cast<int>(entries.size()) - 1) {
        selected_index++;
    }
}

void FileBrowser::enterDirectory() {
    if (selected_index < entries.size() && entries[selected_index].is_directory) {
        setDirectory(entries[selected_index].path);
    }
}

void FileBrowser::goToParent() {
    std::filesystem::path parent = std::filesystem::path(current_path).parent_path();
    if (parent != current_path) {
        setDirectory(parent.string());
    }
}

void FileBrowser::navigateToFile(const std::string& file_path) {
    try {
        std::filesystem::path target_path = std::filesystem::path(file_path);
        std::filesystem::path target_dir = target_path.parent_path();
        std::string target_filename = target_path.filename().string();
        
        // Navigate to the directory containing the file
        setDirectory(target_dir.string());
        
        // Find and select the target file in the entries
        for (size_t i = 0; i < entries.size(); i++) {
            if (entries[i].name == target_filename) {
                selected_index = static_cast<int>(i);
                break;
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // If navigation fails, just ignore it
    }
}

std::string FileBrowser::getSelectedFile() const {
    if (selected_index < entries.size()) {
        return entries[selected_index].path;
    }
    return "";
}

void FileBrowser::scanDirectory() {
    entries.clear();
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(current_path)) {
            FileEntry file_entry;
            file_entry.name = entry.path().filename().string();
            file_entry.path = entry.path().string();
            file_entry.is_directory = entry.is_directory();
            file_entry.is_sid_file = !file_entry.is_directory && isSidFile(file_entry.name);
            
            if (file_entry.is_directory || file_entry.is_sid_file) {
                entries.push_back(file_entry);
            }
        }
        
        std::sort(entries.begin(), entries.end(), [](const FileEntry& a, const FileEntry& b) {
            if (a.is_directory != b.is_directory) {
                return a.is_directory;
            }
            return a.name < b.name;
        });
        
        if (selected_index >= entries.size()) {
            selected_index = std::max(0, static_cast<int>(entries.size()) - 1);
        }
        
    } catch (const std::filesystem::filesystem_error&) {
    }
}

bool FileBrowser::isSidFile(const std::string& filename) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    auto endsWith = [](const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    };
    
    return endsWith(lower, ".sid") || 
           endsWith(lower, ".psid") || 
           endsWith(lower, ".rsid") ||
           endsWith(lower, ".mus") ||
           endsWith(lower, ".str") ||
           endsWith(lower, ".prg");
}
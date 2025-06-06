#include "stil_reader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>

// Debug logging disabled - STIL parsing is working correctly
// static std::ofstream debug_log("/tmp/nancyplayer_debug.log");

StilReader::StilReader() {
}

bool StilReader::loadDatabase(const std::string& hvsc_root) {
    try {
        hvsc_root_path = std::filesystem::canonical(hvsc_root).string();
    } catch (const std::filesystem::filesystem_error& e) {
        hvsc_root_path = hvsc_root;
    }
    
    // Look for STIL.txt in common locations
    std::vector<std::string> stil_paths = {
        hvsc_root_path + "/DOCUMENTS/STIL.txt",
        hvsc_root_path + "/STIL.txt",
        hvsc_root_path + "/documents/STIL.txt",
        hvsc_root_path + "/stil.txt"
    };
    
    // debug_log << "Searching for STIL database in HVSC root: " << hvsc_root_path << std::endl;
    
    for (const auto& path : stil_paths) {
        // debug_log << "Checking: " << path << std::endl;
        if (std::filesystem::exists(path)) {
            // debug_log << "Found STIL database at: " << path << std::endl;
            parseStilFile(path);
            return true;
        }
    }
    
    // debug_log << "STIL.txt not found in any expected location" << std::endl;
    return false;
}

StilEntry StilReader::getInfo(const std::string& sid_file_path) const {
    std::string normalized_path = normalizePathForStil(sid_file_path);
    
    auto it = stil_entries.find(normalized_path);
    if (it != stil_entries.end()) {
        return it->second;
    }
    
    return StilEntry{};
}

bool StilReader::hasInfo(const std::string& sid_file_path) const {
    std::string normalized_path = normalizePathForStil(sid_file_path);
    return stil_entries.find(normalized_path) != stil_entries.end();
}

void StilReader::parseStilFile(const std::string& stil_file_path) {
    std::ifstream file(stil_file_path);
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    std::string current_file;
    StilEntry current_entry;
    std::vector<std::string> comment_lines;
    
    auto saveCurrentEntry = [&]() {
        if (!current_file.empty()) {
            // Join comment lines with spaces
            if (!comment_lines.empty()) {
                std::ostringstream comment_stream;
                for (size_t i = 0; i < comment_lines.size(); ++i) {
                    if (i > 0) comment_stream << " ";
                    comment_stream << comment_lines[i];
                }
                current_entry.comment = comment_stream.str();
            }
            stil_entries[current_file] = current_entry;
        }
    };
    
    while (std::getline(file, line)) {
        // Remove carriage return (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Skip empty lines and comments starting with #
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Check if this is a file path line (starts with /)
        if (line[0] == '/') {
            // Save previous entry
            saveCurrentEntry();
            
            // Start new entry
            current_file = line;
            current_entry = StilEntry{};
            comment_lines.clear();
        }
        else if (!current_file.empty()) {
            // Parse field lines - look for patterns with colons
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string field_name = line.substr(0, colon_pos);
                std::string field_value = line.substr(colon_pos + 1);
                
                // Trim leading/trailing spaces and carriage returns from field name and value
                field_name.erase(0, field_name.find_first_not_of(" \t\r"));
                field_name.erase(field_name.find_last_not_of(" \t\r") + 1);
                field_value.erase(0, field_value.find_first_not_of(" \t\r"));
                field_value.erase(field_value.find_last_not_of(" \t\r") + 1);
                
                if (field_name == "TITLE") {
                    current_entry.title = field_value;
                }
                else if (field_name == "ARTIST") {
                    current_entry.artist = field_value;
                }
                else if (field_name == "COPYRIGHT") {
                    current_entry.copyright = field_value;
                }
                else if (field_name == "COMMENT") {
                    comment_lines.clear();
                    comment_lines.push_back(field_value);
                }
            }
            else if (!comment_lines.empty() && line.find_first_not_of(' ') != std::string::npos) {
                // This is a continuation line for a comment
                std::string trimmed = line;
                trimmed.erase(0, trimmed.find_first_not_of(" \t\r"));
                trimmed.erase(trimmed.find_last_not_of(" \t\r") + 1);
                comment_lines.push_back(trimmed);
            }
            else if (line.find("(#") != std::string::npos) {
                // Subtune information
                current_entry.subtune_info.push_back(line);
            }
        }
    }
    
    // Save last entry
    saveCurrentEntry();
}

std::string StilReader::normalizePathForStil(const std::string& sid_file_path) const {
    try {
        // Get absolute path of the SID file
        std::filesystem::path abs_path;
        if (std::filesystem::path(sid_file_path).is_absolute()) {
            abs_path = std::filesystem::canonical(sid_file_path);
        } else {
            abs_path = std::filesystem::canonical(std::filesystem::current_path() / sid_file_path);
        }
        
        // Get absolute path of HVSC root
        std::filesystem::path hvsc_path = std::filesystem::canonical(hvsc_root_path);
        
        // Get relative path from HVSC root
        std::filesystem::path rel_path = std::filesystem::relative(abs_path, hvsc_path);
        
        // Convert to STIL format (forward slashes, leading slash)
        std::string stil_path = "/" + rel_path.string();
        
        // Replace backslashes with forward slashes (Windows compatibility)
        std::replace(stil_path.begin(), stil_path.end(), '\\', '/');
        
        return stil_path;
    } catch (const std::filesystem::filesystem_error& e) {
        return "";
    }
}
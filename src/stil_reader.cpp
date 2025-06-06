#include "stil_reader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>

static std::ofstream debug_log("/tmp/nancyplayer_debug.log");

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
    
    debug_log << "Searching for STIL database in HVSC root: " << hvsc_root_path << std::endl;
    
    for (const auto& path : stil_paths) {
        debug_log << "Checking: " << path << std::endl;
        if (std::filesystem::exists(path)) {
            debug_log << "Found STIL database at: " << path << std::endl;
            parseStilFile(path);
            return true;
        }
    }
    
    debug_log << "STIL.txt not found in any expected location" << std::endl;
    return false;
}

StilEntry StilReader::getInfo(const std::string& sid_file_path) const {
    std::string normalized_path = normalizePathForStil(sid_file_path);
    debug_log << "getInfo called for: '" << sid_file_path << "' -> normalized: '" << normalized_path << "'" << std::endl;
    
    auto it = stil_entries.find(normalized_path);
    if (it != stil_entries.end()) {
        debug_log << "Found STIL entry!" << std::endl;
        return it->second;
    }
    
    debug_log << "No STIL entry found. Available keys:" << std::endl;
    for (const auto& entry : stil_entries) {
        debug_log << "  '" << entry.first << "'" << std::endl;
    }
    
    return StilEntry{};
}

bool StilReader::hasInfo(const std::string& sid_file_path) const {
    std::string normalized_path = normalizePathForStil(sid_file_path);
    bool has_info = stil_entries.find(normalized_path) != stil_entries.end();
    debug_log << "hasInfo for '" << sid_file_path << "' -> '" << normalized_path << "': " << (has_info ? "YES" : "NO") << std::endl;
    return has_info;
}

void StilReader::parseStilFile(const std::string& stil_file_path) {
    std::ifstream file(stil_file_path);
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    std::string current_file;
    StilEntry current_entry;
    bool in_comment = false;
    std::ostringstream comment_stream;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Check if this is a file path line (starts with /)
        if (line[0] == '/') {
            // Save previous entry if we have one
            if (!current_file.empty()) {
                if (in_comment) {
                    current_entry.comment = comment_stream.str();
                    comment_stream.clear();
                    comment_stream.str("");
                    in_comment = false;
                }
                stil_entries[current_file] = current_entry;
            }
            
            // Start new entry
            current_file = line;
            current_entry = StilEntry{};
            in_comment = false;
        }
        // Check for field lines (TITLE:, ARTIST:, etc.)
        else if (line.find("   TITLE: ") == 0) {
            current_entry.title = line.substr(10);
        }
        else if (line.find("  ARTIST: ") == 0) {
            current_entry.artist = line.substr(10);
        }
        else if (line.find("COPYRIGHT: ") == 0) {
            current_entry.copyright = line.substr(11);
        }
        else if (line.find(" COMMENT: ") == 0) {
            in_comment = true;
            comment_stream << line.substr(10);
        }
        else if (in_comment && line.find("          ") == 0) {
            // Continuation of comment
            comment_stream << " " << line.substr(10);
        }
        else if (line.find("(#") != std::string::npos) {
            // Subtune information
            current_entry.subtune_info.push_back(line);
        }
        else if (in_comment) {
            // End of comment block
            current_entry.comment = comment_stream.str();
            comment_stream.clear();
            comment_stream.str("");
            in_comment = false;
        }
    }
    
    // Save last entry
    if (!current_file.empty()) {
        if (in_comment) {
            current_entry.comment = comment_stream.str();
        }
        stil_entries[current_file] = current_entry;
    }
    
    debug_log << "Loaded " << stil_entries.size() << " STIL entries" << std::endl;
    
    // Log first 10 entries to see the key format
    debug_log << "Sample STIL keys:" << std::endl;
    int count = 0;
    for (const auto& entry : stil_entries) {
        if (count++ >= 10) break;
        debug_log << "  '" << entry.first << "'" << std::endl;
    }
}

std::string StilReader::normalizePathForStil(const std::string& sid_file_path) const {
    try {
        // If the path is already absolute, use it directly
        std::filesystem::path abs_path;
        if (std::filesystem::path(sid_file_path).is_absolute()) {
            abs_path = std::filesystem::canonical(sid_file_path);
        } else {
            abs_path = std::filesystem::canonical(std::filesystem::current_path() / sid_file_path);
        }
        
        std::filesystem::path hvsc_path = std::filesystem::path(hvsc_root_path);
        
        // Get relative path from HVSC root
        std::filesystem::path rel_path = std::filesystem::relative(abs_path, hvsc_path);
        
        // Convert to STIL format (forward slashes, leading slash)
        std::string stil_path = "/" + rel_path.string();
        
        // Replace backslashes with forward slashes (Windows compatibility)
        std::replace(stil_path.begin(), stil_path.end(), '\\', '/');
        
        debug_log << "Path normalization: '" << sid_file_path << "' -> '" << stil_path << "'" << std::endl;
        return stil_path;
    } catch (const std::filesystem::filesystem_error& e) {
        debug_log << "Path normalization failed for '" << sid_file_path << "': " << e.what() << std::endl;
        return "";
    }
}
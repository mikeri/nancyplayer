#include "search.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cctype>

static std::ofstream debug_log("/tmp/nancyplayer_search_debug.log");

Search::Search() {
}

bool Search::loadDatabase(const std::string& hvsc_root) {
    try {
        hvsc_root_path = std::filesystem::canonical(hvsc_root).string();
    } catch (const std::filesystem::filesystem_error& e) {
        hvsc_root_path = hvsc_root;
    }
    
    // Look for Songlengths.md5 in common locations
    std::vector<std::string> songlengths_paths = {
        hvsc_root_path + "/DOCUMENTS/Songlengths.md5",
        hvsc_root_path + "/Songlengths.md5",
        hvsc_root_path + "/documents/Songlengths.md5",
        hvsc_root_path + "/songlengths.md5"
    };
    
    debug_log << "Searching for Songlengths.md5 in HVSC root: " << hvsc_root_path << std::endl;
    
    bool found_songlengths = false;
    for (const auto& path : songlengths_paths) {
        debug_log << "Checking: " << path << std::endl;
        if (std::filesystem::exists(path)) {
            debug_log << "Found Songlengths.md5 at: " << path << std::endl;
            parseSonglengthsFile(path);
            found_songlengths = true;
            break;
        }
    }
    
    if (!found_songlengths) {
        debug_log << "Songlengths.md5 not found in any expected location" << std::endl;
        debug_log << "Search will only work with STIL data if available" << std::endl;
    }
    
    // Also load STIL data for titles and artists
    std::vector<std::string> stil_paths = {
        hvsc_root_path + "/DOCUMENTS/STIL.txt",
        hvsc_root_path + "/STIL.txt",
        hvsc_root_path + "/documents/STIL.txt",
        hvsc_root_path + "/stil.txt"
    };
    
    for (const auto& path : stil_paths) {
        if (std::filesystem::exists(path)) {
            debug_log << "Found STIL database at: " << path << std::endl;
            parseStilFile(path);
            break;
        }
    }
    
    return true;
}

void Search::parseSonglengthsFile(const std::string& songlengths_file_path) {
    std::ifstream file(songlengths_file_path);
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    std::string current_path;
    
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) {
            continue;
        }
        
        // Check for comment lines that contain file paths
        if (line[0] == ';' && line.find('/') != std::string::npos) {
            // Extract path from comment line: "; /DEMOS/0-9/10_Orbyte.sid"
            size_t path_start = line.find('/');
            if (path_start != std::string::npos) {
                current_path = line.substr(path_start);
                // Remove any trailing whitespace
                current_path.erase(current_path.find_last_not_of(" \t\r\n") + 1);
            }
            continue;
        }
        
        // Skip other comment lines
        if (line[0] == ';') {
            continue;
        }
        
        // Parse MD5=length format
        size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos || current_path.empty()) {
            continue;
        }
        
        std::string md5 = line.substr(0, equals_pos);
        std::string length_str = line.substr(equals_pos + 1);
        
        // Parse length (format: mm:ss or mm:ss.ms)
        std::vector<int> lengths;
        size_t dot_pos = length_str.find('.');
        if (dot_pos != std::string::npos) {
            length_str = length_str.substr(0, dot_pos); // Remove milliseconds
        }
        
        size_t colon_pos = length_str.find(':');
        if (colon_pos != std::string::npos) {
            int minutes = std::stoi(length_str.substr(0, colon_pos));
            int seconds = std::stoi(length_str.substr(colon_pos + 1));
            lengths.push_back(minutes * 60 + seconds);
        }
        
        // Create song entry
        SongEntry entry;
        entry.path = current_path;
        entry.md5 = md5;
        entry.lengths = lengths;
        
        // Extract filename
        size_t last_slash = current_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            entry.filename = current_path.substr(last_slash + 1);
        } else {
            entry.filename = current_path;
        }
        
        debug_log << "Parsed entry: " << current_path << " -> " << entry.filename << " (" << (lengths.empty() ? 0 : lengths[0]) << "s)" << std::endl;
        
        // Store in both maps
        song_entries[current_path] = entry;
        md5_to_path[md5] = current_path;
    }
    
    debug_log << "Loaded " << song_entries.size() << " song entries from Songlengths.md5" << std::endl;
    
    // Log first few entries for debugging
    debug_log << "Sample song entries:" << std::endl;
    int count = 0;
    for (const auto& entry_pair : song_entries) {
        if (count++ >= 5) break;
        debug_log << "  Path: '" << entry_pair.first << "'" << std::endl;
        debug_log << "  Filename: '" << entry_pair.second.filename << "'" << std::endl;
        debug_log << "  Title: '" << entry_pair.second.title << "'" << std::endl;
        debug_log << "  Artist: '" << entry_pair.second.artist << "'" << std::endl;
    }
}

void Search::parseStilFile(const std::string& stil_file_path) {
    std::ifstream file(stil_file_path);
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    std::string current_file;
    std::string current_title;
    std::string current_artist;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Check if this is a file path line (starts with /)
        if (line[0] == '/') {
            // Save previous entry if we have one
            if (!current_file.empty() && song_entries.find(current_file) != song_entries.end()) {
                song_entries[current_file].title = current_title;
                song_entries[current_file].artist = current_artist;
            }
            
            // Start new entry
            current_file = line;
            current_title.clear();
            current_artist.clear();
        }
        // Check for field lines
        else if (line.find("   TITLE: ") == 0) {
            current_title = line.substr(10);
        }
        else if (line.find("  ARTIST: ") == 0) {
            current_artist = line.substr(10);
        }
    }
    
    // Save last entry
    if (!current_file.empty() && song_entries.find(current_file) != song_entries.end()) {
        song_entries[current_file].title = current_title;
        song_entries[current_file].artist = current_artist;
    }
    
    debug_log << "Enhanced song entries with STIL data" << std::endl;
}

void Search::indexLocalSidFiles(const std::string& directory) {
    debug_log << "Indexing local SID files in: " << directory << std::endl;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string extension = entry.path().extension().string();
                
                // Convert extension to lowercase for comparison
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".sid") {
                    // Create song entry for SID file
                    SongEntry song_entry;
                    song_entry.path = "/" + std::filesystem::relative(entry.path(), directory).string();
                    song_entry.filename = entry.path().filename().string();
                    song_entry.md5 = ""; // No MD5 available
                    
                    // Replace backslashes with forward slashes (Windows compatibility)
                    std::replace(song_entry.path.begin(), song_entry.path.end(), '\\', '/');
                    
                    debug_log << "Indexed SID file: " << song_entry.path << " (" << song_entry.filename << ")" << std::endl;
                    
                    song_entries[song_entry.path] = song_entry;
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        debug_log << "Error indexing directory: " << e.what() << std::endl;
    }
    
    debug_log << "Indexed " << song_entries.size() << " local SID files" << std::endl;
}

std::vector<SongEntry> Search::search(const std::string& query) const {
    std::vector<SongEntry> results;
    
    debug_log << "Search called with query: '" << query << "'" << std::endl;
    debug_log << "Song entries count: " << song_entries.size() << std::endl;
    
    if (query.empty()) {
        debug_log << "Empty query, returning empty results" << std::endl;
        return results;
    }
    
    // Convert query to lowercase for case-insensitive search
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    debug_log << "Searching for: '" << lower_query << "'" << std::endl;
    
    for (const auto& entry_pair : song_entries) {
        const SongEntry& entry = entry_pair.second;
        
        // Search in filename, title, and artist
        std::string search_text = entry.filename + " " + entry.title + " " + entry.artist;
        std::transform(search_text.begin(), search_text.end(), search_text.begin(), ::tolower);
        
        if (search_text.find(lower_query) != std::string::npos) {
            debug_log << "Found match: " << entry.filename << std::endl;
            results.push_back(entry);
        }
    }
    
    debug_log << "Search returned " << results.size() << " results" << std::endl;
    
    // Sort results by relevance (prefer title/artist matches over filename)
    std::sort(results.begin(), results.end(), [&lower_query](const SongEntry& a, const SongEntry& b) {
        std::string a_title_artist = a.title + " " + a.artist;
        std::string b_title_artist = b.title + " " + b.artist;
        std::transform(a_title_artist.begin(), a_title_artist.end(), a_title_artist.begin(), ::tolower);
        std::transform(b_title_artist.begin(), b_title_artist.end(), b_title_artist.begin(), ::tolower);
        
        bool a_in_title_artist = a_title_artist.find(lower_query) != std::string::npos;
        bool b_in_title_artist = b_title_artist.find(lower_query) != std::string::npos;
        
        if (a_in_title_artist && !b_in_title_artist) return true;
        if (!a_in_title_artist && b_in_title_artist) return false;
        
        return a.getDisplayName() < b.getDisplayName();
    });
    
    return results;
}

SongEntry Search::getSongInfo(const std::string& sid_file_path) const {
    std::string normalized_path = normalizePathForLookup(sid_file_path);
    auto it = song_entries.find(normalized_path);
    if (it != song_entries.end()) {
        return it->second;
    }
    return SongEntry{};
}

bool Search::hasSongInfo(const std::string& sid_file_path) const {
    std::string normalized_path = normalizePathForLookup(sid_file_path);
    return song_entries.find(normalized_path) != song_entries.end();
}

int Search::getSongLength(const std::string& sid_file_path, int track) const {
    std::string normalized_path = normalizePathForLookup(sid_file_path);
    auto it = song_entries.find(normalized_path);
    if (it != song_entries.end() && track >= 1 && track <= (int)it->second.lengths.size()) {
        return it->second.lengths[track - 1]; // Convert to 0-based index
    }
    return 0;
}

std::string Search::normalizePathForLookup(const std::string& sid_file_path) const {
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
        
        // Convert to HVSC format (forward slashes, leading slash)
        std::string hvsc_path_str = "/" + rel_path.string();
        
        // Replace backslashes with forward slashes (Windows compatibility)
        std::replace(hvsc_path_str.begin(), hvsc_path_str.end(), '\\', '/');
        
        debug_log << "Path normalization: '" << sid_file_path << "' -> '" << hvsc_path_str << "'" << std::endl;
        return hvsc_path_str;
    } catch (const std::filesystem::filesystem_error& e) {
        debug_log << "Path normalization failed for '" << sid_file_path << "': " << e.what() << std::endl;
        return "";
    }
}
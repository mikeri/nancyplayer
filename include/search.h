#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

struct SongEntry {
    std::string path;
    std::string filename;
    std::string title;
    std::string artist;
    std::vector<int> lengths; // lengths for each subtune in seconds
    std::string md5;
    
    std::string getDisplayName() const {
        if (!title.empty() && !artist.empty()) {
            return artist + " - " + title;
        } else if (!title.empty()) {
            return title;
        } else {
            return filename;
        }
    }
};

class Search {
public:
    Search();
    
    bool loadDatabase(const std::string& hvsc_root);
    std::vector<SongEntry> search(const std::string& query) const;
    SongEntry getSongInfo(const std::string& sid_file_path) const;
    bool hasSongInfo(const std::string& sid_file_path) const;
    int getSongLength(const std::string& sid_file_path, int track = 1) const;
    size_t getEntryCount() const { return song_entries.size(); }
    
private:
    void parseSonglengthsFile(const std::string& songlengths_file_path);
    void parseStilFile(const std::string& stil_file_path);
    void indexLocalSidFiles(const std::string& directory);
    std::string normalizePathForLookup(const std::string& sid_file_path) const;
    
    std::unordered_map<std::string, SongEntry> song_entries; // keyed by normalized path
    std::unordered_map<std::string, std::string> md5_to_path; // MD5 to path mapping
    std::string hvsc_root_path;
};
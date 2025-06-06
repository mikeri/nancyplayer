#pragma once

#include <string>
#include <map>
#include <vector>

struct StilEntry {
    std::string title;
    std::string artist;
    std::string comment;
    std::string copyright;
    std::vector<std::string> subtune_info;
};

class StilReader {
public:
    StilReader();
    
    bool loadDatabase(const std::string& hvsc_root);
    StilEntry getInfo(const std::string& sid_file_path) const;
    bool hasInfo(const std::string& sid_file_path) const;
    size_t getEntryCount() const { return stil_entries.size(); }
    
private:
    void parseStilFile(const std::string& stil_file_path);
    std::string normalizePathForStil(const std::string& sid_file_path) const;
    
    std::map<std::string, StilEntry> stil_entries;
    std::string hvsc_root_path;
};
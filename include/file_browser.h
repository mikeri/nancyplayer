#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct FileEntry {
    std::string name;
    std::string path;
    bool is_directory;
    bool is_sid_file;
};

class FileBrowser {
public:
    FileBrowser();
    
    void setDirectory(const std::string& path);
    void refresh();
    void moveUp();
    void moveDown();
    void enterDirectory();
    void goToParent();
    
    const std::vector<FileEntry>& getEntries() const { return entries; }
    int getSelectedIndex() const { return selected_index; }
    std::string getCurrentPath() const { return current_path; }
    std::string getSelectedFile() const;
    
private:
    void scanDirectory();
    bool isSidFile(const std::string& filename);
    
    std::string current_path;
    std::vector<FileEntry> entries;
    int selected_index;
};
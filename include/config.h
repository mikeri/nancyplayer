#pragma once

#include <string>
#include <map>
#include <vector>
#include <filesystem>

struct ColorPair {
    int fg = 15; // Default foreground: bright white
    int bg = 0;  // Default background: black
    
    ColorPair() = default;
    ColorPair(int fg, int bg) : fg(fg), bg(bg) {}
};

struct Theme {
    ColorPair top_bar;
    ColorPair status_bar;
    ColorPair bottom_bar;
    ColorPair header;      // Text before colon
    ColorPair colon;       // The colon character
    ColorPair value;       // Text after colon
    ColorPair prefix_sid;  // sid prefix
    ColorPair prefix_dir;  // dir prefix
    ColorPair sid_file;    // SID file name
    ColorPair dir_name;    // Directory name
    ColorPair selected_dir;// Selected directory
    ColorPair selected_sid;// Selected SID file
    ColorPair separator;   // Vertical separator between left and right panels
    
    Theme() {
        // All defaults are fg=15, bg=0 (bright white on black)
        top_bar = ColorPair(15, 0);
        status_bar = ColorPair(15, 0);
        bottom_bar = ColorPair(15, 0);
        header = ColorPair(15, 0);
        colon = ColorPair(15, 0);
        value = ColorPair(15, 0);
        prefix_sid = ColorPair(15, 0);
        prefix_dir = ColorPair(15, 0);
        sid_file = ColorPair(15, 0);
        dir_name = ColorPair(15, 0);
        // Selected items use inverse colors (black on bright white)
        selected_dir = ColorPair(0, 15);
        selected_sid = ColorPair(0, 15);
        separator = ColorPair(15, 0);
    }
};

class Config {
public:
    Config();
    
    bool loadConfig();
    bool loadTheme(const std::string& theme_name);
    void createDefaultConfig();
    void createExampleThemes();
    
    const Theme& getCurrentTheme() const { return current_theme; }
    std::vector<std::string> getAvailableThemes() const;
    
    std::string getConfigDir() const { return config_dir; }
    std::string getThemesDir() const { return themes_dir; }
    std::string getHvscRoot() const { return hvsc_root; }
    std::string getRelativeToHvsc(const std::string& path) const;
    
private:
    void initializeDirectories();
    bool parseThemeFile(const std::string& theme_file_path, Theme& theme);
    void writeThemeFile(const std::string& theme_file_path, const Theme& theme);
    ColorPair parseColorPair(const std::string& value);
    std::string colorPairToString(const ColorPair& cp);
    
    std::string config_dir;
    std::string themes_dir;
    std::string config_file;
    std::string hvsc_root;
    Theme current_theme;
    std::string current_theme_name;
};
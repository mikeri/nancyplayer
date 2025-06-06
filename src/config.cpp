#include "config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

Config::Config() : current_theme_name("default") {
    initializeDirectories();
    
    // Set default HVSC root to ~/Music/C64Music
    const char* home = std::getenv("HOME");
    if (home && *home) {
        hvsc_root = std::string(home) + "/Music/C64Music";
    } else {
        hvsc_root = "./Music/C64Music";  // Fallback
    }
}

void Config::initializeDirectories() {
    // Follow XDG Base Directory specification
    const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
    
    if (xdg_config_home && *xdg_config_home) {
        config_dir = std::string(xdg_config_home) + "/nancyplayer";
    } else {
        const char* home = std::getenv("HOME");
        if (home && *home) {
            config_dir = std::string(home) + "/.config/nancyplayer";
        } else {
            config_dir = "./config";  // Fallback
        }
    }
    
    themes_dir = config_dir + "/themes";
    config_file = config_dir + "/config";
    
    // Create directories if they don't exist
    try {
        std::filesystem::create_directories(config_dir);
        std::filesystem::create_directories(themes_dir);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Warning: Could not create config directories: " << e.what() << std::endl;
    }
}

bool Config::loadConfig() {
    createDefaultConfig();
    createExampleThemes();
    
    // Try to load main config file
    std::ifstream file(config_file);
    if (!file.is_open()) {
        // Create default config with proper HVSC root
        std::ofstream out_file(config_file);
        if (out_file.is_open()) {
            out_file << "theme=default\n";
            out_file << "hvsc_root=" << hvsc_root << "\n";
            out_file.close();
        }
        return loadTheme("default");
    }
    
    std::string line;
    std::string theme_name = "default";
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t equals_pos = line.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = line.substr(0, equals_pos);
            std::string value = line.substr(equals_pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (key == "theme") {
                theme_name = value;
            } else if (key == "hvsc_root") {
                hvsc_root = value;
            }
        }
    }
    
    return loadTheme(theme_name);
}

bool Config::loadTheme(const std::string& theme_name) {
    current_theme_name = theme_name;
    current_theme = Theme(); // Reset to defaults
    
    std::string theme_file = themes_dir + "/" + theme_name + ".theme";
    
    if (!std::filesystem::exists(theme_file)) {
        std::cerr << "Theme file not found: " << theme_file << std::endl;
        return false;
    }
    
    return parseThemeFile(theme_file, current_theme);
}

bool Config::parseThemeFile(const std::string& theme_file_path, Theme& theme) {
    std::ifstream file(theme_file_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t equals_pos = line.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = line.substr(0, equals_pos);
            std::string value = line.substr(equals_pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            ColorPair cp = parseColorPair(value);
            
            if (key == "top_bar") theme.top_bar = cp;
            else if (key == "status_bar") theme.status_bar = cp;
            else if (key == "bottom_bar") theme.bottom_bar = cp;
            else if (key == "header") theme.header = cp;
            else if (key == "colon") theme.colon = cp;
            else if (key == "value") theme.value = cp;
            else if (key == "prefix_sid") theme.prefix_sid = cp;
            else if (key == "prefix_dir") theme.prefix_dir = cp;
            else if (key == "sid_file") theme.sid_file = cp;
            else if (key == "dir_name") theme.dir_name = cp;
            else if (key == "selected_dir") theme.selected_dir = cp;
            else if (key == "selected_sid") theme.selected_sid = cp;
            else if (key == "separator") theme.separator = cp;
        }
    }
    
    return true;
}

ColorPair Config::parseColorPair(const std::string& value) {
    // Format: "fg,bg" or "fg" (bg defaults to 0)
    size_t comma_pos = value.find(',');
    
    if (comma_pos != std::string::npos) {
        int fg = std::stoi(value.substr(0, comma_pos));
        int bg = std::stoi(value.substr(comma_pos + 1));
        return ColorPair(fg, bg);
    } else {
        int fg = std::stoi(value);
        return ColorPair(fg, 0);
    }
}

std::string Config::colorPairToString(const ColorPair& cp) {
    return std::to_string(cp.fg) + "," + std::to_string(cp.bg);
}

void Config::writeThemeFile(const std::string& theme_file_path, const Theme& theme) {
    std::ofstream file(theme_file_path);
    if (!file.is_open()) {
        return;
    }
    
    file << "# Nancy SID Player Theme File\n";
    file << "# Format: element=foreground,background\n";
    file << "# Colors: 0-255 (256-color mode)\n";
    file << "# Default: foreground=1 (white), background=0 (black)\n\n";
    
    file << "top_bar=" << colorPairToString(theme.top_bar) << "\n";
    file << "status_bar=" << colorPairToString(theme.status_bar) << "\n";
    file << "bottom_bar=" << colorPairToString(theme.bottom_bar) << "\n";
    file << "header=" << colorPairToString(theme.header) << "\n";
    file << "colon=" << colorPairToString(theme.colon) << "\n";
    file << "value=" << colorPairToString(theme.value) << "\n";
    file << "prefix_sid=" << colorPairToString(theme.prefix_sid) << "\n";
    file << "prefix_dir=" << colorPairToString(theme.prefix_dir) << "\n";
    file << "sid_file=" << colorPairToString(theme.sid_file) << "\n";
    file << "dir_name=" << colorPairToString(theme.dir_name) << "\n";
    file << "selected_dir=" << colorPairToString(theme.selected_dir) << "\n";
    file << "selected_sid=" << colorPairToString(theme.selected_sid) << "\n";
    file << "separator=" << colorPairToString(theme.separator) << "\n";
}

void Config::createDefaultConfig() {
    std::string default_theme_file = themes_dir + "/default.theme";
    
    if (!std::filesystem::exists(default_theme_file)) {
        Theme default_theme;
        writeThemeFile(default_theme_file, default_theme);
    }
}

void Config::createExampleThemes() {
    // Dark theme
    {
        std::string dark_theme_file = themes_dir + "/dark.theme";
        if (!std::filesystem::exists(dark_theme_file)) {
            Theme dark_theme;
            dark_theme.top_bar = ColorPair(15, 236);      // White on dark gray
            dark_theme.status_bar = ColorPair(250, 238);  // Light gray on slightly lighter gray
            dark_theme.bottom_bar = ColorPair(246, 240);  // Gray on light gray
            dark_theme.header = ColorPair(14, 0);         // Bright yellow
            dark_theme.colon = ColorPair(8, 0);           // Dark gray
            dark_theme.value = ColorPair(15, 0);          // White
            dark_theme.prefix_sid = ColorPair(10, 0);     // Bright green
            dark_theme.prefix_dir = ColorPair(12, 0);     // Bright blue
            dark_theme.sid_file = ColorPair(11, 0);       // Bright yellow
            dark_theme.dir_name = ColorPair(14, 0);       // Bright cyan
            dark_theme.selected_dir = ColorPair(236, 14); // Dark gray on bright cyan (inverse)
            dark_theme.selected_sid = ColorPair(238, 11); // Light gray on bright yellow (inverse)
            dark_theme.separator = ColorPair(242, 0);     // Gray separator
            writeThemeFile(dark_theme_file, dark_theme);
        }
    }
    
    // Light theme
    {
        std::string light_theme_file = themes_dir + "/light.theme";
        if (!std::filesystem::exists(light_theme_file)) {
            Theme light_theme;
            light_theme.top_bar = ColorPair(0, 255);      // Black on white
            light_theme.status_bar = ColorPair(236, 250); // Dark gray on light gray
            light_theme.bottom_bar = ColorPair(240, 253); // Gray on very light gray
            light_theme.header = ColorPair(4, 255);       // Blue
            light_theme.colon = ColorPair(8, 255);        // Dark gray
            light_theme.value = ColorPair(0, 255);        // Black
            light_theme.prefix_sid = ColorPair(2, 255);   // Green
            light_theme.prefix_dir = ColorPair(4, 255);   // Blue
            light_theme.sid_file = ColorPair(5, 255);     // Magenta
            light_theme.dir_name = ColorPair(6, 255);     // Cyan
            light_theme.selected_dir = ColorPair(253, 6); // Very light gray on cyan (inverse)
            light_theme.selected_sid = ColorPair(253, 5); // Very light gray on magenta (inverse)
            light_theme.separator = ColorPair(8, 255);    // Dark gray separator on white
            writeThemeFile(light_theme_file, light_theme);
        }
    }
    
    // Synthwave theme
    {
        std::string synthwave_theme_file = themes_dir + "/synthwave.theme";
        if (!std::filesystem::exists(synthwave_theme_file)) {
            Theme synthwave_theme;
            synthwave_theme.top_bar = ColorPair(201, 53);    // Bright magenta on dark magenta
            synthwave_theme.status_bar = ColorPair(51, 17);  // Cyan on dark blue
            synthwave_theme.bottom_bar = ColorPair(201, 53); // Bright magenta on dark magenta
            synthwave_theme.header = ColorPair(201, 0);      // Bright magenta
            synthwave_theme.colon = ColorPair(51, 0);        // Cyan
            synthwave_theme.value = ColorPair(15, 0);        // White
            synthwave_theme.prefix_sid = ColorPair(201, 0);  // Bright magenta
            synthwave_theme.prefix_dir = ColorPair(51, 0);   // Cyan
            synthwave_theme.sid_file = ColorPair(208, 0);    // Orange
            synthwave_theme.dir_name = ColorPair(51, 0);     // Cyan
            synthwave_theme.selected_dir = ColorPair(17, 51); // Dark blue on cyan (inverse)
            synthwave_theme.selected_sid = ColorPair(53, 201);// Dark magenta on bright magenta (inverse)
            synthwave_theme.separator = ColorPair(93, 0);     // Bright purple separator
            writeThemeFile(synthwave_theme_file, synthwave_theme);
        }
    }
    
    // Retro green theme
    {
        std::string retro_theme_file = themes_dir + "/retro.theme";
        if (!std::filesystem::exists(retro_theme_file)) {
            Theme retro_theme;
            retro_theme.top_bar = ColorPair(46, 22);      // Bright green on dark green
            retro_theme.status_bar = ColorPair(82, 0);    // Lime green
            retro_theme.bottom_bar = ColorPair(46, 22);   // Bright green on dark green
            retro_theme.header = ColorPair(82, 0);        // Lime green
            retro_theme.colon = ColorPair(40, 0);         // Green
            retro_theme.value = ColorPair(46, 0);         // Bright green
            retro_theme.prefix_sid = ColorPair(82, 0);    // Lime green
            retro_theme.prefix_dir = ColorPair(40, 0);    // Green
            retro_theme.sid_file = ColorPair(118, 0);     // Light green
            retro_theme.dir_name = ColorPair(82, 0);      // Lime green
            retro_theme.selected_dir = ColorPair(22, 82);  // Dark green on lime green (inverse)
            retro_theme.selected_sid = ColorPair(22, 46);  // Dark green on bright green (inverse)
            retro_theme.separator = ColorPair(34, 0);      // Forest green separator
            writeThemeFile(retro_theme_file, retro_theme);
        }
    }
    
    // Bumblebee theme (based on cmus theme)
    {
        std::string bumblebee_theme_file = themes_dir + "/bumblebee.theme";
        if (!std::filesystem::exists(bumblebee_theme_file)) {
            Theme bumblebee_theme;
            bumblebee_theme.top_bar = ColorPair(252, 236);      // Light gray on dark gray (win_title)
            bumblebee_theme.status_bar = ColorPair(245, 235);   // Gray on dark gray (statusline)
            bumblebee_theme.bottom_bar = ColorPair(229, 236);   // Yellow on dark gray (titleline)
            bumblebee_theme.header = ColorPair(172, 0);         // Orange/info color
            bumblebee_theme.colon = ColorPair(236, 0);          // Dark gray separator
            bumblebee_theme.value = ColorPair(246, 0);          // Default text (win_fg)
            bumblebee_theme.prefix_sid = ColorPair(184, 0);     // Currently playing color
            bumblebee_theme.prefix_dir = ColorPair(229, 0);     // Directory color
            bumblebee_theme.sid_file = ColorPair(184, 0);       // Currently playing color
            bumblebee_theme.dir_name = ColorPair(229, 0);       // Directory color
            bumblebee_theme.selected_dir = ColorPair(229, 58);  // Active selection (yellow on dark green)
            bumblebee_theme.selected_sid = ColorPair(226, 58);  // Current playing selection (bright yellow on dark green)
            bumblebee_theme.separator = ColorPair(236, 0);      // Dark gray separator
            writeThemeFile(bumblebee_theme_file, bumblebee_theme);
        }
    }
}

std::vector<std::string> Config::getAvailableThemes() const {
    std::vector<std::string> themes;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(themes_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".theme") {
                std::string theme_name = entry.path().stem().string();
                themes.push_back(theme_name);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error reading themes directory: " << e.what() << std::endl;
    }
    
    std::sort(themes.begin(), themes.end());
    return themes;
}

std::string Config::getRelativeToHvsc(const std::string& path) const {
    try {
        std::filesystem::path abs_path = std::filesystem::canonical(path);
        std::filesystem::path hvsc_path = std::filesystem::canonical(hvsc_root);
        
        std::filesystem::path rel_path = std::filesystem::relative(abs_path, hvsc_path);
        return rel_path.string();
    } catch (const std::filesystem::filesystem_error& e) {
        // If we can't get relative path, just return the original
        return path;
    }
}

bool Config::validateHvscRoot() const {
    if (!std::filesystem::exists(hvsc_root)) {
        return false;
    }
    
    if (!std::filesystem::is_directory(hvsc_root)) {
        return false;
    }
    
    // Check for common HVSC subdirectories to verify it's actually HVSC
    std::vector<std::string> hvsc_indicators = {
        "DEMOS",
        "GAMES", 
        "MUSICIANS",
        "DOCUMENTS"
    };
    
    int found_indicators = 0;
    for (const auto& indicator : hvsc_indicators) {
        std::string indicator_path = hvsc_root + "/" + indicator;
        if (std::filesystem::exists(indicator_path) && std::filesystem::is_directory(indicator_path)) {
            found_indicators++;
        }
    }
    
    // Require at least 2 of the common HVSC directories to be present
    return found_indicators >= 2;
}
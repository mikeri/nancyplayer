#pragma once

#include <ncurses.h>
#include <string>
#include <vector>
#include <memory>
#include <map>

class Player;
class FileBrowser;
class StilReader;
class Search;
class Config;

class TUI {
public:
    TUI();
    ~TUI();
    
    void run();
    void refresh();
    void handleInput();
    void handleResize();
    
private:
    void initWindows();
    void destroyWindows();
    void initColors();
    int getColorPair(int fg, int bg);
    void drawHeader();
    void drawBrowser();
    void drawStilInfo();
    void drawStatus();
    void drawHelp();
    void drawSearchResults();
    void drawSeparator();
    void resetScrollPositions();
    void createSearchWindow();
    void destroySearchWindow();
    std::string cropTextLeft(const std::string& text, int max_width);
    
    WINDOW* header_win;
    WINDOW* browser_win;
    WINDOW* separator_win;
    WINDOW* stil_win;
    WINDOW* status_win;
    WINDOW* help_win;
    WINDOW* search_win;
    
    std::unique_ptr<Player> player;
    std::unique_ptr<FileBrowser> browser;
    std::unique_ptr<StilReader> stil_reader;
    std::unique_ptr<Search> search;
    std::unique_ptr<Config> config;
    
    bool running;
    bool search_mode;
    std::string search_query;
    std::vector<struct SongEntry> search_results;
    int search_selected;
    int screen_height;
    int screen_width;
    std::map<std::pair<int, int>, int> color_pair_cache;
    int next_color_pair;
    int browser_start_line;
    int search_start_line;
};
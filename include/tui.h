#pragma once

#include <ncurses.h>
#include <string>
#include <vector>
#include <memory>

class Player;
class FileBrowser;
class StilReader;
class Search;

class TUI {
public:
    TUI();
    ~TUI();
    
    void run();
    void refresh();
    void handleInput();
    
private:
    void initWindows();
    void destroyWindows();
    void drawHeader();
    void drawBrowser();
    void drawStilInfo();
    void drawStatus();
    void drawHelp();
    void drawSearchResults();
    
    WINDOW* header_win;
    WINDOW* browser_win;
    WINDOW* stil_win;
    WINDOW* status_win;
    WINDOW* help_win;
    
    std::unique_ptr<Player> player;
    std::unique_ptr<FileBrowser> browser;
    std::unique_ptr<StilReader> stil_reader;
    std::unique_ptr<Search> search;
    
    bool running;
    bool search_mode;
    std::string search_query;
    std::vector<struct SongEntry> search_results;
    int search_selected;
    int screen_height;
    int screen_width;
};
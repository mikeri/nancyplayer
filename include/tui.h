#pragma once

#include <ncurses.h>
#include <string>
#include <vector>
#include <memory>

class Player;
class FileBrowser;
class StilReader;

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
    void drawPlayer();
    void drawBrowser();
    void drawStilInfo();
    void drawStatus();
    void drawHelp();
    
    WINDOW* header_win;
    WINDOW* player_win;
    WINDOW* browser_win;
    WINDOW* stil_win;
    WINDOW* status_win;
    WINDOW* help_win;
    
    std::unique_ptr<Player> player;
    std::unique_ptr<FileBrowser> browser;
    std::unique_ptr<StilReader> stil_reader;
    
    bool running;
    int screen_height;
    int screen_width;
};
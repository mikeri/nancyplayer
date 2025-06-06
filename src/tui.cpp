#include "tui.h"
#include "player.h"
#include "file_browser.h"
#include "stil_reader.h"
#include <algorithm>
#include <iostream>

TUI::TUI() : running(false) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(100);
    
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    
    getmaxyx(stdscr, screen_height, screen_width);
    
    player = std::make_unique<Player>();
    browser = std::make_unique<FileBrowser>();
    stil_reader = std::make_unique<StilReader>();
    
    initWindows();
}

TUI::~TUI() {
    destroyWindows();
    endwin();
}

void TUI::initWindows() {
    int header_height = 3;
    int status_height = 2;
    int help_height = 3;
    int player_height = 8;
    int remaining_height = screen_height - header_height - status_height - help_height - player_height;
    int browser_width = screen_width / 2;
    
    if (remaining_height < 1) remaining_height = 1;
    
    header_win = newwin(header_height, screen_width, 0, 0);
    player_win = newwin(player_height, screen_width, header_height, 0);
    browser_win = newwin(remaining_height, browser_width, header_height + player_height, 0);
    stil_win = newwin(remaining_height, screen_width - browser_width, header_height + player_height, browser_width);
    status_win = newwin(status_height, screen_width, screen_height - help_height - status_height, 0);
    help_win = newwin(help_height, screen_width, screen_height - help_height, 0);
    
    wbkgd(header_win, COLOR_PAIR(1));
    wbkgd(status_win, COLOR_PAIR(2));
    
    keypad(header_win, TRUE);
    keypad(player_win, TRUE);
    keypad(browser_win, TRUE);
    keypad(stil_win, TRUE);
    keypad(status_win, TRUE);
    keypad(help_win, TRUE);
}

void TUI::destroyWindows() {
    if (header_win) delwin(header_win);
    if (player_win) delwin(player_win);
    if (browser_win) delwin(browser_win);
    if (stil_win) delwin(stil_win);
    if (status_win) delwin(status_win);
    if (help_win) delwin(help_win);
}

void TUI::run() {
    if (screen_height < 20 || screen_width < 60) {
        endwin();
        std::cerr << "Terminal too small. Need at least 60x20." << std::endl;
        return;
    }
    
    running = true;
    browser->setDirectory(".");
    stil_reader->loadDatabase(".");
    
    refresh();
    
    while (running) {
        handleInput();
        refresh();
    }
}

void TUI::refresh() {
    drawHeader();
    drawPlayer();
    drawBrowser();
    drawStilInfo();
    drawStatus();
    drawHelp();
    
    doupdate();
}

void TUI::drawHeader() {
    werase(header_win);
    box(header_win, 0, 0);
    
    mvwprintw(header_win, 1, 2, "Nancy SID Player");
    mvwprintw(header_win, 1, screen_width - 20, "Press 'q' to quit");
    
    wnoutrefresh(header_win);
}

void TUI::drawPlayer() {
    werase(player_win);
    box(player_win, 0, 0);
    
    mvwprintw(player_win, 0, 2, " Player ");
    
    if (!player->getCurrentFile().empty()) {
        mvwprintw(player_win, 1, 2, "File: %s", player->getCurrentFile().c_str());
        mvwprintw(player_win, 2, 2, "Title: %s", player->getTitle().c_str());
        mvwprintw(player_win, 3, 2, "Author: %s", player->getAuthor().c_str());
        mvwprintw(player_win, 4, 2, "Copyright: %s", player->getCopyright().c_str());
        mvwprintw(player_win, 5, 2, "Track: %d/%d", player->getCurrentTrack(), player->getTrackCount());
        
        int minutes = player->getPlayTime() / 60;
        int seconds = player->getPlayTime() % 60;
        mvwprintw(player_win, 6, 2, "Time: %02d:%02d", minutes, seconds);
        
        std::string status = player->isPlaying() ? (player->isPaused() ? "PAUSED" : "PLAYING") : "STOPPED";
        wattron(player_win, COLOR_PAIR(player->isPlaying() ? 4 : 5));
        mvwprintw(player_win, 6, screen_width - 15, "[%s]", status.c_str());
        wattroff(player_win, COLOR_PAIR(player->isPlaying() ? 4 : 5));
    } else {
        mvwprintw(player_win, 3, 2, "No file loaded");
    }
    
    wnoutrefresh(player_win);
}

void TUI::drawBrowser() {
    werase(browser_win);
    box(browser_win, 0, 0);
    
    wattron(browser_win, COLOR_PAIR(3));
    mvwprintw(browser_win, 0, 2, " File Browser ");
    wattroff(browser_win, COLOR_PAIR(3));
    
    int height, width;
    getmaxyx(browser_win, height, width);
    
    mvwprintw(browser_win, 1, 2, "Path: %s", browser->getCurrentPath().c_str());
    
    const auto& entries = browser->getEntries();
    int selected = browser->getSelectedIndex();
    int start_line = std::max(0, selected - (height - 5));
    
    for (int i = 0; i < std::min((int)entries.size(), height - 4); i++) {
        int entry_idx = start_line + i;
        if (entry_idx >= entries.size()) break;
        
        const auto& entry = entries[entry_idx];
        int line = i + 3;
        
        if (entry_idx == selected) {
            wattron(browser_win, A_REVERSE);
        }
        
        if (entry.is_directory) {
            mvwprintw(browser_win, line, 2, "[DIR] %s", entry.name.c_str());
        } else if (entry.is_sid_file) {
            mvwprintw(browser_win, line, 2, "[SID] %s", entry.name.c_str());
        } else {
            mvwprintw(browser_win, line, 2, "      %s", entry.name.c_str());
        }
        
        if (entry_idx == selected) {
            wattroff(browser_win, A_REVERSE);
        }
    }
    
    wnoutrefresh(browser_win);
}

void TUI::drawStilInfo() {
    werase(stil_win);
    box(stil_win, 0, 0);
    
    mvwprintw(stil_win, 0, 2, " STIL Information ");
    
    int height, width;
    getmaxyx(stil_win, height, width);
    
    std::string selected_file = browser->getSelectedFile();
    
    if (!selected_file.empty() && stil_reader->hasInfo(selected_file)) {
        StilEntry info = stil_reader->getInfo(selected_file);
        
        int line = 2;
        if (!info.title.empty()) {
            mvwprintw(stil_win, line++, 2, "Title: %s", info.title.c_str());
        }
        if (!info.artist.empty()) {
            mvwprintw(stil_win, line++, 2, "Artist: %s", info.artist.c_str());
        }
        if (!info.copyright.empty()) {
            mvwprintw(stil_win, line++, 2, "Copyright: %s", info.copyright.c_str());
        }
        
        if (!info.comment.empty()) {
            line++;
            mvwprintw(stil_win, line++, 2, "Comment:");
            
            // Word wrap the comment
            std::string comment = info.comment;
            int max_width = width - 4;
            size_t pos = 0;
            while (pos < comment.length() && line < height - 2) {
                size_t end = std::min(pos + max_width, comment.length());
                if (end < comment.length()) {
                    // Find last space
                    while (end > pos && comment[end] != ' ') end--;
                    if (end == pos) end = pos + max_width; // Force break if no space
                }
                
                std::string line_text = comment.substr(pos, end - pos);
                mvwprintw(stil_win, line++, 4, "%s", line_text.c_str());
                pos = end + 1; // Skip the space
            }
        }
        
        if (!info.subtune_info.empty()) {
            line++;
            mvwprintw(stil_win, line++, 2, "Subtunes:");
            for (size_t i = 0; i < info.subtune_info.size() && line < height - 2; i++) {
                mvwprintw(stil_win, line++, 4, "%zu: %s", i + 1, info.subtune_info[i].c_str());
            }
        }
    } else {
        int line = 2;
        mvwprintw(stil_win, line++, 2, "No STIL information");
        if (!selected_file.empty()) {
            mvwprintw(stil_win, line++, 2, "for this file");
        } else {
            mvwprintw(stil_win, line++, 2, "No file selected");
        }
        mvwprintw(stil_win, line++, 2, "DB: %zu entries", stil_reader->getEntryCount());
    }
    
    wnoutrefresh(stil_win);
}

void TUI::drawStatus() {
    werase(status_win);
    
    mvwprintw(status_win, 0, 2, "Files: %zu | Path: %s", 
              browser->getEntries().size(), browser->getCurrentPath().c_str());
    
    wnoutrefresh(status_win);
}

void TUI::drawHelp() {
    werase(help_win);
    box(help_win, 0, 0);
    
    mvwprintw(help_win, 1, 2, "j/k: Up/Down | h: Parent dir | l/ENTER: Play/Enter dir | SPACE: Pause | s: Stop | n/p: Next/Prev track | q: Quit");
    
    wnoutrefresh(help_win);
}

void TUI::handleInput() {
    int ch = getch();
    
    switch (ch) {
        case 'q':
        case 'Q':
            running = false;
            break;
            
        // Vim-style navigation
        case 'j':
        case KEY_DOWN:
            browser->moveDown();
            break;
            
        case 'k':
        case KEY_UP:
            browser->moveUp();
            break;
            
        case 'h':
        case KEY_BACKSPACE:
        case 127:
            browser->goToParent();
            break;
            
        case 'l':
        case '\n':
        case '\r':
        case KEY_ENTER:
            {
                std::string selected = browser->getSelectedFile();
                if (!selected.empty()) {
                    if (std::filesystem::is_directory(selected)) {
                        browser->enterDirectory();
                    } else {
                        player->loadFile(selected);
                        player->play();
                    }
                }
            }
            break;
            
        case ' ':
            if (player->isPlaying()) {
                player->pause();
            } else if (!player->getCurrentFile().empty()) {
                player->play();
            }
            break;
            
        case 's':
        case 'S':
            player->stop();
            break;
            
        case 'n':
        case 'N':
            player->nextTrack();
            break;
            
        case 'p':
        case 'P':
            player->prevTrack();
            break;
    }
}

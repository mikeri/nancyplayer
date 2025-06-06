#include "tui.h"
#include "player.h"
#include "file_browser.h"
#include "stil_reader.h"
#include "search.h"
#include <algorithm>
#include <iostream>

TUI::TUI() : running(false), search_mode(false), search_selected(0) {
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
    search = std::make_unique<Search>();
    
    initWindows();
}

TUI::~TUI() {
    destroyWindows();
    endwin();
}

void TUI::initWindows() {
    int header_height = 1;
    int status_height = 1;
    int help_height = 1;
    int main_height = screen_height - header_height - status_height - help_height;
    int browser_width = screen_width / 2;
    
    if (main_height < 1) main_height = 1;
    
    header_win = newwin(header_height, screen_width, 0, 0);
    browser_win = newwin(main_height, browser_width, header_height, 0);
    stil_win = newwin(main_height, screen_width - browser_width, header_height, browser_width);
    status_win = newwin(status_height, screen_width, screen_height - help_height - status_height, 0);
    help_win = newwin(help_height, screen_width, screen_height - help_height, 0);
    
    wbkgd(header_win, COLOR_PAIR(1));
    wbkgd(status_win, COLOR_PAIR(2));
    
    keypad(header_win, TRUE);
    keypad(browser_win, TRUE);
    keypad(stil_win, TRUE);
    keypad(status_win, TRUE);
    keypad(help_win, TRUE);
}

void TUI::destroyWindows() {
    if (header_win) delwin(header_win);
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
    search->loadDatabase(".");
    
    refresh();
    
    while (running) {
        handleInput();
        refresh();
    }
}

void TUI::refresh() {
    drawHeader();
    if (search_mode) {
        drawSearchResults();
    } else {
        drawBrowser();
    }
    drawStilInfo();
    drawStatus();
    drawHelp();
    
    doupdate();
}

void TUI::drawHeader() {
    werase(header_win);
    
    mvwprintw(header_win, 0, 0, "Nancy SID Player");
    
    wnoutrefresh(header_win);
}


void TUI::drawBrowser() {
    werase(browser_win);
    
    int height, width;
    getmaxyx(browser_win, height, width);
    
    mvwprintw(browser_win, 0, 0, "Path: %s", browser->getCurrentPath().c_str());
    
    const auto& entries = browser->getEntries();
    int selected = browser->getSelectedIndex();
    int start_line = std::max(0, selected - (height - 3));
    
    for (int i = 0; i < std::min((int)entries.size(), height - 2); i++) {
        int entry_idx = start_line + i;
        if (entry_idx >= entries.size()) break;
        
        const auto& entry = entries[entry_idx];
        int line = i + 2;
        
        if (entry_idx == selected) {
            wattron(browser_win, A_REVERSE);
        }
        
        if (entry.is_directory) {
            mvwprintw(browser_win, line, 0, "[DIR] %s", entry.name.c_str());
        } else if (entry.is_sid_file) {
            mvwprintw(browser_win, line, 0, "[SID] %s", entry.name.c_str());
        } else {
            mvwprintw(browser_win, line, 0, "      %s", entry.name.c_str());
        }
        
        if (entry_idx == selected) {
            wattroff(browser_win, A_REVERSE);
        }
    }
    
    wnoutrefresh(browser_win);
}

void TUI::drawStilInfo() {
    werase(stil_win);
    
    int height, width;
    getmaxyx(stil_win, height, width);
    
    int line = 0;
    
    // Player Information Section
    if (!player->getCurrentFile().empty()) {
        mvwprintw(stil_win, line++, 0, "File: %s", player->getCurrentFile().c_str());
        mvwprintw(stil_win, line++, 0, "Title: %s", player->getTitle().c_str());
        mvwprintw(stil_win, line++, 0, "Author: %s", player->getAuthor().c_str());
        mvwprintw(stil_win, line++, 0, "Copyright: %s", player->getCopyright().c_str());
        mvwprintw(stil_win, line++, 0, "Track: %d/%d", player->getCurrentTrack(), player->getTrackCount());
        
        int minutes = player->getPlayTime() / 60;
        int seconds = player->getPlayTime() % 60;
        
        // Get song length from search database
        int song_length = search->getSongLength(player->getCurrentFile(), player->getCurrentTrack());
        if (song_length > 0) {
            int length_minutes = song_length / 60;
            int length_seconds = song_length % 60;
            mvwprintw(stil_win, line++, 0, "Time: %02d:%02d / %02d:%02d", minutes, seconds, length_minutes, length_seconds);
        } else {
            mvwprintw(stil_win, line++, 0, "Time: %02d:%02d", minutes, seconds);
        }
        
        std::string status = player->isPlaying() ? (player->isPaused() ? "PAUSED" : "PLAYING") : "STOPPED";
        wattron(stil_win, COLOR_PAIR(player->isPlaying() ? 4 : 5));
        mvwprintw(stil_win, line++, 0, "Status: [%s]", status.c_str());
        wattroff(stil_win, COLOR_PAIR(player->isPlaying() ? 4 : 5));
        
        line++; // Empty line separator
    } else {
        mvwprintw(stil_win, line++, 0, "No file loaded");
        line++; // Empty line separator
    }
    
    // STIL Information Section
    std::string selected_file = browser->getSelectedFile();
    
    if (!selected_file.empty() && stil_reader->hasInfo(selected_file)) {
        StilEntry info = stil_reader->getInfo(selected_file);
        
        mvwprintw(stil_win, line++, 0, "STIL Information:");
        line++;
        
        if (!info.title.empty()) {
            mvwprintw(stil_win, line++, 0, "Title: %s", info.title.c_str());
        }
        if (!info.artist.empty()) {
            mvwprintw(stil_win, line++, 0, "Artist: %s", info.artist.c_str());
        }
        if (!info.copyright.empty()) {
            mvwprintw(stil_win, line++, 0, "Copyright: %s", info.copyright.c_str());
        }
        
        if (!info.comment.empty()) {
            line++;
            mvwprintw(stil_win, line++, 0, "Comment:");
            
            // Word wrap the comment
            std::string comment = info.comment;
            int max_width = width - 2;
            size_t pos = 0;
            while (pos < comment.length() && line < height - 1) {
                size_t end = std::min(pos + max_width, comment.length());
                if (end < comment.length()) {
                    // Find last space
                    while (end > pos && comment[end] != ' ') end--;
                    if (end == pos) end = pos + max_width; // Force break if no space
                }
                
                std::string line_text = comment.substr(pos, end - pos);
                mvwprintw(stil_win, line++, 2, "%s", line_text.c_str());
                pos = end + 1; // Skip the space
            }
        }
        
        if (!info.subtune_info.empty()) {
            line++;
            mvwprintw(stil_win, line++, 0, "Subtunes:");
            for (size_t i = 0; i < info.subtune_info.size() && line < height - 1; i++) {
                mvwprintw(stil_win, line++, 2, "%zu: %s", i + 1, info.subtune_info[i].c_str());
            }
        }
    } else {
        mvwprintw(stil_win, line++, 0, "STIL Information:");
        line++;
        mvwprintw(stil_win, line++, 0, "No STIL information available");
        mvwprintw(stil_win, line++, 0, "DB: %zu entries", stil_reader->getEntryCount());
    }
    
    wnoutrefresh(stil_win);
}


void TUI::drawStatus() {
    werase(status_win);
    
    if (search_mode) {
        mvwprintw(status_win, 0, 0, "Search: %s", search_query.c_str());
    } else {
        mvwprintw(status_win, 0, 0, "Files: %zu | Path: %s", 
                  browser->getEntries().size(), browser->getCurrentPath().c_str());
    }
    
    wnoutrefresh(status_win);
}

void TUI::drawHelp() {
    werase(help_win);
    
    if (search_mode) {
        mvwprintw(help_win, 0, 0, "j/k: Up/Down | ENTER: Play | ESC: Exit search | Type to search | SPACE: Pause | s: Stop | J/K: Next/Prev track | q: Quit");
    } else {
        mvwprintw(help_win, 0, 0, "j/k: Up/Down | h: Parent dir | l/ENTER: Play/Enter dir | /: Search | SPACE: Pause | s: Stop | J/K: Next/Prev track | q: Quit");
    }
    
    wnoutrefresh(help_win);
}

void TUI::drawSearchResults() {
    werase(browser_win);
    
    int height, width;
    getmaxyx(browser_win, height, width);
    
    mvwprintw(browser_win, 0, 0, "Search results (%zu found):", search_results.size());
    
    if (search_results.empty()) {
        mvwprintw(browser_win, 2, 0, "No results found");
        wnoutrefresh(browser_win);
        return;
    }
    
    int start_line = std::max(0, search_selected - (height - 4));
    
    for (int i = 0; i < std::min((int)search_results.size(), height - 2); i++) {
        int entry_idx = start_line + i;
        if (entry_idx >= (int)search_results.size()) break;
        
        const auto& entry = search_results[entry_idx];
        int line = i + 2;
        
        if (entry_idx == search_selected) {
            wattron(browser_win, A_REVERSE);
        }
        
        std::string display_text = entry.getDisplayName();
        if (display_text.length() > width - 1) {
            display_text = display_text.substr(0, width - 4) + "...";
        }
        
        mvwprintw(browser_win, line, 0, "%s", display_text.c_str());
        
        if (entry_idx == search_selected) {
            wattroff(browser_win, A_REVERSE);
        }
    }
    
    wnoutrefresh(browser_win);
}

void TUI::handleInput() {
    int ch = getch();
    
    if (search_mode) {
        switch (ch) {
            case 27: // ESC
                search_mode = false;
                search_query.clear();
                search_results.clear();
                search_selected = 0;
                break;
                
            case KEY_BACKSPACE:
            case 127:
            case '\b':
                if (!search_query.empty()) {
                    search_query.pop_back();
                    search_results = search->search(search_query);
                    search_selected = 0;
                }
                break;
                
            case '\n':
            case '\r':
            case KEY_ENTER:
                if (!search_results.empty() && search_selected < (int)search_results.size()) {
                    const auto& entry = search_results[search_selected];
                    std::string full_path = entry.path;
                    // Convert HVSC path to absolute path by adding current working directory
                    if (full_path[0] == '/') {
                        full_path = "." + full_path;
                    }
                    player->loadFile(full_path);
                    player->play();
                    search_mode = false;
                    search_query.clear();
                    search_results.clear();
                    search_selected = 0;
                }
                break;
                
            case 'j':
            case KEY_DOWN:
                if (!search_results.empty()) {
                    search_selected = std::min(search_selected + 1, (int)search_results.size() - 1);
                }
                break;
                
            case 'k':
            case KEY_UP:
                if (!search_results.empty()) {
                    search_selected = std::max(search_selected - 1, 0);
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
                
            case 'J':
                player->nextTrack();
                break;
                
            case 'K':
                player->prevTrack();
                break;
                
            case 'q':
            case 'Q':
                running = false;
                break;
                
            default:
                if (ch >= 32 && ch <= 126) { // Printable characters
                    search_query += (char)ch;
                    search_results = search->search(search_query);
                    search_selected = 0;
                }
                break;
        }
    } else {
        switch (ch) {
            case 'q':
            case 'Q':
                running = false;
                break;
                
            case '/':
                search_mode = true;
                search_query.clear();
                search_results.clear();
                search_selected = 0;
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
                
            case 'J':
                player->nextTrack();
                break;
                
            case 'K':
                player->prevTrack();
                break;
        }
    }
}

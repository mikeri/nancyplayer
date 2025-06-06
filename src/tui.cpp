#include "tui.h"
#include "player.h"
#include "file_browser.h"
#include "stil_reader.h"
#include "search.h"
#include "config.h"
#include <algorithm>
#include <iostream>

TUI::TUI() : running(false), search_mode(false), search_selected(0), next_color_pair(1), browser_start_line(0), search_start_line(0) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(100);
    
    initColors();
    
    getmaxyx(stdscr, screen_height, screen_width);
    
    player = std::make_unique<Player>();
    browser = std::make_unique<FileBrowser>();
    stil_reader = std::make_unique<StilReader>();
    search = std::make_unique<Search>();
    config = std::make_unique<Config>();
    
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
    int separator_width = 1;
    int stil_width = screen_width - browser_width - separator_width;
    
    if (main_height < 1) main_height = 1;
    
    header_win = newwin(header_height, screen_width, 0, 0);
    browser_win = newwin(main_height, browser_width, header_height, 0);
    separator_win = newwin(main_height, separator_width, header_height, browser_width);
    stil_win = newwin(main_height, stil_width, header_height, browser_width + separator_width);
    status_win = newwin(status_height, screen_width, screen_height - help_height - status_height, 0);
    help_win = newwin(help_height, screen_width, screen_height - help_height, 0);
    
    const auto& theme = config->getCurrentTheme();
    wbkgd(header_win, COLOR_PAIR(getColorPair(theme.top_bar.fg, theme.top_bar.bg)));
    wbkgd(status_win, COLOR_PAIR(getColorPair(theme.status_bar.fg, theme.status_bar.bg)));
    
    keypad(header_win, TRUE);
    keypad(browser_win, TRUE);
    keypad(separator_win, TRUE);
    keypad(stil_win, TRUE);
    keypad(status_win, TRUE);
    keypad(help_win, TRUE);
}

void TUI::destroyWindows() {
    if (header_win) {
        delwin(header_win);
        header_win = nullptr;
    }
    if (browser_win) {
        delwin(browser_win);
        browser_win = nullptr;
    }
    if (separator_win) {
        delwin(separator_win);
        separator_win = nullptr;
    }
    if (stil_win) {
        delwin(stil_win);
        stil_win = nullptr;
    }
    if (status_win) {
        delwin(status_win);
        status_win = nullptr;
    }
    if (help_win) {
        delwin(help_win);
        help_win = nullptr;
    }
}

void TUI::run() {
    if (screen_height < 20 || screen_width < 60) {
        endwin();
        std::cerr << "Terminal too small. Need at least 60x20." << std::endl;
        return;
    }
    
    running = true;
    config->loadConfig();
    
    // Validate HVSC root directory before starting
    if (!config->validateHvscRoot()) {
        endwin();
        std::cerr << "HVSC directory not found or invalid: " << config->getHvscRoot() << std::endl;
        std::cerr << "Please edit the configuration file: " << config->getConfigDir() << "/config" << std::endl;
        std::cerr << "Set hvsc_root to point to your HVSC collection directory." << std::endl;
        std::cerr << "Example: hvsc_root=/path/to/your/C64Music" << std::endl;
        return;
    }
    
    browser->setDirectory(config->getHvscRoot());
    stil_reader->loadDatabase(config->getHvscRoot());
    search->loadDatabase(config->getHvscRoot());
    
    refresh();
    
    while (running) {
        handleInput();
        handleResize();
        refresh();
    }
}

void TUI::refresh() {
    // Safety check - don't draw if windows aren't initialized
    if (!header_win || !browser_win || !separator_win || !stil_win || !status_win || !help_win) {
        return;
    }
    
    drawHeader();
    if (search_mode) {
        drawSearchResults();
    } else {
        drawBrowser();
    }
    drawSeparator();
    drawStilInfo();
    drawStatus();
    drawHelp();
    
    doupdate();
}

void TUI::drawHeader() {
    werase(header_win);
    
    const auto& theme = config->getCurrentTheme();
    int width = getmaxx(header_win);
    
    // Fill entire line with background color
    wattron(header_win, COLOR_PAIR(getColorPair(theme.top_bar.fg, theme.top_bar.bg)));
    mvwprintw(header_win, 0, 0, "%-*s", width, "");
    
    std::string relative_path = config->getRelativeToHvsc(browser->getCurrentPath());
    mvwprintw(header_win, 0, 0, "Nancy SID Player - %s", relative_path.c_str());
    
    wattroff(header_win, COLOR_PAIR(getColorPair(theme.top_bar.fg, theme.top_bar.bg)));
    wnoutrefresh(header_win);
}

void TUI::initColors() {
    start_color();
    
    // Check if terminal supports 256 colors
    if (COLORS >= 256) {
        // Use 256 color mode
        for (int i = 0; i < 256; i++) {
            if (can_change_color()) {
                // Terminal supports color changing (rare)
                continue;
            }
        }
    }
    
    // Initialize default color pairs
    init_pair(1, 15, COLOR_BLACK);  // Bright white on black
    init_pair(2, COLOR_BLACK, 15);  // Black on bright white
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
}

int TUI::getColorPair(int fg, int bg) {
    std::pair<int, int> key = std::make_pair(fg, bg);
    
    auto it = color_pair_cache.find(key);
    if (it != color_pair_cache.end()) {
        return it->second;
    }
    
    // Create new color pair
    int pair_num = next_color_pair++;
    if (pair_num >= COLOR_PAIRS) {
        // Fallback to white on black if we run out of pairs
        return 1;
    }
    
    init_pair(pair_num, fg, bg);
    color_pair_cache[key] = pair_num;
    return pair_num;
}


void TUI::drawBrowser() {
    werase(browser_win);
    
    const auto& theme = config->getCurrentTheme();
    int height, width;
    getmaxyx(browser_win, height, width);
    
    const auto& entries = browser->getEntries();
    int selected = browser->getSelectedIndex();
    
    // Calculate scroll position with 2-line buffer from top/bottom
    int buffer = 2;
    
    // Only scroll when selected item gets within buffer distance of edges
    if (selected < browser_start_line + buffer) {
        // Too close to top, scroll up
        browser_start_line = std::max(0, selected - buffer);
    } else if (selected >= browser_start_line + height - buffer) {
        // Too close to bottom, scroll down
        browser_start_line = std::min((int)entries.size() - height, selected - height + buffer + 1);
    }
    
    // Ensure browser_start_line is within valid bounds
    browser_start_line = std::max(0, std::min(browser_start_line, (int)entries.size() - height));
    if ((int)entries.size() <= height) {
        browser_start_line = 0;
    }
    
    int start_line = browser_start_line;
    
    for (int i = 0; i < std::min((int)entries.size(), height); i++) {
        int entry_idx = start_line + i;
        if (entry_idx >= entries.size()) break;
        
        const auto& entry = entries[entry_idx];
        int line = i;
        
        if (entry.is_directory) {
            if (entry_idx == selected) {
                wattron(browser_win, COLOR_PAIR(getColorPair(theme.selected_dir.fg, theme.selected_dir.bg)));
                mvwprintw(browser_win, line, 0, " %-*s", width - 1, entry.name.c_str());
            } else {
                wattron(browser_win, COLOR_PAIR(getColorPair(theme.dir_name.fg, theme.dir_name.bg)));
                mvwprintw(browser_win, line, 1, "%s", entry.name.c_str());
            }
            
        } else if (entry.is_sid_file) {
            if (entry_idx == selected) {
                wattron(browser_win, COLOR_PAIR(getColorPair(theme.selected_sid.fg, theme.selected_sid.bg)));
                mvwprintw(browser_win, line, 0, " %-*s", width - 1, entry.name.c_str());
            } else {
                wattron(browser_win, COLOR_PAIR(getColorPair(theme.sid_file.fg, theme.sid_file.bg)));
                mvwprintw(browser_win, line, 1, "%s", entry.name.c_str());
            }
        } else {
            if (entry_idx == selected) {
                wattron(browser_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
                mvwprintw(browser_win, line, 0, " %-*s", width - 1, entry.name.c_str());
            } else {
                wattron(browser_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
                mvwprintw(browser_win, line, 1, "%s", entry.name.c_str());
            }
        }
        
        // Reset attributes
        wattrset(browser_win, A_NORMAL);
    }
    
    wnoutrefresh(browser_win);
}

void TUI::drawStilInfo() {
    werase(stil_win);
    
    const auto& theme = config->getCurrentTheme();
    int height, width;
    getmaxyx(stil_win, height, width);
    
    int line = 0;
    
    // Player Information Section
    if (!player->getCurrentFile().empty()) {
        // Get relative file path
        std::string relative_file = config->getRelativeToHvsc(player->getCurrentFile());
        
        // File
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        mvwprintw(stil_win, line, 1, "%9s", "File");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        mvwprintw(stil_win, line, 10, ": ");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        mvwprintw(stil_win, line++, 12, "%s", relative_file.c_str());
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        
        // Title
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        mvwprintw(stil_win, line, 1, "%9s", "Title");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        mvwprintw(stil_win, line, 10, ": ");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        mvwprintw(stil_win, line++, 12, "%s", player->getTitle().c_str());
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        
        // Author
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        mvwprintw(stil_win, line, 1, "%9s", "Author");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        mvwprintw(stil_win, line, 10, ": ");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        mvwprintw(stil_win, line++, 12, "%s", player->getAuthor().c_str());
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        
        // Copyright
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        mvwprintw(stil_win, line, 1, "%9s", "Copyright");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        mvwprintw(stil_win, line, 10, ": ");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        mvwprintw(stil_win, line++, 12, "%s", player->getCopyright().c_str());
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        
        // Track
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        mvwprintw(stil_win, line, 1, "%9s", "Track");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        mvwprintw(stil_win, line, 10, ": ");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        mvwprintw(stil_win, line++, 12, "%d/%d", player->getCurrentTrack(), player->getTrackCount());
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        
        line++; // Empty line separator
    } else {
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        mvwprintw(stil_win, line++, 1, "No file loaded");
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        line++; // Empty line separator
    }
    
    // STIL Information Section
    std::string selected_file = browser->getSelectedFile();
    
    if (!selected_file.empty() && stil_reader->hasInfo(selected_file)) {
        StilEntry info = stil_reader->getInfo(selected_file);
        
        mvwprintw(stil_win, line++, 1, "STIL Information");
        line++;
        
        if (!info.title.empty()) {
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            mvwprintw(stil_win, line, 1, "%9s", "Title");
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
            mvwprintw(stil_win, line, 10, ": ");
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
            mvwprintw(stil_win, line++, 12, "%s", info.title.c_str());
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        }
        if (!info.artist.empty()) {
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            mvwprintw(stil_win, line, 1, "%9s", "Artist");
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
            mvwprintw(stil_win, line, 10, ": ");
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
            mvwprintw(stil_win, line++, 12, "%s", info.artist.c_str());
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        }
        if (!info.copyright.empty()) {
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            mvwprintw(stil_win, line, 1, "%9s", "Copyright");
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
            mvwprintw(stil_win, line, 10, ": ");
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.colon.fg, theme.colon.bg)));
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
            mvwprintw(stil_win, line++, 12, "%s", info.copyright.c_str());
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        }
        
        if (!info.comment.empty()) {
            line++;
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            mvwprintw(stil_win, line++, 1, "Comment:");
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            
            // Word wrap the comment
            std::string comment = info.comment;
            int max_width = width - 3;
            size_t pos = 0;
            while (pos < comment.length() && line < height - 1) {
                size_t end = std::min(pos + max_width, comment.length());
                if (end < comment.length()) {
                    // Find last space
                    while (end > pos && comment[end] != ' ') end--;
                    if (end == pos) end = pos + max_width; // Force break if no space
                }
                
                std::string line_text = comment.substr(pos, end - pos);
                wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
                mvwprintw(stil_win, line++, 3, "%s", line_text.c_str());
                wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
                pos = end + 1; // Skip the space
            }
        }
        
        if (!info.subtune_info.empty()) {
            line++;
            wattron(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            mvwprintw(stil_win, line++, 1, "Subtunes:");
            wattroff(stil_win, COLOR_PAIR(getColorPair(theme.header.fg, theme.header.bg)));
            for (size_t i = 0; i < info.subtune_info.size() && line < height - 1; i++) {
                wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
                mvwprintw(stil_win, line++, 3, "%zu: %s", i + 1, info.subtune_info[i].c_str());
                wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
            }
        }
    } else {
        mvwprintw(stil_win, line++, 1, "STIL Information");
        line++;
        wattron(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
        mvwprintw(stil_win, line++, 1, "No STIL information available");
        mvwprintw(stil_win, line++, 1, "DB: %zu entries", stil_reader->getEntryCount());
        wattroff(stil_win, COLOR_PAIR(getColorPair(theme.value.fg, theme.value.bg)));
    }
    
    // Reset all attributes to ensure clean state
    wattrset(stil_win, A_NORMAL);
    
    wnoutrefresh(stil_win);
}


void TUI::drawStatus() {
    werase(status_win);
    
    const auto& theme = config->getCurrentTheme();
    int width = getmaxx(status_win);
    
    // Fill entire line with background color
    wattron(status_win, COLOR_PAIR(getColorPair(theme.status_bar.fg, theme.status_bar.bg)));
    mvwprintw(status_win, 0, 0, "%-*s", width, "");
    
    if (search_mode) {
        mvwprintw(status_win, 0, 0, "Search: %s", search_query.c_str());
    } else {
        // Left side: File count
        mvwprintw(status_win, 0, 0, "Files: %zu", browser->getEntries().size());
        
        // Right side: Time and Status (if playing)
        if (!player->getCurrentFile().empty()) {
            int minutes = player->getPlayTime() / 60;
            int seconds = player->getPlayTime() % 60;
            
            // Get song length from search database
            int song_length = search->getSongLength(player->getCurrentFile(), player->getCurrentTrack());
            std::string time_str;
            if (song_length > 0) {
                int length_minutes = song_length / 60;
                int length_seconds = song_length % 60;
                time_str = std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds) + 
                          " / " + std::to_string(length_minutes) + ":" + (length_seconds < 10 ? "0" : "") + std::to_string(length_seconds);
            } else {
                time_str = std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
            }
            
            std::string status = player->isPlaying() ? (player->isPaused() ? "PAUSED" : "PLAYING") : "STOPPED";
            std::string status_info = time_str + " [" + status + "]";
            
            // Right align the status info
            int status_len = status_info.length();
            if (status_len < width) {
                mvwprintw(status_win, 0, width - status_len, "%s", status_info.c_str());
            }
        }
    }
    
    wattroff(status_win, COLOR_PAIR(getColorPair(theme.status_bar.fg, theme.status_bar.bg)));
    wnoutrefresh(status_win);
}

void TUI::drawHelp() {
    werase(help_win);
    
    const auto& theme = config->getCurrentTheme();
    wbkgd(help_win, COLOR_PAIR(getColorPair(theme.bottom_bar.fg, theme.bottom_bar.bg)));
    
    if (search_mode) {
        mvwprintw(help_win, 0, 0, "j/k: Up/Down | ENTER: Play | ESC: Exit search | Type to search | SPACE: Pause | s: Stop | J/K: Next/Prev track | q: Quit");
    } else {
        mvwprintw(help_win, 0, 0, "j/k: Up/Down | h: Parent dir | l/ENTER: Play/Enter dir | /: Search | SPACE: Pause | s: Stop | J/K: Next/Prev track | q: Quit");
    }
    
    wnoutrefresh(help_win);
}

void TUI::drawSeparator() {
    werase(separator_win);
    
    const auto& theme = config->getCurrentTheme();
    wattron(separator_win, COLOR_PAIR(getColorPair(theme.separator.fg, theme.separator.bg)));
    
    int height, width;
    getmaxyx(separator_win, height, width);
    
    // Draw vertical line character for the full height using ACS_VLINE for better compatibility
    for (int i = 0; i < height; i++) {
        mvwaddch(separator_win, i, 0, ACS_VLINE);
    }
    
    wattroff(separator_win, COLOR_PAIR(getColorPair(theme.separator.fg, theme.separator.bg)));
    wnoutrefresh(separator_win);
}

void TUI::drawSearchResults() {
    werase(browser_win);
    
    int height, width;
    getmaxyx(browser_win, height, width);
    
    mvwprintw(browser_win, 0, 0, "Search results (%zu found):", search_results.size());
    
    if (search_results.empty()) {
        mvwprintw(browser_win, 1, 0, "No results found");
        wnoutrefresh(browser_win);
        return;
    }
    
    // Calculate scroll position with 2-line buffer from top/bottom for search results
    int buffer = 2;
    int available_height = height - 1; // Account for header line
    
    // Only scroll when selected item gets within buffer distance of edges
    if (search_selected < search_start_line + buffer) {
        // Too close to top, scroll up
        search_start_line = std::max(0, search_selected - buffer);
    } else if (search_selected >= search_start_line + available_height - buffer) {
        // Too close to bottom, scroll down
        search_start_line = std::min((int)search_results.size() - available_height, search_selected - available_height + buffer + 1);
    }
    
    // Ensure start_line is within valid bounds
    search_start_line = std::max(0, std::min(search_start_line, (int)search_results.size() - available_height));
    if ((int)search_results.size() <= available_height) {
        search_start_line = 0;
    }
    
    int start_line = search_start_line;
    
    for (int i = 0; i < std::min((int)search_results.size(), height - 1); i++) {
        int entry_idx = start_line + i;
        if (entry_idx >= (int)search_results.size()) break;
        
        const auto& entry = search_results[entry_idx];
        int line = i + 1;
        
        std::string display_text = entry.getDisplayName();
        if (display_text.length() > width - 1) {
            display_text = display_text.substr(0, width - 4) + "...";
        }
        
        if (entry_idx == search_selected) {
            wattron(browser_win, A_REVERSE);
            mvwprintw(browser_win, line, 0, " %-*s", width - 1, display_text.c_str());
            wattroff(browser_win, A_REVERSE);
        } else {
            mvwprintw(browser_win, line, 1, "%s", display_text.c_str());
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

void TUI::handleResize() {
    // Check if we received a SIGWINCH signal (terminal resize)
    if (is_term_resized(screen_height, screen_width)) {
        // Get new dimensions
        int new_height, new_width;
        getmaxyx(stdscr, new_height, new_width);
        
        // Update stored dimensions
        screen_height = new_height;
        screen_width = new_width;
        
        // Check minimum size
        if (screen_height < 20 || screen_width < 60) {
            return; // Don't resize if too small
        }
        
        // Notify ncurses that we've handled the resize
        resize_term(new_height, new_width);
        
        // Destroy old windows safely
        destroyWindows();
        
        // Clear the screen completely
        erase();
        refresh();
        
        // Reinitialize windows with new dimensions
        initWindows();
        
        // Reset scroll positions for new window sizes
        resetScrollPositions();
        
        // Force a complete redraw
        clearok(stdscr, TRUE);
        refresh();
    }
}

void TUI::resetScrollPositions() {
    browser_start_line = 0;
    search_start_line = 0;
}

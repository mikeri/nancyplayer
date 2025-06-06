#include "tui.h"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        TUI tui;
        tui.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
    
    return 0;
}
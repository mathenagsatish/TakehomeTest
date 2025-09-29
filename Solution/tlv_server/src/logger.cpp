#include "logger.hpp"
#include <iostream>

void ConsoleLogger::log(std::string_view msg) {
    std::lock_guard<std::mutex> lock(m);
    std::cout << msg << std::endl;
}

ILogger& Logger::instance() {
    static ConsoleLogger logger;
    return logger;
}

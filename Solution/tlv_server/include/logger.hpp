#pragma once
#include <string>
#include <mutex>
#include <string_view>

struct ILogger {
    virtual ~ILogger() = default;
    virtual void log(std::string_view msg) = 0;
};

class ConsoleLogger : public ILogger {
    std::mutex m;
public:
    void log(std::string_view msg) override;
};

class Logger {    
public:
    static ILogger& instance();
};

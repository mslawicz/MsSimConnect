#pragma once

#include <string>
#include <unordered_map>

enum class LogLevel
{
    None,
    Always,
    Alarm,
    Warning,
    Info,
    Debug,
    NoOfLevels
};

class Console
{
public:
    Console(Console const&) = delete;
    Console& operator=(Console const&) = delete;
    static Console& getInstance();
    void log(LogLevel level, std::string message);
    void handler(void);
    bool isQuitRequest(void) const { return quitRequest; }
private:
    Console();
    LogLevel currentLevel{ LogLevel::Debug };
    const std::unordered_map<LogLevel, std::string> levelText
    {
        {LogLevel::None, ""},
        {LogLevel::Always, ":"},
        {LogLevel::Alarm, "alarm"},
        {LogLevel::Warning, "warning"},
        {LogLevel::Info, "info"},
        {LogLevel::Debug, "debug"}
    };
    bool quitRequest{ false };
};

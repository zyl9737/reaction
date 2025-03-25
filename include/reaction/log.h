#ifndef REACTION_LOG_H
#define REACTION_LOG_H

#include <iostream>
#include <string>
#include <sstream>

#if __has_include(<format>) && (__cplusplus >= 202002L)
#include <format>
#define REACTION_HAS_FORMAT 1
#else
#define REACTION_HAS_FORMAT 0
#endif

namespace reaction {
class Log {
public:
    template <typename... Args>
    static void info(const std::string &format_str, Args &&...args) {
        log("INFO", format_str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(const std::string &format_str, Args &&...args) {
        log("WARN", format_str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(const std::string &format_str, Args &&...args) {
        log("ERROR", format_str, std::forward<Args>(args)...);
    }

private:
#if REACTION_HAS_FORMAT
    template <typename... Args>
    static void log(const std::string &level, const std::string &format_str, Args &&...args) {
        std::string message = std::vformat(format_str, std::make_format_args(args...));
        std::cout << "[" << level << "] " << message << std::endl;
    }
#else
    template <typename... Args>
    static void log(const std::string &level, const std::string &format_str, Args &&...args) {
        std::ostringstream oss;
        replace_placeholders(oss, format_str, std::forward<Args>(args)...);
        std::cout << "[" << level << "] " << oss.str() << std::endl;
    }

    static void replace_placeholders(std::ostringstream &oss, const std::string &str) {
        oss << str;
    }

    template <typename T, typename... Args>
    static void replace_placeholders(std::ostringstream &oss, const std::string &str, T &&first, Args &&...rest) {
        size_t pos = str.find("{}");
        if (pos != std::string::npos) {
            oss << str.substr(0, pos);
            oss << std::forward<T>(first);
            replace_placeholders(oss, str.substr(pos + 2), std::forward<Args>(rest)...);
        } else {
            oss << str;
        }
    }
#endif
};

} // namespace reaction

#endif // REACTION_LOG_H
// include/reaction/exception.h
#ifndef REACTION_EXCEPTION_H
#define REACTION_EXCEPTION_H

#include <stdexcept>
#include <string>
#include <format>  // C++20的std::format

namespace reaction {

    class Exception : public std::runtime_error {
        public:
            template <typename... Args>
            explicit Exception(const std::string& format_str, Args&&... args)
                : std::runtime_error(createMessage(format_str, std::forward<Args>(args)...)) {}
    
        private:
            template <typename... Args>
            std::string createMessage(const std::string_view fmt_str, Args&&... args) {
                auto fmt_args{ std::make_format_args(args...) };
                return std::string { std::vformat(fmt_str, fmt_args) };
            }
        };

} // namespace reaction

#endif // REACTION_EXCEPTION_H

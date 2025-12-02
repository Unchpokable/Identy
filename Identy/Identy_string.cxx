#include "Identy_pch.hxx"

#include "Identy_strings.hxx"

namespace
{
constexpr const char* WHITESPACE_CHARS = " \t\r\n";
} // namespace

std::string_view identy::strings::trim_whitespace(std::string_view string)
{
    auto start = string.find_first_not_of(WHITESPACE_CHARS);
    auto end = string.find_last_not_of(WHITESPACE_CHARS);

    if(start == std::string_view::npos || end == std::string_view::npos) {
        return {};
    }

    return string.substr(start, end - start + 1);
}

#pragma once
#include <string>
#include <string_view>

namespace toy {

struct Location {
    int line = 1;
    int column = 1;
};

class ErrorReporter {
public:
    ErrorReporter(std::string_view source, std::string_view filename);

    void error(Location loc, std::string_view message);
    void warning(Location loc, std::string_view message);

    bool hasErrors() const { return m_hasErrors; }

private:
    void report(Location loc, std::string_view message, std::string_view type, std::string_view color);
    void printSourceLine(Location loc);

    std::string_view m_source;
    std::string_view m_filename;
    bool m_hasErrors = false;
};

} // namespace toy

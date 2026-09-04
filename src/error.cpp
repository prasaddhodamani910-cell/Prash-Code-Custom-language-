#include "error.h"
#include <iostream>

namespace toy {

ErrorReporter::ErrorReporter(std::string_view source, std::string_view filename)
    : m_source(source), m_filename(filename) {}

void ErrorReporter::error(Location loc, std::string_view message) {
    if (loc.line == m_lastErrorLoc.line && loc.column == m_lastErrorLoc.column && message == m_lastErrorMessage) {
        return; // Suppress duplicate
    }
    m_lastErrorLoc = loc;
    m_lastErrorMessage = std::string(message);
    report(loc, message, "error", "\033[31m"); // Red
    m_hasErrors = true;
}

void ErrorReporter::warning(Location loc, std::string_view message) {
    report(loc, message, "warning", "\033[33m"); // Yellow
}

void ErrorReporter::report(Location loc, std::string_view message, std::string_view type, std::string_view color) {
    std::cerr << "\033[1m" << m_filename << ":" << loc.line << ":" << loc.column << ": "
              << color << type << ": \033[0m\033[1m" << message << "\033[0m\n";
    printSourceLine(loc);
}

void ErrorReporter::printSourceLine(Location loc) {
    size_t start = 0;
    for (int i = 1; i < loc.line; ++i) {
        start = m_source.find('\n', start);
        if (start == std::string_view::npos) return;
        start++;
    }

    size_t end = m_source.find('\n', start);
    if (end == std::string_view::npos) {
        end = m_source.length();
    }

    std::string_view line = m_source.substr(start, end - start);
    std::cerr << line << '\n';

    for (int i = 1; i < loc.column; ++i) {
        if (i - 1 < (int)line.length() && line[i - 1] == '\t') {
            std::cerr << '\t';
        } else {
            std::cerr << ' ';
        }
    }
    std::cerr << "\033[1;32m^\033[0m\n"; // Green caret
}

} // namespace toy

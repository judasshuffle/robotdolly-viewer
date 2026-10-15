#pragma once

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace robotdolly {
// Edit only the numeric token in the existing [neural] intensity assignment.
// Keep other settings, comments, spacing, BOM and line endings byte-for-byte.
inline bool updateIntensityText(const std::string& original, float percent,
                                std::string& output, std::string& message) {
    if (!std::isfinite(percent) || percent < 0.f || percent > 100.f) {
        message = "AI intensity must be between 0 and 100%.";
        return false;
    }
    auto trim = [](const std::string& value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return std::string();
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    };
    std::ostringstream strength;
    strength.imbue(std::locale::classic()); // TOML always uses a decimal point.
    strength << std::fixed << std::setprecision(2) << (percent / 100.f);
    bool neural = false;
    size_t valueStart = std::string::npos, valueEnd = 0;
    for (size_t pos = 0; pos < original.size();) {
        const auto eol = original.find('\n', pos);
        const auto stop = eol == std::string::npos ? original.size() : eol + 1;
        const std::string line = original.substr(pos, stop - pos);
        std::string clean = trim(line.substr(0, line.find('#')));
        if (pos == 0 && clean.compare(0, 3, "\xEF\xBB\xBF") == 0)
            clean = trim(clean.substr(3));
        if (!clean.empty() && clean.front() == '[') neural = (clean == "[neural]");
        const auto equals = line.find('=');
        if (neural && equals != std::string::npos && trim(line.substr(0, equals)) == "intensity") {
            if (valueStart != std::string::npos) {
                message = "Duplicate [neural] intensity; no changes made.";
                return false;
            }
            const auto begin = line.find_first_not_of(" \t", equals + 1);
            if (begin == std::string::npos || line[begin] == '#' || line[begin] == '\r' || line[begin] == '\n') {
                message = "Empty [neural] intensity; no changes made.";
                return false;
            }
            auto end = line.find_first_of(" \t\r\n#", begin);
            if (end == std::string::npos) end = line.size();
            valueStart = pos + begin;
            valueEnd = pos + end;
        }
        pos = stop;
    }
    if (valueStart == std::string::npos) {
        message = "Missing [neural] intensity in sidecar.toml; no changes made.";
        return false;
    }
    output = original.substr(0, valueStart) + strength.str() + original.substr(valueEnd);
    return true;
}

#ifdef _WIN32
inline bool saveIntensity(const std::filesystem::path& config, float percent, std::string& message) {
    std::string original;
    {
        std::ifstream in(config, std::ios::binary);
        if (!in) { message = "Cannot open Sidecar configuration."; return false; }
        original.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        if (in.bad()) { message = "Cannot read Sidecar configuration."; return false; }
    } // Essential: MSVC's ifstream does not share DELETE access. Close BEFORE replacement.
    std::string output;
    if (!updateIntensityText(original, percent, output, message)) return false;

    // A sibling file keeps replacement on the same volume. CREATE_NEW avoids
    // overwriting a temp file belonging to another writer or an earlier failure.
    const std::filesystem::path temporary = config.wstring() + L".robotdolly-" +
        std::to_wstring(GetCurrentProcessId()) + L".tmp";
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        message = "Cannot create temporary Sidecar configuration. Windows error: " + std::to_string(GetLastError());
        return false;
    }
    DWORD written = 0;
    const bool wrote = output.size() <= MAXDWORD &&
        WriteFile(file, output.data(), static_cast<DWORD>(output.size()), &written, nullptr) &&
        written == output.size() && FlushFileBuffers(file);
    DWORD error = wrote ? ERROR_SUCCESS : GetLastError();
    if (!CloseHandle(file) && !error) error = GetLastError();
    if (!wrote || error) {
        DeleteFileW(temporary.c_str());
        message = "Could not write Sidecar intensity settings. Windows error: " + std::to_string(error);
        return false;
    }
    if (!MoveFileExW(temporary.c_str(), config.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = GetLastError();
        DeleteFileW(temporary.c_str());
        message = "Could not replace Sidecar configuration. Windows error: " + std::to_string(error);
        return false; // Original is intact; caller must not restart the renderer.
    }
    return true;
}
#endif
}

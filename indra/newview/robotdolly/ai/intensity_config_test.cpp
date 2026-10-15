#include "intensity_config.h"
#include <iostream>
#include <limits>
#include <stdexcept>

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::string read(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    check(static_cast<bool>(in), "read fixture");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(text.data(), text.size());
    out.close();
    check(static_cast<bool>(out), "write fixture");
}
struct CommaDecimal : std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
};

int main(int argc, char** argv) {
    try {
        const std::string source = "\xEF\xBB\xBF[neural] # options\r\n"
            "enable_hooks = 2\r\n\tintensity  =  0.85  # keep this\r\n"
            "color_strength = 0.75\r\n[other] # unrelated\r\nintensity = 7\r\n";
        std::string output, message;
        for (float percent : {0.f, 5.f, 50.f, 85.f, 100.f}) {
            check(robotdolly::updateIntensityText(source, percent, output, message), "edit CRLF and commented section");
            std::ostringstream number;
            number << std::fixed << std::setprecision(2) << percent / 100.f;
            std::string expected = source;
            expected.replace(expected.find("0.85"), 4, number.str());
            check(output == expected, "preserve every byte outside intensity value");
        }
        check(robotdolly::updateIntensityText("[neural]\nintensity=0.85", 0.f, output, message)
            && output == "[neural]\nintensity=0.00", "LF and no final newline");
        const auto locale = std::locale();
        std::locale::global(std::locale(locale, new CommaDecimal));
        check(robotdolly::updateIntensityText(source, 50.f, output, message)
            && output.find("0.50") != std::string::npos, "locale-independent TOML value");
        std::locale::global(locale);
        for (float invalid : {-1.f, 101.f, std::numeric_limits<float>::quiet_NaN(),
                              std::numeric_limits<float>::infinity()})
            check(!robotdolly::updateIntensityText(source, invalid, output, message), "invalid percentage rejected");
        for (const std::string invalid : {"[other]\nintensity=0.5\n", "[neural]\n# intensity=0.5\n",
                "[neural]\n[other] # test\nintensity=0.5\n", "[neural]\nintensity=\n",
                "[neural]\nintensity=0.5\nintensity=0.6\n"})
            check(!robotdolly::updateIntensityText(invalid, 50.f, output, message), "missing/ambiguous setting rejected");
#ifdef _WIN32
        const auto dir = std::filesystem::temp_directory_path() /
            ("robotdolly-intensity-test-" + std::to_string(GetCurrentProcessId()));
        check(std::filesystem::create_directory(dir), "create isolated test folder");
        const auto config = dir / "sidecar.toml";
        const auto replacement = dir / "replacement.tmp";
        write(config, source);
        write(replacement, source);
        {
            // Reproduce the original failure with the SAME C++ input stream.
            std::ifstream held(config, std::ios::binary);
            check(!MoveFileExW(replacement.c_str(), config.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH), "open ifstream blocks replacement");
            const DWORD error = GetLastError();
            std::cout << "Reproduced replacement failure with open ifstream: Windows error " << error << '\n';
            check(error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION, "expected sharing failure");
            check(!robotdolly::saveIntensity(config, 0.f, message), "external lock fails safely");
            check(read(config) == source, "failed replacement leaves original unchanged");
        }
        // After the input handle closes, the production writer must succeed.
        for (float percent : {0.f, 50.f, 100.f, 85.f}) {
            check(robotdolly::saveIntensity(config, percent, message), "save after releasing lock");
            robotdolly::updateIntensityText(source, percent, output, message);
            check(read(config) == output, "saved value and other settings are exact");
        }
        write(config, source);
        SetFileAttributesW(config.c_str(), FILE_ATTRIBUTE_READONLY);
        check(!robotdolly::saveIntensity(config, 50.f, message), "read-only config fails safely");
        SetFileAttributesW(config.c_str(), FILE_ATTRIBUTE_NORMAL);
        check(read(config) == source, "read-only original preserved");
        const auto temporary = config.wstring() + L".robotdolly-" + std::to_wstring(GetCurrentProcessId()) + L".tmp";
        check(!std::filesystem::exists(temporary), "failed save removes its temporary file");
        write(temporary, "another writer");
        check(!robotdolly::saveIntensity(config, 50.f, message), "existing temp file not overwritten");
        check(read(temporary) == "another writer" && read(config) == source, "temp collision preserves both files");
        std::filesystem::remove(temporary);
        if (argc == 2) {
            // Optional real configuration fixture: copy only, never edit source.
            const std::string actual = read(std::filesystem::path(argv[1]));
            write(config, actual);
            for (float percent : {0.f, 50.f, 100.f}) {
                check(robotdolly::saveIntensity(config, percent, message), "save copied runtime config");
                robotdolly::updateIntensityText(actual, percent, output, message);
                check(read(config) == output, "actual runtime settings preserved");
            }
            check(read(std::filesystem::path(argv[1])) == actual, "source runtime fixture unchanged");
        }
        std::filesystem::remove_all(dir);
#endif
        std::cout << "Intensity configuration tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

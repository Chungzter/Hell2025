#include <chrono>

namespace Util {
    double GetCurrentTime() {
        using namespace std::chrono;
        return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
    }

    std::string TimestampToString(uint64_t timestamp) {
        try {
            std::chrono::sys_time<std::chrono::seconds> tp{ std::chrono::seconds{timestamp} };
            std::chrono::zoned_time zt{ std::chrono::current_zone(), tp };
            return std::format("{:%Y-%m-%d %H:%M:%S %Z}", zt); // %Z gives timezone abbr.

        }
        catch (const std::exception& e) {
            return "Invalid Timestamp";
        }
    }
}
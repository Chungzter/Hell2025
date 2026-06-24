#include "Util.h"

namespace Util {
    void PackUint64(uint64_t value, uint32_t& xOut, uint32_t& yOut) {
        xOut = (uint64_t)((value & 0xFFFFFFFF00000000LL) >> 32);
        yOut = (uint64_t)(value & 0xFFFFFFFFLL);
    }

    void UnpackUint64(uint32_t xValue, uint32_t yValue, uint64_t& out) {
        out = ((uint64_t)xValue) << 32 | yValue;
    }
}

/*

    uint64_t value = 1234567890987654321;
    uint64_t result = 0;
    uint32_t x;
    uint32_t y;
    Util::PackUint64(value, x, y);
    Util::UnpackUint64(x, y, result);

    std::cout << "value: " << value << "\n";
    std::cout << "x: " << x << "\n";
    std::cout << "y: " << y << "\n";
    std::cout << "result: " << result << "\n\n";

*/
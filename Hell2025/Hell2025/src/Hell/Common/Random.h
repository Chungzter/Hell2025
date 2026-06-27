#pragma once

#include <random>

namespace Hell::Random {

    inline std::mt19937& Generator() {
        thread_local std::mt19937 generator(std::random_device{}());
        return generator;
    }

    inline float Float(float min, float max) {
        std::uniform_real_distribution<float> distribution(min, max);
        return distribution(Generator());
    }

    inline int Int(int min, int max) {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(Generator());
    }
}

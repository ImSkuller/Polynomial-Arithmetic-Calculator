#include "polycalc/services/Timer.hpp"

namespace polycalc {

void Timer::start() noexcept {
    start_ = std::chrono::steady_clock::now();
    running_ = true;
}

void Timer::stop() noexcept {
    end_ = std::chrono::steady_clock::now();
    running_ = false;
}

double Timer::elapsedMilliseconds() const noexcept {
    const auto endPoint = running_ ? std::chrono::steady_clock::now() : end_;
    return std::chrono::duration<double, std::milli>(endPoint - start_).count();
}

} // namespace polycalc

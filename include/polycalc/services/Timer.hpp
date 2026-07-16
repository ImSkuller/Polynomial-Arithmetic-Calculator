#pragma once

#include <chrono>
#include <utility>

namespace polycalc {

// A stopwatch built on steady_clock. elapsedMilliseconds() can be queried
// while still running, giving a live reading.
class Timer {
public:
    void start() noexcept;
    void stop() noexcept;
    double elapsedMilliseconds() const noexcept;

    template <typename Callable>
    static double measureMilliseconds(Callable&& callable) {
        Timer timer;
        timer.start();
        std::forward<Callable>(callable)();
        timer.stop();
        return timer.elapsedMilliseconds();
    }

private:
    std::chrono::steady_clock::time_point start_{};
    std::chrono::steady_clock::time_point end_{};
    bool running_ = false;
};

} // namespace polycalc

#ifndef TIMER_SYSTEM_HPP
#define TIMER_SYSTEM_HPP

#include <algorithm>

// Lightweight countdown timer. Call Tick(dt) each frame; Reset() to restart.
struct Timer {
    float total = 0.0F;
    float remaining = 0.0F;

    Timer() = default;
    explicit Timer(float totalSec) : total(totalSec), remaining(0.0F) {}

    void Tick(float dt) { remaining = std::max(0.0F, remaining - dt); }

    // Restart from full duration.
    void Reset() { remaining = total; }

    // Change total and immediately restart.
    void ResetTo(float seconds) {
        total = seconds;
        remaining = seconds;
    }

    bool IsRunning() const { return remaining > 0.0F; }
    bool IsExpired() const { return remaining <= 0.0F; }

    float GetRemaining() const { return remaining; }
    float GetTotal() const { return total; }
};

#endif // TIMER_SYSTEM_HPP

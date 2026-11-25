#pragma once

#include "model.hpp"
#include <atomic>
#include <string>

namespace tracker {

struct TimerTickResult {
    bool elapsed_incremented = false;
    bool segment_completed = false;
    bool autosave_due = false;
    std::string completed_segment_label;
};

/// Manages the study/break segment timer state and periodic autosave cadence.
class TimerController {
public:
    [[nodiscard]] bool is_running() const { return running_.load(); }
    void set_running(bool running) { running_.store(running); }
    void pause() { running_.store(false); }

    /// Toggles the running state and returns the new running state.
    bool toggle();

    /// Advances the active segment by 1 second if running and reports completion/autosave events.
    TimerTickResult tick(Roadmap& roadmap);

private:
    std::atomic<bool> running_{false};
    int ticks_since_save_ = 0;
};

} // namespace tracker

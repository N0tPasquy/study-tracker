#pragma once

#include <functional>

namespace tracker {

using SoundPlayerFunc = std::function<void()>;

/// Plays a distinctive timer alarm: an asynchronous rhythmic sequence of 4 terminal bells
/// combined with a native system chime (pw-play / aplay / canberra on Linux, afplay on macOS).
void play_alarm_sound();

} // namespace tracker

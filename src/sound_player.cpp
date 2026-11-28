#include "sound_player.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

namespace tracker {

void play_alarm_sound() {
    std::thread([]() {
#if defined(__APPLE__)
        std::system("afplay /System/Library/Sounds/Glass.aiff >/dev/null 2>&1 || "
                    "afplay /System/Library/Sounds/Ping.aiff >/dev/null 2>&1 &");
#elif defined(__linux__)
        std::system("pw-play /usr/share/sounds/freedesktop/stereo/complete.oga >/dev/null 2>&1 || "
                    "pw-play /usr/share/sounds/freedesktop/stereo/alarm-clock-elapsed.oga >/dev/null 2>&1 || "
                    "paplay /usr/share/sounds/freedesktop/stereo/complete.oga >/dev/null 2>&1 || "
                    "canberra-gtk-play -i complete >/dev/null 2>&1 || "
                    "aplay /usr/share/sounds/alsa/Front_Center.wav >/dev/null 2>&1 &");
#endif

        // 4 rhythmic beeps spaced by 220ms so it sounds like a real alarm and cannot be missed
        for (int i = 0; i < 4; ++i) {
            std::cout << '\a' << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(220));
        }
    }).detach();
}

} // namespace tracker

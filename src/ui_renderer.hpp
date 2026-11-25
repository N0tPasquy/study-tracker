#pragma once

#include "model.hpp"
#include "navigation_controller.hpp"
#include <string>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

namespace tracker {

struct UIRenderContext {
    const Roadmap& roadmap;
    NavigationController& navigation;
    AppMode mode;
    const std::string& note_buffer;
    int note_cursor;
    ftxui::Box& note_box;
    bool timer_running;
    bool data_saved;
    const std::string& alert_message;
};

[[nodiscard]] ftxui::Element render_header(const Roadmap& roadmap);
[[nodiscard]] ftxui::Element render_focus_view(const UIRenderContext& ctx);
[[nodiscard]] ftxui::Element render_roadmap_view(const Roadmap& roadmap, NavigationController& navigation);
[[nodiscard]] ftxui::Element render_footer(const UIRenderContext& ctx);

} // namespace tracker

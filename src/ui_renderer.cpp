#include "ui_renderer.hpp"
#include "date_utils.hpp"
#include "stats.hpp"

#include <algorithm>
#include <cstdio>
#include <utility>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace tracker {

namespace {

size_t utf8_codepoint_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

} // namespace

ftxui::Element render_header(const Roadmap& roadmap) {
    using namespace ftxui;

    const int weekly_done = count_weekly_completed_blocks(roadmap);
    const int total_done = count_total_completed_blocks(roadmap);
    const int total_blocks = static_cast<int>(roadmap.blocks.size());
    const float global_ratio = (total_blocks > 0) ? (static_cast<float>(total_done) / total_blocks) : 0.0f;
    const int percent = static_cast<int>(global_ratio * 100.0f);

    const Color weekly_color = (weekly_done >= roadmap.weekly_goal_blocks)
        ? Color(Color::GreenLight)
        : Color(Color::Yellow);

    return vbox({
        hbox({
            text(" " + roadmap.exam_name + " ") | bold | color(Color::White) | bgcolor(Color::BlueLight),
            separator(),
            text(" Esame: " + roadmap.exam_date + " (" + format_countdown(roadmap.exam_date) + ") ") | color(Color::CyanLight),
            separator(),
            text(" Obiettivo Settimana: " + std::to_string(weekly_done) + "/" +
                 std::to_string(roadmap.weekly_goal_blocks) + " ") | bold | color(weekly_color),
            filler(),
            text(" Progresso: " + std::to_string(total_done) + "/" + std::to_string(total_blocks) +
                 " (" + std::to_string(percent) + "%) ") | bold | color(Color::White),
            gauge(global_ratio) | size(WIDTH, EQUAL, 18) | color(Color::Green),
        }),
        separatorLight(),
    });
}

ftxui::Element render_focus_view(const UIRenderContext& ctx) {
    using namespace ftxui;

    const auto& block = ctx.roadmap.blocks[ctx.roadmap.current_block_index];
    const FocusSection focus_section = ctx.navigation.focus_section();

    // 1. Block Header
    auto phase_badge = text(" " + block.module + " ") | bold |
                       bgcolor(get_phase_color(block.phase)) | color(Color::Black);
    auto block_title = text(" [#" + std::to_string(block.id) + "] " + block.title) | bold | color(Color::White);

    Element status_badge;
    if (block.completed) {
        status_badge = text(" [COMPLETATO" + (block.completed_at.empty() ? "" : ": " + block.completed_at) + "] ") |
                       bold | color(Color::GreenLight);
    } else {
        status_badge = text(" [IN CORSO] ") | bold | color(Color::YellowLight);
    }

    auto block_header = hbox({
        phase_badge,
        text(" "),
        block_title,
        filler(),
        status_badge,
        text(" [M: Completa Blocco] ") | dim,
    });

    // 2. Segments & Timer Panel
    Elements seg_elements;
    int study_elapsed = 0, study_target = 0;
    int break_elapsed = 0, break_target = 0;

    for (int i = 0; i < static_cast<int>(block.segments.size()); ++i) {
        const auto& seg = block.segments[i];
        const bool is_active = (i == block.current_segment_index);

        if (seg.type == "study") {
            study_elapsed += seg.elapsed_seconds;
            study_target += seg.target_minutes * 60;
        } else {
            break_elapsed += seg.elapsed_seconds;
            break_target += seg.target_minutes * 60;
        }

        const std::string mark = seg.done ? "[x] " : (is_active ? "[>] " : "[ ] ");
        const std::string time_str = format_time_mm_ss(seg.elapsed_seconds) + "/" +
                                     format_time_mm_ss(seg.target_minutes * 60);

        const Color type_col = (seg.type == "study") ? Color::DeepSkyBlue1 : Color::DarkOrange;
        auto seg_text = text(mark + seg.label + " (" + time_str + ")") | color(type_col);

        if (is_active) {
            seg_elements.push_back(hbox({
                seg_text | bold | inverted,
            }));
        } else {
            seg_elements.push_back(seg_text);
        }

        if (i < static_cast<int>(block.segments.size()) - 1) {
            seg_elements.push_back(text("  │  ") | dim);
        }
    }

    auto segments_row = hbox(std::move(seg_elements));

    // Active Segment Big Timer
    Element timer_display;
    if (block.current_segment_index >= 0 &&
        block.current_segment_index < static_cast<int>(block.segments.size())) {
        const auto& cur_seg = block.segments[block.current_segment_index];
        const int target_sec = cur_seg.target_minutes * 60;
        float seg_ratio = (target_sec > 0) ? (static_cast<float>(cur_seg.elapsed_seconds) / target_sec) : 0.0f;
        if (seg_ratio > 1.0f) seg_ratio = 1.0f;

        const Color timer_col = cur_seg.done
            ? Color(Color::GreenLight)
            : (ctx.timer_running ? Color(Color::CyanLight) : Color(Color::GrayLight));
        const Color seg_badge_bg = (cur_seg.type == "study") ? Color(Color::Blue) : Color(Color::DarkOrange);

        std::string overtime_str;
        if (cur_seg.elapsed_seconds > target_sec) {
            const int extra = cur_seg.elapsed_seconds - target_sec;
            overtime_str = " (+" + format_duration_hm(extra) + " Overtime)";
        }

        const std::string status_label = ctx.timer_running ? "TIMER IN CORSO" : "IN PAUSA";

        timer_display = vbox({
            hbox({
                text("  " + cur_seg.label + "  ") | bold | color(Color::White) | bgcolor(seg_badge_bg),
                text("  " + format_time_mm_ss(cur_seg.elapsed_seconds) + " / " + format_time_mm_ss(target_sec)) | bold | color(timer_col),
                text(overtime_str) | bold | color(Color::YellowLight),
                text("  [" + status_label + "]") | (ctx.timer_running ? color(Color::Green) : dim),
            }),
            separatorEmpty(),
            gauge(seg_ratio) | color(cur_seg.done ? Color::Green : Color::Cyan),
            separatorEmpty(),
            hbox({
                text("  Comandi: [t] " + std::string(ctx.timer_running ? "Pausa" : "Avvia") +
                     "  [h/l] Segmento Prec/Succ  [c] " + std::string(cur_seg.done ? "Riapri Segmento" : "Completa Segmento") + "  [R] Reset Segmento") | dim,
            }),
        });
    }

    Element alert_box = text("");
    if (!ctx.alert_message.empty()) {
        alert_box = vbox({
            separatorEmpty(),
            hbox({
                filler(),
                text("  🔔 " + ctx.alert_message + "  ") | bold | bgcolor(Color::Gold1) | color(Color::Black),
                filler(),
            }),
        });
    }

    auto time_summary = hbox({
        text("  Tempo Blocco — Studio: " + format_duration_hm(study_elapsed) + " / " + format_duration_hm(study_target) +
             "  │  Pause: " + format_duration_hm(break_elapsed) + " / " + format_duration_hm(break_target)) | dim,
    });

    auto timer_panel = window(
        text(" Segmenti & Timer ") | bold,
        vbox({
            segments_row,
            separatorLight(),
            timer_display,
            alert_box,
            separatorLight(),
            time_summary,
        })
    );

    // 3. Tasks Checklist
    int done_tasks = 0;
    const int total_tasks = static_cast<int>(block.tasks.size());
    for (const auto& t : block.tasks) {
        if (t.done) done_tasks++;
    }

    Elements task_elements;
    for (int i = 0; i < total_tasks; ++i) {
        const auto& t = block.tasks[i];
        const bool is_selected = (focus_section == FocusSection::Tasks && i == ctx.navigation.selected_task_index());

        const std::string mark = t.done ? "[✔] " : "[ ] ";
        const std::string pointer = is_selected ? "▶ " : "  ";

        auto item_text = text(pointer + mark + t.text);
        if (t.done) {
            item_text = item_text | color(Color::GreenLight);
        }

        if (is_selected) {
            task_elements.push_back(item_text | bold | bgcolor(Color::NavyBlue) | color(Color::White));
        } else {
            task_elements.push_back(item_text);
        }
    }

    if (task_elements.empty()) {
        task_elements.push_back(text("  (Nessun task per questo blocco)") | dim);
    }

    std::string task_panel_title = " Checklist Tasks (" + std::to_string(done_tasks) + "/" + std::to_string(total_tasks) + ") ";
    if (focus_section == FocusSection::Tasks) {
        task_panel_title += "[j/k: Muovi, x/Space: Spunta, Tab: Risorse]";
    } else {
        task_panel_title += "[Tab per selezionare]";
    }

    auto checklist_panel = window(
        text(task_panel_title) | bold | (focus_section == FocusSection::Tasks ? color(Color::CyanLight) : color(Color::GrayLight)),
        vbox(std::move(task_elements))
    );

    // 3b. Resources Panel
    int watched_res = 0;
    const int total_res = static_cast<int>(block.resources.size());
    for (const auto& r : block.resources) {
        if (r.watched) watched_res++;
    }

    Elements res_elements;
    for (int i = 0; i < total_res; ++i) {
        const auto& r = block.resources[i];
        const bool is_selected = (focus_section == FocusSection::Resources && i == ctx.navigation.selected_resource_index());

        const std::string mark = r.watched ? "[✔] " : "[ ] ";
        const std::string pointer = is_selected ? "▶ " : "  ";

        std::string badge_label = "[VID]";
        Color badge_bg = Color::DeepSkyBlue1;
        if (r.type == "playlist") {
            badge_label = "[PL]";
            badge_bg = Color::Gold1;
        } else if (r.type != "video") {
            badge_label = "[LINK]";
            badge_bg = Color::CadetBlue;
        }

        auto badge_elem = text(" " + badge_label + " ") | bold | color(Color::Black) | bgcolor(badge_bg);
        auto item_text = text(" " + r.title);
        if (r.watched) {
            item_text = item_text | color(Color::GreenLight) | dim;
        }

        auto row_content = hbox({
            text(pointer + mark),
            badge_elem,
            item_text,
        });

        if (is_selected) {
            res_elements.push_back(row_content | bold | bgcolor(Color::NavyBlue) | color(Color::White));
        } else {
            res_elements.push_back(row_content);
        }
    }

    if (res_elements.empty()) {
        res_elements.push_back(text("  Nessuna risorsa collegata (usa il libro di testo)") | dim);
    }

    std::string res_panel_title = " Risorse (" + std::to_string(watched_res) + "/" + std::to_string(total_res) + ") ";
    if (focus_section == FocusSection::Resources) {
        res_panel_title += "[j/k: Muovi, x/Space: Visto, Enter/o: Apri URL, Tab: Tasks]";
    } else {
        res_panel_title += "[Tab per selezionare]";
    }

    auto resources_panel = window(
        text(res_panel_title) | bold | (focus_section == FocusSection::Resources ? color(Color::CyanLight) : color(Color::GrayLight)),
        vbox(std::move(res_elements))
    );

    // 4. Last Note Box
    Element note_content;
    if (ctx.mode == AppMode::Insert) {
        const int clamped_cursor = std::clamp(ctx.note_cursor, 0, static_cast<int>(ctx.note_buffer.size()));
        Element text_line;

        if (ctx.note_buffer.empty()) {
            text_line = hbox({
                text("  "),
                text(" ") | bgcolor(Color::Gold1) | color(Color::Black) | focusCursorBlockBlinking,
            });
        } else if (clamped_cursor >= static_cast<int>(ctx.note_buffer.size())) {
            text_line = hbox({
                text("  " + ctx.note_buffer) | color(Color::White),
                text(" ") | bgcolor(Color::Gold1) | color(Color::Black) | focusCursorBlockBlinking,
            });
        } else {
            size_t char_len = utf8_codepoint_len(static_cast<unsigned char>(ctx.note_buffer[clamped_cursor]));
            if (clamped_cursor + char_len > ctx.note_buffer.size()) {
                char_len = ctx.note_buffer.size() - clamped_cursor;
            }

            const std::string before = ctx.note_buffer.substr(0, clamped_cursor);
            const std::string cursor_char = ctx.note_buffer.substr(clamped_cursor, char_len);
            const std::string after = ctx.note_buffer.substr(clamped_cursor + char_len);

            Elements line_parts;
            line_parts.push_back(text("  " + before) | color(Color::White));
            line_parts.push_back(text(cursor_char) | bgcolor(Color::Gold1) | color(Color::Black) | bold | focusCursorBlockBlinking);
            if (!after.empty()) {
                line_parts.push_back(text(after) | color(Color::White));
            }
            text_line = hbox(std::move(line_parts));
        }

        note_content = vbox({
            text_line,
            text("  [MODALITÀ INSERT: Scrivi liberamente. Premi Esc o Enter per salvare] ") | color(Color::YellowLight) | dim,
        });
    } else {
        if (ctx.note_buffer.empty()) {
            note_content = text("  (Nessuna nota. Premi 'i' o 'a' per scrivere dove ti sei fermato...)") | dim;
        } else {
            note_content = text("  " + ctx.note_buffer) | color(Color::White);
        }
    }

    auto note_panel = window(
        text(" Ultimo Punto / Dubbi (last_note) [i/a: Modifica, Esc: Salva] ") | bold,
        note_content
    ) | reflect(ctx.note_box);

    if (ctx.mode == AppMode::Insert) {
        note_panel = note_panel | color(Color::Gold1);
    }

    return vbox({
        block_header,
        separatorEmpty(),
        timer_panel,
        separatorEmpty(),
        checklist_panel,
        separatorEmpty(),
        resources_panel,
        separatorEmpty(),
        note_panel,
    });
}

ftxui::Element render_roadmap_view(const Roadmap& roadmap, NavigationController& navigation) {
    using namespace ftxui;

    const int total_blocks = static_cast<int>(roadmap.blocks.size());
    constexpr int visible_count = 14;

    navigation.adjust_roadmap_scroll(total_blocks, visible_count);
    const int scroll_offset = navigation.roadmap_scroll_offset();
    const int selected_index = navigation.roadmap_selected_index();

    Elements rows;

    if (scroll_offset > 0) {
        rows.push_back(text("  ▲ ... altri " + std::to_string(scroll_offset) + " blocchi sopra ...") | dim | center);
    }

    const int end_idx = std::min(total_blocks, scroll_offset + visible_count);
    for (int i = scroll_offset; i < end_idx; ++i) {
        const auto& b = roadmap.blocks[i];
        const bool is_selected = (i == selected_index);
        const bool is_current = (i == roadmap.current_block_index);

        const std::string pointer = is_selected ? "▶ " : "  ";
        const std::string status_mark = b.completed ? "[✔]" : (is_current ? "[>]" : "[ ]");

        int done_tasks = 0;
        for (const auto& t : b.tasks) {
            if (t.done) done_tasks++;
        }
        const std::string tasks_stat = "(" + std::to_string(done_tasks) + "/" + std::to_string(b.tasks.size()) + ")";

        char id_buf[16];
        std::snprintf(id_buf, sizeof(id_buf), "[#%02d]", b.id);

        auto phase_tag = text(" F" + std::to_string(b.phase) + " ") | bold |
                         bgcolor(get_phase_color(b.phase)) | color(Color::Black);

        auto row_elem = hbox({
            text(pointer) | bold | color(Color::Yellow),
            text(status_mark + " ") | (b.completed ? color(Color::GreenLight) : (is_current ? color(Color::CyanLight) : dim)),
            text(std::string(id_buf) + " ") | bold | color(Color::White),
            phase_tag,
            text(" " + b.title) | (b.completed ? color(Color::GrayLight) : color(Color::White)),
            filler(),
            text(tasks_stat + " ") | dim,
            b.completed ? text(b.completed_at + " ") | color(Color::Green) : text("          "),
        });

        if (is_selected) {
            rows.push_back(row_elem | bold | bgcolor(Color::NavyBlue) | color(Color::White));
        } else if (is_current) {
            rows.push_back(row_elem | bgcolor(Color::DarkBlue));
        } else {
            rows.push_back(row_elem);
        }
    }

    if (end_idx < total_blocks) {
        rows.push_back(text("  ▼ ... altri " + std::to_string(total_blocks - end_idx) + " blocchi sotto ...") | dim | center);
    }

    auto list_box = window(
        text(" ROADMAP COMPLETA (" + std::to_string(total_blocks) + " BLOCCHI) ") | bold,
        vbox(std::move(rows))
    );

    return vbox({
        list_box,
        separatorEmpty(),
        hbox({
            filler(),
            text(" Scorri: [j/k] o [↓/↑]  │  Mezza Pagina: [Ctrl+d/u]  │  Inizio/Fine: [gg/G]  │  Seleziona: [Enter/l]  │  Chiudi: [Esc/q] ") | dim,
            filler(),
        }),
    });
}

ftxui::Element render_footer(const UIRenderContext& ctx) {
    using namespace ftxui;

    Element mode_badge = (ctx.mode == AppMode::Normal)
        ? text(" NORMAL ") | bold | bgcolor(Color::CadetBlue) | color(Color::Black)
        : text(" INSERT ") | bold | bgcolor(Color::Gold1) | color(Color::Black);

    const ViewState view_state = ctx.navigation.view_state();
    Element view_badge = (view_state == ViewState::Focus)
        ? text(" FOCUS ") | bold | bgcolor(Color::RoyalBlue1) | color(Color::White)
        : text(" ROADMAP ") | bold | bgcolor(Color::Purple) | color(Color::White);

    Element legend;
    if (ctx.mode == AppMode::Insert) {
        legend = text(" [Esc/Enter] Salva ed Esci  [Ctrl+S] Salva ") | color(Color::YellowLight);
    } else if (view_state == ViewState::Focus) {
        if (ctx.navigation.focus_section() == FocusSection::Resources) {
            legend = text(" [j/k] Risorse  [Enter/o] Apri Link  [x/Space] Visto  [Tab] Tasks  [t] Timer  [h/l] Segmento  [H/L] Blocco  [r] Roadmap  [ZZ/q] Esci ") | dim;
        } else {
            legend = text(" [j/k] Tasks  [x/Space] Spunta  [Tab] Risorse  [t] Timer  [h/l] Segmento  [H/L] Blocco  [i/a] Nota  [r] Roadmap  [ZZ/q] Esci ") | dim;
        }
    } else {
        legend = text(" [j/k] Muovi  [Enter/l] Seleziona  [Ctrl+d/u] Pagina  [gg/G] Cima/Fondo  [Esc/q] Indietro ") | dim;
    }

    Element timer_status = ctx.timer_running
        ? text(" TIMER ON ") | bold | bgcolor(Color::GreenLight) | color(Color::Black)
        : text(" PAUSA ") | dim | bgcolor(Color::GrayDark) | color(Color::White);

    Element save_status = ctx.data_saved
        ? text(" [Salvato] ") | color(Color::GreenLight)
        : text(" [Modificato] ") | bold | color(Color::YellowLight);

    return vbox({
        separatorLight(),
        hbox({
            mode_badge,
            text(" "),
            view_badge,
            text(" "),
            legend,
            filler(),
            timer_status,
            text(" "),
            save_status,
        }),
    });
}

} // namespace tracker

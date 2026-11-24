#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <ftxui/component/screen_interactive.hpp>
#include "../src/model.hpp"
#include "../src/storage.hpp"
#include "../src/ui.hpp"

void test_load_and_utf8() {
    std::filesystem::path original_path = "./roadmap_matematica2.json";
    assert(std::filesystem::exists(original_path));

    auto roadmap = tracker::load_roadmap(original_path);
    assert(roadmap.blocks.size() == 42);
    assert(roadmap.exam_name == "Matematica II + Probabilità");
    assert(roadmap.exam_date == "2027-01-25");

    // Check block 38 and 40 have 3 segments
    assert(roadmap.blocks[37].segments.size() == 3);
    assert(roadmap.blocks[39].segments.size() == 3);
    // Standard blocks have 5 segments
    assert(roadmap.blocks[0].segments.size() == 5);

    // Verify UTF-8 characters are preserved
    bool found_utf8 = false;
    for (const auto& block : roadmap.blocks) {
        if (block.title.find("Equazioni differenziali") != std::string::npos ||
            block.title.find("Probabilità") != std::string::npos ||
            block.module.find("Probabilità") != std::string::npos) {
            found_utf8 = true;
            break;
        }
    }
    assert(found_utf8);
    std::cout << "[PASS] test_load_and_utf8\n";
}

void test_date_and_weekly_goal() {
    auto roadmap = tracker::load_roadmap("./roadmap_matematica2.json");

    int initial_weekly = tracker::count_weekly_completed_blocks(roadmap);
    std::string today = tracker::get_current_date_iso();
    assert(today.size() == 10 && today[4] == '-' && today[7] == '-');

    // Mark block 0 as completed today
    roadmap.blocks[0].completed = true;
    roadmap.blocks[0].completed_at = today;

    assert(tracker::is_in_current_week(today));
    assert(tracker::count_weekly_completed_blocks(roadmap) == initial_weekly + 1);

    // Mark block 0 as not completed (reopen)
    roadmap.blocks[0].completed = false;
    roadmap.blocks[0].completed_at = "";
    assert(tracker::count_weekly_completed_blocks(roadmap) == initial_weekly);

    // Test countdown
    int days = tracker::get_days_until(roadmap.exam_date);
    assert(days > 0); // 2027-01-25 is in future
    std::string countdown_str = tracker::format_countdown(roadmap.exam_date);
    assert(countdown_str.find("giorni") != std::string::npos);

    std::cout << "[PASS] test_date_and_weekly_goal\n";
}

void test_atomic_save_and_reload() {
    std::filesystem::path test_json = "./test_roadmap_temp.json";
    if (std::filesystem::exists(test_json)) {
        std::filesystem::remove(test_json);
    }

    auto roadmap = tracker::load_roadmap("./roadmap_matematica2.json");

    roadmap.current_block_index = 2;
    roadmap.blocks[2].current_segment_index = 1;
    roadmap.blocks[2].segments[1].elapsed_seconds = 145;
    roadmap.blocks[2].segments[1].done = true;
    roadmap.blocks[2].tasks[0].done = true;
    roadmap.blocks[2].last_note = "Esercizio 4 completato con successo. Rivedere matrice hessiana.";

    bool saved = tracker::save_roadmap(roadmap, test_json);
    assert(saved);
    assert(std::filesystem::exists(test_json));
    assert(!std::filesystem::exists(test_json.string() + ".tmp"));

    auto reloaded = tracker::load_roadmap(test_json);
    assert(reloaded.current_block_index == 2);
    assert(reloaded.blocks.size() == 42);
    assert(reloaded.blocks[2].current_segment_index == 1);
    assert(reloaded.blocks[2].segments[1].elapsed_seconds == 145);
    assert(reloaded.blocks[2].segments[1].done == true);
    assert(reloaded.blocks[2].tasks[0].done == true);
    assert(reloaded.blocks[2].last_note == "Esercizio 4 completato con successo. Rivedere matrice hessiana.");

    std::filesystem::remove(test_json);
    std::cout << "[PASS] test_atomic_save_and_reload\n";
}

void test_ui_events_and_modal_navigation() {
    std::filesystem::path test_json = "./test_ui_events.json";
    auto roadmap = tracker::load_roadmap("./roadmap_matematica2.json");
    tracker::save_roadmap(roadmap, test_json);

    auto screen = ftxui::ScreenInteractive::FitComponent();
    tracker::UIController controller(roadmap, test_json, screen);
    auto component = controller.CreateComponent();

    using namespace ftxui;

    // 1. Initial state: block 0, segment 0, task 0 not done
    assert(roadmap.current_block_index == 0);
    assert(roadmap.blocks[0].current_segment_index == 0);
    assert(!roadmap.blocks[0].tasks[0].done);

    // 2. Toggle task 0 with 'x'
    bool handled = component->OnEvent(Event::Character('x'));
    assert(handled);
    assert(roadmap.blocks[0].tasks[0].done == true);

    // 3. Move task down with 'j' and toggle with 'Space'
    component->OnEvent(Event::Character('j'));
    component->OnEvent(Event::Character(' '));
    assert(roadmap.blocks[0].tasks[1].done == true);

    // 4. Test timer toggle with 't'
    assert(!controller.is_timer_running());
    component->OnEvent(Event::Character('t'));
    assert(controller.is_timer_running());

    // Advance timer with tick_timer()
    int initial_elapsed = roadmap.blocks[0].segments[0].elapsed_seconds;
    controller.tick_timer();
    assert(roadmap.blocks[0].segments[0].elapsed_seconds == initial_elapsed + 1);

    // 5. Test segment switch with 'l' (should auto-pause timer!)
    component->OnEvent(Event::Character('l'));
    assert(roadmap.blocks[0].current_segment_index == 1);
    assert(!controller.is_timer_running()); // Verified auto-pause!

    // 6. Test block completion toggle with 'M'
    assert(!roadmap.blocks[0].completed);
    component->OnEvent(Event::Character('M'));
    assert(roadmap.blocks[0].completed);
    assert(!roadmap.blocks[0].completed_at.empty());

    // 7. Test INSERT mode: press 'i'
    component->OnEvent(Event::Character('i'));
    // In insert mode, characters like 'j', 'k', 't', 'x' should NOT trigger commands!
    component->OnEvent(Event::Character('C'));
    component->OnEvent(Event::Character('i'));
    component->OnEvent(Event::Character('a'));
    component->OnEvent(Event::Character('o'));
    // Press Return to save and exit INSERT mode (Opzione A approved by user)
    component->OnEvent(Event::Return);

    assert(roadmap.blocks[0].last_note.find("Ciao") != std::string::npos);

    // 8. Test Roadmap View: press 'r'
    component->OnEvent(Event::Character('r'));
    // Move selection in roadmap with 'j'
    component->OnEvent(Event::Character('j'));
    component->OnEvent(Event::Character('j'));
    // Select block with 'Enter'
    component->OnEvent(Event::Return);
    assert(roadmap.current_block_index == 2);

    // Clean up
    std::filesystem::remove(test_json);
    std::cout << "[PASS] test_ui_events_and_modal_navigation\n";
}

int main() {
    std::cout << "--- RUNNING TRACKER UNIT & INTEGRATION TESTS ---\n";
    test_load_and_utf8();
    test_date_and_weekly_goal();
    test_atomic_save_and_reload();
    test_ui_events_and_modal_navigation();
    std::cout << "--- ALL TESTS PASSED SUCCESSFULLY! ---\n";
    return 0;
}

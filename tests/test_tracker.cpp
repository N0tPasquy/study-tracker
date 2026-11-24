#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <ftxui/component/screen_interactive.hpp>
#include "../src/model.hpp"
#include "../src/storage.hpp"
#include "../src/ui.hpp"
#include "../src/url_opener.hpp"

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
    // Ensure deterministic test fixture independent of user runtime progress
    roadmap.current_block_index = 0;
    roadmap.blocks[0].current_segment_index = 0;
    roadmap.blocks[0].segments[0].done = false;
    roadmap.blocks[0].segments[1].done = false;
    roadmap.blocks[0].segments[2].done = false;
    roadmap.blocks[0].tasks[0].done = false;
    roadmap.blocks[0].tasks[1].done = false;
    roadmap.blocks[0].completed = false;
    roadmap.blocks[0].completed_at = "";
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

    // 5b. Test completing segment with 'c' on segment 1
    assert(!roadmap.blocks[0].segments[1].done);
    component->OnEvent(Event::Character('c'));
    assert(roadmap.blocks[0].segments[1].done);
    // Advances to next segment (index 2)
    assert(roadmap.blocks[0].current_segment_index == 2);

    // Return to segment 1 with 'h'
    component->OnEvent(Event::Character('h'));
    assert(roadmap.blocks[0].current_segment_index == 1);
    assert(roadmap.blocks[0].segments[1].done);

    // Pressing 'c' on an already completed segment removes the checkmark (reopen)
    component->OnEvent(Event::Character('c'));
    assert(!roadmap.blocks[0].segments[1].done);
    assert(roadmap.blocks[0].current_segment_index == 1);

    // Test 'R' resets both elapsed_seconds and done
    roadmap.blocks[0].segments[1].elapsed_seconds = 45;
    roadmap.blocks[0].segments[1].done = true;
    component->OnEvent(Event::Character('R'));
    assert(roadmap.blocks[0].segments[1].elapsed_seconds == 0);
    assert(!roadmap.blocks[0].segments[1].done);

    // 6. Test block completion toggle with 'M'
    assert(!roadmap.blocks[0].completed);
    component->OnEvent(Event::Character('M'));
    assert(roadmap.blocks[0].completed);
    assert(!roadmap.blocks[0].completed_at.empty());

    // 6b. Test unmapped keys in NORMAL mode ('u', 'o' while focus is on Tasks)
    // They must be consumed and must NOT pollute the note buffer!
    assert(controller.get_focus_section() == tracker::FocusSection::Tasks);
    std::string note_before = roadmap.blocks[0].last_note;
    bool handled_u = component->OnEvent(Event::Character('u'));
    assert(handled_u);
    bool handled_o = component->OnEvent(Event::Character('o'));
    assert(handled_o);
    assert(roadmap.blocks[0].last_note == note_before);

    // 7. Test INSERT mode: press 'i'
    component->OnEvent(Event::Character('i'));
    assert(controller.get_note_cursor() == 0); // 'i' puts cursor at the beginning

    // In insert mode, characters like 'j', 'k', 't', 'x' should NOT trigger commands!
    component->OnEvent(Event::Character('C'));
    component->OnEvent(Event::Character('i'));
    component->OnEvent(Event::Character('a'));
    component->OnEvent(Event::Character('o'));
    assert(controller.get_note_cursor() == 4);

    // Arrow navigation moves cursor
    component->OnEvent(Event::ArrowLeft);
    assert(controller.get_note_cursor() == 3);
    component->OnEvent(Event::ArrowRight);
    assert(controller.get_note_cursor() == 4);

    // Press Return to save and exit INSERT mode (Opzione A approved by user)
    component->OnEvent(Event::Return);

    assert(roadmap.blocks[0].last_note.find("Ciao") != std::string::npos);

    // Test 'a' enters insert mode and places cursor at the end
    int prev_size = static_cast<int>(roadmap.blocks[0].last_note.size());
    component->OnEvent(Event::Character('a'));
    assert(controller.get_note_cursor() == prev_size);
    component->OnEvent(Event::Escape);

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

void test_resources_serialization_and_retrocompat() {
    // 1. Deserialization of Block JSON without "resources" key
    nlohmann::json j_old = {
        {"id", 1},
        {"phase", 0},
        {"module", "Fase 0"},
        {"title", "Test Old Block"},
        {"completed", false},
        {"completed_at", ""},
        {"current_segment_index", 0},
        {"segments", nlohmann::json::array()},
        {"tasks", nlohmann::json::array()},
        {"last_note", ""}
    };

    tracker::Block b_old = j_old.get<tracker::Block>();
    assert(b_old.resources.empty());

    // 2. Deserialization of Block with "resources": []
    nlohmann::json j_empty = j_old;
    j_empty["resources"] = nlohmann::json::array();
    tracker::Block b_empty = j_empty.get<tracker::Block>();
    assert(b_empty.resources.empty());

    // 3. Round-trip with video and playlist resources
    tracker::Resource r1{"Video 1", "https://www.youtube.com/watch?v=123", "video", false};
    tracker::Resource r2{"Playlist 1", "https://www.youtube.com/playlist?list=abc", "playlist", true};
    b_empty.resources = {r1, r2};

    nlohmann::json j_serialized = b_empty;
    tracker::Block b_roundtrip = j_serialized.get<tracker::Block>();
    assert(b_roundtrip.resources.size() == 2);
    assert(b_roundtrip.resources[0].title == "Video 1");
    assert(b_roundtrip.resources[0].url == "https://www.youtube.com/watch?v=123");
    assert(b_roundtrip.resources[0].type == "video");
    assert(!b_roundtrip.resources[0].watched);

    assert(b_roundtrip.resources[1].title == "Playlist 1");
    assert(b_roundtrip.resources[1].url == "https://www.youtube.com/playlist?list=abc");
    assert(b_roundtrip.resources[1].type == "playlist");
    assert(b_roundtrip.resources[1].watched);

    // 4. Default safe values for missing fields inside Resource
    nlohmann::json j_minimal_res = {
        {"title", "Default Resource"},
        {"url", "https://example.com/video"}
    };
    tracker::Resource r_minimal = j_minimal_res.get<tracker::Resource>();
    assert(r_minimal.title == "Default Resource");
    assert(r_minimal.url == "https://example.com/video");
    assert(r_minimal.type == "video");
    assert(!r_minimal.watched);

    std::cout << "[PASS] test_resources_serialization_and_retrocompat\n";
}

void test_url_validation() {
    // Valid URLs
    assert(tracker::is_valid_http_url("https://www.youtube.com/watch?v=fgYEQxrBlUU"));
    assert(tracker::is_valid_http_url("https://www.youtube.com/playlist?list=PL..."));
    assert(tracker::is_valid_http_url("http://example.com/test?a=1&b=2#section"));
    assert(tracker::is_valid_http_url("https://youtu.be/abc123XYZ"));

    // Invalid URLs
    assert(!tracker::is_valid_http_url(""));
    assert(!tracker::is_valid_http_url("https://"));
    assert(!tracker::is_valid_http_url("http://"));
    assert(!tracker::is_valid_http_url("ftp://example.com"));
    assert(!tracker::is_valid_http_url("file:///etc/passwd"));
    assert(!tracker::is_valid_http_url("javascript:alert(1)"));
    assert(!tracker::is_valid_http_url("https://example.com/test\n"));
    assert(!tracker::is_valid_http_url("https://example.com/test\r"));
    assert(!tracker::is_valid_http_url("https://example.com/test with spaces"));
    assert(!tracker::is_valid_http_url("https://example.com; rm -rf /"));
    assert(!tracker::is_valid_http_url("https://example.com\"test\""));
    assert(!tracker::is_valid_http_url("https://example.com'test'"));
    assert(!tracker::is_valid_http_url("https://example.com`test`"));

    std::cout << "[PASS] test_url_validation\n";
}

void test_resources_ui_and_url_opener_mock() {
    std::filesystem::path test_json = "./test_resources_temp.json";
    auto roadmap = tracker::load_roadmap("./roadmap_matematica2.json");
    roadmap.current_block_index = 0;
    roadmap.blocks[0].current_segment_index = 0;
    roadmap.blocks[0].resources[0].watched = false;
    roadmap.blocks[0].resources[1].watched = false;
    tracker::save_roadmap(roadmap, test_json);

    std::string opened_url;
    int open_count = 0;
    auto mock_opener = [&](const std::string& url) -> bool {
        opened_url = url;
        open_count++;
        return true;
    };

    auto screen = ftxui::ScreenInteractive::FitComponent();
    tracker::UIController controller(roadmap, test_json, screen, mock_opener);
    auto component = controller.CreateComponent();

    using namespace ftxui;

    // 1. Initial focus is Tasks
    assert(controller.get_focus_section() == tracker::FocusSection::Tasks);

    // 2. Tab switches focus to Resources
    bool handled_tab = component->OnEvent(Event::Tab);
    assert(handled_tab);
    assert(controller.get_focus_section() == tracker::FocusSection::Resources);

    // Block 0 has 6 resources
    assert(roadmap.blocks[0].resources.size() == 6);
    assert(controller.get_selected_resource_index() == 0);
    assert(!roadmap.blocks[0].resources[0].watched);

    // 3. Toggle watched with 'x'
    component->OnEvent(Event::Character('x'));
    assert(roadmap.blocks[0].resources[0].watched == true);

    // 4. Toggle watched back with 'Space'
    component->OnEvent(Event::Character(' '));
    assert(roadmap.blocks[0].resources[0].watched == false);

    // 5. Navigate resources with 'j'
    component->OnEvent(Event::Character('j'));
    assert(controller.get_selected_resource_index() == 1);

    // 6. Open resource 1 with 'o'
    component->OnEvent(Event::Character('o'));
    assert(open_count == 1);
    assert(opened_url == roadmap.blocks[0].resources[1].url);

    // 7. Open resource 1 with 'Return'
    component->OnEvent(Event::Return);
    assert(open_count == 2);
    assert(opened_url == roadmap.blocks[0].resources[1].url);

    // 8. Switching block resets selected_resource_index_
    controller.switch_to_block(1);
    assert(controller.get_selected_resource_index() == 0);

    // 9. Block with empty resources: safe no-op on 'o' and 'x'
    int empty_block_idx = -1;
    for (int i = 0; i < static_cast<int>(roadmap.blocks.size()); ++i) {
        if (roadmap.blocks[i].resources.empty()) {
            empty_block_idx = i;
            break;
        }
    }
    assert(empty_block_idx >= 0);
    controller.switch_to_block(empty_block_idx);
    controller.set_focus_section(tracker::FocusSection::Resources);
    open_count = 0;
    component->OnEvent(Event::Character('o'));
    assert(open_count == 0); // No opener call
    component->OnEvent(Event::Character('x')); // No crash

    // 10. Verify persistence
    controller.switch_to_block(0);
    controller.set_focus_section(tracker::FocusSection::Resources);
    component->OnEvent(Event::Character('x')); // Watched resource 0
    controller.trigger_save();

    auto reloaded = tracker::load_roadmap(test_json);
    assert(reloaded.blocks[0].resources[0].watched == true);

    // Clean up
    std::filesystem::remove(test_json);
    std::cout << "[PASS] test_resources_ui_and_url_opener_mock\n";
}

int main() {
    std::cout << "--- RUNNING TRACKER UNIT & INTEGRATION TESTS ---\n";
    test_load_and_utf8();
    test_date_and_weekly_goal();
    test_atomic_save_and_reload();
    test_ui_events_and_modal_navigation();
    test_resources_serialization_and_retrocompat();
    test_url_validation();
    test_resources_ui_and_url_opener_mock();
    std::cout << "--- ALL TESTS PASSED SUCCESSFULLY! ---\n";
    return 0;
}

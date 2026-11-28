#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <ftxui/component/screen_interactive.hpp>

#include "../src/date_utils.hpp"
#include "../src/model.hpp"
#include "../src/stats.hpp"
#include "../src/storage.hpp"
#include "../src/ui.hpp"
#include "../src/url_opener.hpp"
#include "../src/sound_player.hpp"

namespace {

struct TempFileGuard {
    std::filesystem::path path;

    explicit TempFileGuard(const std::string& filename)
        : path(std::filesystem::temp_directory_path() / filename) {
        cleanup();
    }

    ~TempFileGuard() { cleanup(); }

    void cleanup() const {
        std::error_code ec;
        std::filesystem::remove(path, ec);
        std::filesystem::remove(path.string() + ".tmp", ec);
    }
};

std::filesystem::path repo_roadmap_path() {
#ifdef TRACKER_SOURCE_DIR
    return std::filesystem::path(TRACKER_SOURCE_DIR) / "roadmap_matematica2.json";
#else
    return std::filesystem::path("./roadmap_matematica2.json");
#endif
}

tracker::Roadmap make_sample_roadmap() {
    tracker::Roadmap rm;
    rm.exam_name = "Matematica II + Probabilità";
    rm.exam_date = "2027-01-25";
    rm.weekly_goal_blocks = 3;
    rm.current_block_index = 0;

    for (int i = 0; i < 4; ++i) {
        tracker::Block b;
        b.id = i + 1;
        b.phase = i % 2;
        b.module = "Fase " + std::to_string(b.phase);
        b.title = "Blocco di test " + std::to_string(i + 1) + " — Equazioni differenziali";
        b.segments = {
            {"Teoria", "study", 1, 0, false},
            {"Pausa", "break", 15, 0, false},
            {"Esercizi", "study", 60, 0, false},
        };
        b.tasks = {
            {"Studiare il teorema principale", false},
            {"Svolgere 5 esercizi", false},
        };
        if (i == 0) {
            b.resources = {
                {"Video Lezione 1", "https://www.youtube.com/watch?v=abc123", "video", false},
                {"Playlist Completa", "https://www.youtube.com/playlist?list=PLxyz", "playlist", false},
            };
        }
        rm.blocks.push_back(std::move(b));
    }

    return rm;
}

} // namespace

TEST_CASE("Load bundled roadmap and verify UTF-8 preservation") {
    const auto original_path = repo_roadmap_path();
    REQUIRE(std::filesystem::exists(original_path));

    const auto roadmap = tracker::load_roadmap(original_path);
    CHECK(roadmap.blocks.size() == 42);
    CHECK(roadmap.exam_name == "Matematica II + Probabilità");
    CHECK(roadmap.exam_date == "2027-01-25");

    // Check block 38 and 40 have 3 segments, standard blocks have 5
    CHECK(roadmap.blocks[37].segments.size() == 3);
    CHECK(roadmap.blocks[39].segments.size() == 3);
    CHECK(roadmap.blocks[0].segments.size() == 5);

    bool found_utf8 = false;
    for (const auto& block : roadmap.blocks) {
        if (block.title.find("Equazioni differenziali") != std::string::npos ||
            block.title.find("Probabilità") != std::string::npos ||
            block.module.find("Probabilità") != std::string::npos) {
            found_utf8 = true;
            break;
        }
    }
    CHECK(found_utf8);
}

TEST_CASE("Date helpers and weekly goal calculation") {
    auto roadmap = make_sample_roadmap();

    const int initial_weekly = tracker::count_weekly_completed_blocks(roadmap);
    const std::string today = tracker::get_current_date_iso();
    REQUIRE(today.size() == 10);
    CHECK(today[4] == '-');
    CHECK(today[7] == '-');

    CHECK(tracker::parse_date_iso(today).has_value());
    CHECK_FALSE(tracker::parse_date_iso("invalid").has_value());
    CHECK_FALSE(tracker::parse_date_iso("2026-13-01").has_value());
    CHECK_FALSE(tracker::parse_date_iso("2026-00-10").has_value());
    CHECK_FALSE(tracker::parse_date_iso("2026-01-99").has_value());

    CHECK(tracker::is_in_current_week(today));
    CHECK(tracker::format_countdown(today) == "OGGI");

    roadmap.blocks[0].completed = true;
    roadmap.blocks[0].completed_at = today;
    CHECK(tracker::count_weekly_completed_blocks(roadmap) == initial_weekly + 1);
    CHECK(tracker::count_total_completed_blocks(roadmap) == 1);

    roadmap.blocks[0].completed = false;
    roadmap.blocks[0].completed_at = "";
    CHECK(tracker::count_weekly_completed_blocks(roadmap) == initial_weekly);
    CHECK(tracker::count_total_completed_blocks(roadmap) == 0);

    CHECK(tracker::get_days_until("2099-01-01") > 0);
    CHECK(tracker::format_countdown("2099-01-01").find("giorni") != std::string::npos);

    CHECK(tracker::format_time_mm_ss(125) == "02:05");
    CHECK(tracker::format_duration_hm(3660) == "1h 1m");
    CHECK(tracker::format_duration_hm(900) == "15m");
}

TEST_CASE("Atomic save, reload, and error handling") {
    SUBCASE("Successful round-trip save and reload") {
        TempFileGuard guard("study_tracker_test_atomic.json");
        auto roadmap = make_sample_roadmap();

        roadmap.current_block_index = 2;
        roadmap.blocks[2].current_segment_index = 1;
        roadmap.blocks[2].segments[1].elapsed_seconds = 145;
        roadmap.blocks[2].segments[1].done = true;
        roadmap.blocks[2].tasks[0].done = true;
        roadmap.blocks[2].last_note = "Esercizio 4 completato con successo. Rivedere matrice hessiana.";

        CHECK(tracker::save_roadmap(roadmap, guard.path));
        CHECK(std::filesystem::exists(guard.path));
        CHECK_FALSE(std::filesystem::exists(guard.path.string() + ".tmp"));

        const auto reloaded = tracker::load_roadmap(guard.path);
        CHECK(reloaded.current_block_index == 2);
        CHECK(reloaded.blocks.size() == 4);
        CHECK(reloaded.blocks[2].current_segment_index == 1);
        CHECK(reloaded.blocks[2].segments[1].elapsed_seconds == 145);
        CHECK(reloaded.blocks[2].segments[1].done == true);
        CHECK(reloaded.blocks[2].tasks[0].done == true);
        CHECK(reloaded.blocks[2].last_note == "Esercizio 4 completato con successo. Rivedere matrice hessiana.");
    }

    SUBCASE("Missing file and malformed JSON throw std::runtime_error") {
        CHECK_THROWS_AS((void)tracker::load_roadmap("/nonexistent/path/missing.json"), std::runtime_error);

        TempFileGuard malformed("study_tracker_test_malformed.json");
        {
            std::ofstream ofs(malformed.path);
            ofs << "{ invalid json !!!";
        }
        CHECK_THROWS_AS((void)tracker::load_roadmap(malformed.path), std::runtime_error);
    }

    SUBCASE("Failed save returns false without leaving tmp file and UI tracks unsaved state") {
        auto roadmap = make_sample_roadmap();
        const std::filesystem::path bad_path = "/nonexistent_dir_12345/roadmap.json";
        CHECK_FALSE(tracker::save_roadmap(roadmap, bad_path));
        CHECK_FALSE(std::filesystem::exists(bad_path.string() + ".tmp"));

        auto screen = ftxui::ScreenInteractive::FitComponent();
        tracker::UIController controller(roadmap, bad_path, screen);
        controller.trigger_save();
        CHECK_FALSE(controller.is_data_saved());
        CHECK(controller.get_alert_message().find("Errore") != std::string::npos);
    }
}

TEST_CASE("UI events, modal navigation, and timer completion") {
    TempFileGuard guard("study_tracker_test_ui_events.json");
    auto roadmap = make_sample_roadmap();
    REQUIRE(tracker::save_roadmap(roadmap, guard.path));

    auto screen = ftxui::ScreenInteractive::FitComponent();
    tracker::UIController controller(roadmap, guard.path, screen);
    auto component = controller.CreateComponent();

    using namespace ftxui;

    CHECK(roadmap.current_block_index == 0);
    CHECK(roadmap.blocks[0].current_segment_index == 0);
    CHECK_FALSE(roadmap.blocks[0].tasks[0].done);

    // Toggle task 0 with 'x'
    CHECK(component->OnEvent(Event::Character('x')));
    CHECK(roadmap.blocks[0].tasks[0].done);
    CHECK(controller.is_data_saved());

    // Move task down with 'j' and toggle with Space
    component->OnEvent(Event::Character('j'));
    component->OnEvent(Event::Character(' '));
    CHECK(roadmap.blocks[0].tasks[1].done);

    // Timer toggle with 't' and tick_timer()
    CHECK_FALSE(controller.is_timer_running());
    component->OnEvent(Event::Character('t'));
    CHECK(controller.is_timer_running());

    int alarm_sound_invocations = 0;
    controller.set_sound_player([&alarm_sound_invocations]() {
        alarm_sound_invocations++;
    });

    const int initial_elapsed = roadmap.blocks[0].segments[0].elapsed_seconds;
    controller.tick_timer();
    CHECK(roadmap.blocks[0].segments[0].elapsed_seconds == initial_elapsed + 1);

    // Advance timer to segment target (1 minute = 60s) -> auto-complete + auto-pause + alert + alarm sound
    for (int s = 1; s < 60; ++s) {
        controller.tick_timer();
    }
    CHECK(roadmap.blocks[0].segments[0].elapsed_seconds == 60);
    CHECK(roadmap.blocks[0].segments[0].done);
    CHECK_FALSE(controller.is_timer_running());
    CHECK(controller.get_alert_message().find("completato") != std::string::npos);
    CHECK(alarm_sound_invocations == 1);

    // Resuming timer enters overtime cleanly without losing completion
    component->OnEvent(Event::Character('t'));
    CHECK(controller.is_timer_running());
    controller.tick_timer();
    CHECK(roadmap.blocks[0].segments[0].elapsed_seconds == 61);
    CHECK(controller.is_timer_running());

    // Segment switch with 'l' auto-pauses timer
    component->OnEvent(Event::Character('l'));
    CHECK(roadmap.blocks[0].current_segment_index == 1);
    CHECK_FALSE(controller.is_timer_running());

    // Complete segment 1 with 'c' -> marks done and advances to segment 2
    CHECK_FALSE(roadmap.blocks[0].segments[1].done);
    component->OnEvent(Event::Character('c'));
    CHECK(roadmap.blocks[0].segments[1].done);
    CHECK(roadmap.blocks[0].current_segment_index == 2);

    // Return to segment 1 with 'h' and reopen with 'c'
    component->OnEvent(Event::Character('h'));
    CHECK(roadmap.blocks[0].current_segment_index == 1);
    CHECK(roadmap.blocks[0].segments[1].done);

    component->OnEvent(Event::Character('c'));
    CHECK_FALSE(roadmap.blocks[0].segments[1].done);
    CHECK(roadmap.blocks[0].current_segment_index == 1);

    // 'R' resets elapsed_seconds and done
    roadmap.blocks[0].segments[1].elapsed_seconds = 45;
    roadmap.blocks[0].segments[1].done = true;
    component->OnEvent(Event::Character('R'));
    CHECK(roadmap.blocks[0].segments[1].elapsed_seconds == 0);
    CHECK_FALSE(roadmap.blocks[0].segments[1].done);

    // Block completion toggle with 'M'
    CHECK_FALSE(roadmap.blocks[0].completed);
    component->OnEvent(Event::Character('M'));
    CHECK(roadmap.blocks[0].completed);
    CHECK_FALSE(roadmap.blocks[0].completed_at.empty());

    // Unmapped keys in NORMAL mode must not leak into note_buffer
    CHECK(controller.get_focus_section() == tracker::FocusSection::Tasks);
    const std::string note_before = roadmap.blocks[0].last_note;
    CHECK(component->OnEvent(Event::Character('u')));
    CHECK(component->OnEvent(Event::Character('o')));
    CHECK(roadmap.blocks[0].last_note == note_before);

    // INSERT mode via 'i'
    component->OnEvent(Event::Character('i'));
    CHECK(controller.get_note_cursor() == 0);

    component->OnEvent(Event::Character('C'));
    component->OnEvent(Event::Character('i'));
    component->OnEvent(Event::Character('a'));
    component->OnEvent(Event::Character('o'));
    CHECK(controller.get_note_cursor() == 4);

    component->OnEvent(Event::ArrowLeft);
    CHECK(controller.get_note_cursor() == 3);
    component->OnEvent(Event::ArrowRight);
    CHECK(controller.get_note_cursor() == 4);

    component->OnEvent(Event::Return);
    CHECK(roadmap.blocks[0].last_note.find("Ciao") != std::string::npos);

    // 'a' places cursor at end
    const int prev_size = static_cast<int>(roadmap.blocks[0].last_note.size());
    component->OnEvent(Event::Character('a'));
    CHECK(controller.get_note_cursor() == prev_size);
    component->OnEvent(Event::Escape);

    // Roadmap view navigation
    component->OnEvent(Event::Character('r'));
    component->OnEvent(Event::Character('j'));
    component->OnEvent(Event::Character('j'));
    component->OnEvent(Event::Return);
    CHECK(roadmap.current_block_index == 2);
}

TEST_CASE("Resources serialization and backward compatibility") {
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

    const tracker::Block b_old = j_old.get<tracker::Block>();
    CHECK(b_old.resources.empty());

    nlohmann::json j_empty = j_old;
    j_empty["resources"] = nlohmann::json::array();
    tracker::Block b_empty = j_empty.get<tracker::Block>();
    CHECK(b_empty.resources.empty());

    const tracker::Resource r1{"Video 1", "https://www.youtube.com/watch?v=123", "video", false};
    const tracker::Resource r2{"Playlist 1", "https://www.youtube.com/playlist?list=abc", "playlist", true};
    b_empty.resources = {r1, r2};

    const nlohmann::json j_serialized = b_empty;
    const tracker::Block b_roundtrip = j_serialized.get<tracker::Block>();
    REQUIRE(b_roundtrip.resources.size() == 2);
    CHECK(b_roundtrip.resources[0].title == "Video 1");
    CHECK(b_roundtrip.resources[0].url == "https://www.youtube.com/watch?v=123");
    CHECK(b_roundtrip.resources[0].type == "video");
    CHECK_FALSE(b_roundtrip.resources[0].watched);

    CHECK(b_roundtrip.resources[1].title == "Playlist 1");
    CHECK(b_roundtrip.resources[1].url == "https://www.youtube.com/playlist?list=abc");
    CHECK(b_roundtrip.resources[1].type == "playlist");
    CHECK(b_roundtrip.resources[1].watched);

    nlohmann::json j_minimal_res = {
        {"title", "Default Resource"},
        {"url", "https://example.com/video"}
    };
    const tracker::Resource r_minimal = j_minimal_res.get<tracker::Resource>();
    CHECK(r_minimal.title == "Default Resource");
    CHECK(r_minimal.url == "https://example.com/video");
    CHECK(r_minimal.type == "video");
    CHECK_FALSE(r_minimal.watched);
}

TEST_CASE("HTTP URL validation rejects unsafe or invalid URLs") {
    CHECK(tracker::is_valid_http_url("https://www.youtube.com/watch?v=fgYEQxrBlUU"));
    CHECK(tracker::is_valid_http_url("https://www.youtube.com/playlist?list=PL..."));
    CHECK(tracker::is_valid_http_url("http://example.com/test?a=1&b=2#section"));
    CHECK(tracker::is_valid_http_url("https://youtu.be/abc123XYZ"));

    CHECK_FALSE(tracker::is_valid_http_url(""));
    CHECK_FALSE(tracker::is_valid_http_url("https://"));
    CHECK_FALSE(tracker::is_valid_http_url("http://"));
    CHECK_FALSE(tracker::is_valid_http_url("ftp://example.com"));
    CHECK_FALSE(tracker::is_valid_http_url("file:///etc/passwd"));
    CHECK_FALSE(tracker::is_valid_http_url("javascript:alert(1)"));
    CHECK_FALSE(tracker::is_valid_http_url("https://example.com/test\n"));
    CHECK_FALSE(tracker::is_valid_http_url("https://example.com/test\r"));
    CHECK_FALSE(tracker::is_valid_http_url("https://example.com/test with spaces"));
    CHECK_FALSE(tracker::is_valid_http_url("https://example.com; rm -rf /"));
    CHECK_FALSE(tracker::is_valid_http_url("https://example.com\"test\""));
    CHECK_FALSE(tracker::is_valid_http_url("https://example.com'test'"));
    CHECK_FALSE(tracker::is_valid_http_url("https://example.com`test`"));
}

TEST_CASE("Resources UI navigation and URL opener mock") {
    TempFileGuard guard("study_tracker_test_resources.json");
    auto roadmap = make_sample_roadmap();
    REQUIRE(tracker::save_roadmap(roadmap, guard.path));

    std::string opened_url;
    int open_count = 0;
    auto mock_opener = [&](const std::string& url) -> bool {
        opened_url = url;
        open_count++;
        return true;
    };

    auto screen = ftxui::ScreenInteractive::FitComponent();
    tracker::UIController controller(roadmap, guard.path, screen, mock_opener);
    auto component = controller.CreateComponent();

    using namespace ftxui;

    CHECK(controller.get_focus_section() == tracker::FocusSection::Tasks);

    CHECK(component->OnEvent(Event::Tab));
    CHECK(controller.get_focus_section() == tracker::FocusSection::Resources);

    REQUIRE(roadmap.blocks[0].resources.size() == 2);
    CHECK(controller.get_selected_resource_index() == 0);
    CHECK_FALSE(roadmap.blocks[0].resources[0].watched);

    component->OnEvent(Event::Character('x'));
    CHECK(roadmap.blocks[0].resources[0].watched);

    component->OnEvent(Event::Character(' '));
    CHECK_FALSE(roadmap.blocks[0].resources[0].watched);

    component->OnEvent(Event::Character('j'));
    CHECK(controller.get_selected_resource_index() == 1);

    component->OnEvent(Event::Character('o'));
    CHECK(open_count == 1);
    CHECK(opened_url == roadmap.blocks[0].resources[1].url);

    component->OnEvent(Event::Return);
    CHECK(open_count == 2);
    CHECK(opened_url == roadmap.blocks[0].resources[1].url);

    // Block 1 has empty resources: safe no-op
    controller.switch_to_block(1);
    CHECK(controller.get_selected_resource_index() == 0);
    controller.set_focus_section(tracker::FocusSection::Resources);
    open_count = 0;
    component->OnEvent(Event::Character('o'));
    CHECK(open_count == 0);
    component->OnEvent(Event::Character('x'));

    // Persistence check
    controller.switch_to_block(0);
    controller.set_focus_section(tracker::FocusSection::Resources);
    component->OnEvent(Event::Character('x'));
    controller.trigger_save();

    const auto reloaded = tracker::load_roadmap(guard.path);
    CHECK(reloaded.blocks[0].resources[0].watched);
}

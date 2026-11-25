#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>

#include "model.hpp"
#include "storage.hpp"
#include "ui.hpp"

namespace {

constexpr std::string_view kAppVersion = "1.0.0";

std::atomic<int> g_signal_received{0};
static_assert(std::atomic<int>::is_always_lock_free,
              "std::atomic<int> must be lock-free for async-signal-safe signal handling");

void signal_handler(int signum) {
    g_signal_received.store(signum, std::memory_order_relaxed);
}

void print_usage(std::string_view program_name) {
    std::cout << "Study Tracker TUI v" << kAppVersion << "\n\n"
              << "Uso:\n"
              << "  " << program_name << " [opzioni] [percorso/a/roadmap.json]\n\n"
              << "Opzioni:\n"
              << "  -h, --help     Mostra questo messaggio di aiuto ed esce\n"
              << "  -v, --version  Mostra la versione ed esce\n";
}

std::optional<std::filesystem::path> find_default_roadmap(const char* argv0) {
    const std::filesystem::path p1 = "./roadmap_matematica2.json";
    if (std::filesystem::exists(p1)) {
        return p1;
    }

    if (argv0 != nullptr) {
        const std::filesystem::path exe_dir = std::filesystem::path(argv0).parent_path();
        if (!exe_dir.empty()) {
            const std::filesystem::path p2 = exe_dir / "roadmap_matematica2.json";
            if (std::filesystem::exists(p2)) {
                return p2;
            }
            const std::filesystem::path p3 = exe_dir.parent_path() / "roadmap_matematica2.json";
            if (std::filesystem::exists(p3)) {
                return p3;
            }
        }
    }

    return std::nullopt;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const std::string_view program_name = (argc > 0 && argv[0] != nullptr) ? argv[0] : "study_tracker";
        std::optional<std::filesystem::path> cli_path;

        for (int i = 1; i < argc; ++i) {
            const std::string_view arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                print_usage(program_name);
                return 0;
            }
            if (arg == "-v" || arg == "--version") {
                std::cout << "study_tracker " << kAppVersion << "\n";
                return 0;
            }
            if (!cli_path) {
                cli_path = std::filesystem::path(arg);
            }
        }

        const std::filesystem::path path = cli_path.has_value()
            ? *cli_path
            : find_default_roadmap(argc > 0 ? argv[0] : nullptr).value_or(std::filesystem::path("./roadmap_matematica2.json"));

        if (!std::filesystem::exists(path)) {
            std::cerr << "Errore: File non trovato al percorso '" << path.string() << "'\n";
            std::cerr << "Uso: " << program_name << " [percorso/a/roadmap.json]\n";
            return 1;
        }

        tracker::Roadmap roadmap = tracker::load_roadmap(path);

        auto screen = ftxui::ScreenInteractive::Fullscreen();
        tracker::UIController controller(roadmap, path, screen);

        g_signal_received.store(0, std::memory_order_relaxed);
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
#if defined(SIGHUP)
        std::signal(SIGHUP, signal_handler);
#endif

        // Dedicated background timer & signal-polling thread
        std::thread timer_thread([&controller, &screen]() {
            auto last_tick = std::chrono::steady_clock::now();
            while (controller.is_app_running()) {
                if (g_signal_received.load(std::memory_order_relaxed) != 0) {
                    controller.stop_app();
                    screen.ExitLoopClosure()();
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick).count() >= 1000) {
                    last_tick = now;
                    screen.Post([&controller]() {
                        controller.tick_timer();
                    });
                    screen.PostEvent(ftxui::Event::Custom);
                }
            }
        });

        // Run UI Loop
        auto component = controller.CreateComponent();
        screen.Loop(component);

        // Cleanup on exit
        controller.stop_app();
        if (timer_thread.joinable()) {
            timer_thread.join();
        }

        // Final atomic save
        controller.trigger_save();

    } catch (const std::exception& e) {
        std::cerr << "Errore fatale: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

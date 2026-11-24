#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <functional>

#include "model.hpp"
#include "storage.hpp"
#include "ui.hpp"

namespace {
    std::function<void()> g_exit_closure;
    void signal_handler(int signum) {
        (void)signum;
        if (g_exit_closure) {
            g_exit_closure();
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        std::filesystem::path path;
        if (argc > 1) {
            path = argv[1];
        } else {
            // Cerca in ordine: working directory attuale, cartella del binario, parent del binario (es. build/..)
            std::filesystem::path p1 = "./roadmap_matematica2.json";
            std::filesystem::path exe_dir = std::filesystem::path(argv[0]).parent_path();
            std::filesystem::path p2 = exe_dir / "roadmap_matematica2.json";
            std::filesystem::path p3 = exe_dir.parent_path() / "roadmap_matematica2.json";

            if (std::filesystem::exists(p1)) {
                path = p1;
            } else if (std::filesystem::exists(p2)) {
                path = p2;
            } else if (std::filesystem::exists(p3)) {
                path = p3;
            } else {
                path = p1; // Lascia p1 per segnalare l'errore
            }
        }

        if (!std::filesystem::exists(path)) {
            std::cerr << "Errore: File non trovato al percorso '" << path.string() << "'\n";
            std::cerr << "Uso: " << argv[0] << " [percorso/a/roadmap.json]\n";
            return 1;
        }

        tracker::Roadmap roadmap = tracker::load_roadmap(path);

        auto screen = ftxui::ScreenInteractive::Fullscreen();
        tracker::UIController controller(roadmap, path, screen);

        g_exit_closure = [&]() {
            controller.stop_app();
            screen.ExitLoopClosure()();
        };

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
#if defined(SIGHUP)
        std::signal(SIGHUP, signal_handler);
#endif

        // Dedicated background timer thread
        std::thread timer_thread([&controller, &screen]() {
            auto last_tick = std::chrono::steady_clock::now();
            while (controller.is_app_running()) {
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

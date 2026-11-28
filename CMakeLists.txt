cmake_minimum_required(VERSION 3.16)
project(study_tracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Allow older third-party FetchContent dependencies (e.g. doctest) on CMake >= 4.0
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

# Fetch FTXUI
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0
)

# Fetch nlohmann/json
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

# Fetch doctest
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11
)

FetchContent_MakeAvailable(ftxui json doctest)

add_library(tracker_lib STATIC
  src/date_utils.cpp
  src/navigation_controller.cpp
  src/persistence_controller.cpp
  src/sound_player.cpp
  src/stats.cpp
  src/storage.cpp
  src/timer_controller.cpp
  src/ui.cpp
  src/ui_renderer.cpp
  src/url_opener.cpp
)

target_include_directories(tracker_lib PUBLIC src)

target_link_libraries(tracker_lib PUBLIC
  ftxui::screen
  ftxui::dom
  ftxui::component
  nlohmann_json::nlohmann_json
)

add_executable(study_tracker
  src/main.cpp
)

target_link_libraries(study_tracker PRIVATE tracker_lib)

add_executable(tracker_tests
  tests/test_tracker.cpp
)

target_link_libraries(tracker_tests PRIVATE
  tracker_lib
  doctest::doctest
)

target_compile_definitions(tracker_tests PRIVATE
  TRACKER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MSVC)
  target_compile_options(tracker_lib PRIVATE /W4)
  target_compile_options(study_tracker PRIVATE /W4)
  target_compile_options(tracker_tests PRIVATE /W4)
else()
  target_compile_options(tracker_lib PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(study_tracker PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(tracker_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(
  NAME tracker_tests
  COMMAND tracker_tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

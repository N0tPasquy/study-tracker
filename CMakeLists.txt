cmake_minimum_required(VERSION 3.16)
project(study_tracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

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

FetchContent_MakeAvailable(ftxui json)

add_executable(study_tracker
  src/main.cpp
  src/storage.cpp
  src/ui.cpp
  src/url_opener.cpp
)

target_include_directories(study_tracker PRIVATE src)

target_link_libraries(study_tracker PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
  nlohmann_json::nlohmann_json
)

add_executable(tracker_tests
  tests/test_tracker.cpp
  src/storage.cpp
  src/ui.cpp
  src/url_opener.cpp
)

target_include_directories(tracker_tests PRIVATE src)

target_link_libraries(tracker_tests PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
  nlohmann_json::nlohmann_json
)

if(MSVC)
  target_compile_options(study_tracker PRIVATE /W4)
  target_compile_options(tracker_tests PRIVATE /W4)
else()
  target_compile_options(study_tracker PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(tracker_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

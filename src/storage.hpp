#pragma once

#include "model.hpp"
#include <filesystem>
#include <string>

namespace tracker {

/// Loads the roadmap from the given JSON file path. Throws std::runtime_error on failure.
[[nodiscard]] Roadmap load_roadmap(const std::filesystem::path& file_path);

/// Atomically saves the roadmap to the given JSON file path (via .tmp file and rename).
[[nodiscard]] bool save_roadmap(const Roadmap& roadmap, const std::filesystem::path& file_path);

} // namespace tracker

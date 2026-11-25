#pragma once

#include "model.hpp"
#include <filesystem>

namespace tracker {

/// Manages roadmap persistence state (dirty flag and atomic saves).
class PersistenceController {
public:
    explicit PersistenceController(std::filesystem::path file_path);

    /// Saves the roadmap atomically; updates saved status only on success.
    [[nodiscard]] bool save(const Roadmap& roadmap);

    void mark_dirty() { data_saved_ = false; }

    [[nodiscard]] bool is_saved() const { return data_saved_; }
    [[nodiscard]] const std::filesystem::path& file_path() const { return file_path_; }

private:
    std::filesystem::path file_path_;
    bool data_saved_ = true;
};

} // namespace tracker

#include "persistence_controller.hpp"
#include "storage.hpp"

#include <utility>

namespace tracker {

PersistenceController::PersistenceController(std::filesystem::path file_path)
    : file_path_(std::move(file_path)) {}

bool PersistenceController::save(const Roadmap& roadmap) {
    if (!save_roadmap(roadmap, file_path_)) {
        data_saved_ = false;
        return false;
    }
    data_saved_ = true;
    return true;
}

} // namespace tracker

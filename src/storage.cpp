#include "storage.hpp"
#include <fstream>
#include <iostream>
#include <system_error>

namespace tracker {

Roadmap load_roadmap(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) {
        throw std::runtime_error("File non trovato: " + file_path.string());
    }

    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Impossibile aprire il file in lettura: " + file_path.string());
    }

    nlohmann::json j;
    ifs >> j;
    return j.get<Roadmap>();
}

bool save_roadmap(const Roadmap& roadmap, const std::filesystem::path& file_path) {
    try {
        std::filesystem::path tmp_path = file_path.string() + ".tmp";
        {
            std::ofstream ofs(tmp_path, std::ios::out | std::ios::trunc);
            if (!ofs.is_open()) {
                std::cerr << "Errore: impossibile creare file temporaneo " << tmp_path << "\n";
                return false;
            }

            nlohmann::json j = roadmap;
            ofs << j.dump(2) << "\n";
            ofs.flush();
            if (!ofs.good()) {
                std::cerr << "Errore durante la scrittura su " << tmp_path << "\n";
                return false;
            }
        }

        std::error_code ec;
        std::filesystem::rename(tmp_path, file_path, ec);
        if (ec) {
            std::cerr << "Errore nella rinomina del file temporaneo: " << ec.message() << "\n";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Eccezione durante il salvataggio: " << e.what() << "\n";
        return false;
    }
}

} // namespace tracker

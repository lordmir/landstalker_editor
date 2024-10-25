#include <landstalker/misc/include/Labels.h>

// Static member definitions
std::map<int, std::string> Labels::m_rooms;
std::map<int, std::string> Labels::m_bgms;

void Labels::loadData(const std::string& filename) {
    try {
        YAML::Node config = YAML::LoadFile(filename);
        
        // Load rooms
        if (config["rooms"]) {  // Check if "rooms" node exists
            try {
                for (const auto& room : config["rooms"]) {
                    m_rooms[room.first.as<int>()] = room.second.as<std::string>();
                }
            } catch (const YAML::Exception& e) {
                std::cerr << "Error parsing rooms in YAML: " << e.what() << std::endl;
            }
        }

        // Load bgms
        if (config["bgms"]) {  // Check if "bgms" node exists
            try {
                for (const auto& entity : config["bgms"]) {
                    m_bgms[entity.first.as<int>()] = entity.second.as<std::string>();
                }
            } catch (const YAML::Exception& e) {
                std::cerr << "Error parsing bgms in YAML: " << e.what() << std::endl;
            }
        }
        
    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading YAML file '" << filename << "': " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error accessing file '" << filename << "': " << e.what() << std::endl;
    }
}

std::optional<std::string> Labels::GetRoom(int id) {
    auto it = m_rooms.find(id);
    if (it != m_rooms.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string> Labels::GetBgm(int id) {
    auto it = m_bgms.find(id);
    if (it != m_bgms.end()) {
        return it->second;
    }
    return std::nullopt;
}

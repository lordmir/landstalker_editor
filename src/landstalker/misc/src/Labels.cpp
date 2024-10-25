#include <landstalker/misc/include/Labels.h>

// Static member definitions
std::map<int, std::string> Labels::m_rooms;
std::map<int, std::string> Labels::m_entities;
std::map<int, std::string> Labels::m_behaviours;
std::map<int, std::string> Labels::m_bgms;

void Labels::loadData(const std::string& filename) {
    YAML::Node config = YAML::LoadFile(filename);
    // Load rooms
    for (const auto& room : config["rooms"]) {
        m_rooms[room.first.as<int>()] = room.second.as<std::string>();
    }
    // Load entities
    for (const auto& entity : config["entities"]) {
        m_entities[entity.first.as<int>()] = entity.second.as<std::string>();
    }
    // Load behaviours
    for (const auto& entity : config["behaviours"]) {
        m_behaviours[entity.first.as<int>()] = entity.second.as<std::string>();
    }

    // Load bgms
    for (const auto& entity : config["bgms"]) {
        m_bgms[entity.first.as<int>()] = entity.second.as<std::string>();
    }
}

std::optional<std::string> Labels::GetRoom(int id) {
    auto it = m_rooms.find(id);
    if (it != m_rooms.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string> Labels::GetEntity(int id) {
    auto it = m_entities.find(id);
    if (it != m_entities.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string> Labels::GetBehaviour(int id) {
    auto it = m_behaviours.find(id);
    if (it != m_behaviours.end()) {
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

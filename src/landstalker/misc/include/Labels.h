#ifndef LABELS_H_
#define LABELS_H_

#include <yaml-cpp/yaml.h>
#include <optional>
#include <iostream>

class Labels {
public:
    static void loadData(const std::string& filename);
    static std::optional<std::string> GetRoom(int id);
    static std::optional<std::string> GetBgm(int id);

private:
    static std::map<int, std::string> m_rooms;
    static std::map<int, std::string> m_bgms;
};

#endif // LABELS_H_
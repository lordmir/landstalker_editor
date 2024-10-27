// Labels.h
#ifndef LABELS_H_
#define LABELS_H_

#include <yaml-cpp/yaml.h>
#include <optional>
#include <iostream>
#include <map>
#include <string>
#include <utility>

class Labels {
public:
    static void LoadData(const std::string& filename);
    static std::optional<std::string> GetFromId(const std::string& what, int id);

private:
    static std::map<std::pair<std::string, int>, std::string> m_data;
};

#endif // LABELS_H_

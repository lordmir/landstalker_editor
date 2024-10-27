// Labels.cpp
#include <landstalker/misc/include/Labels.h>

std::map<std::pair<std::string, int>, std::string> Labels::m_data;

void Labels::LoadData(const std::string& filename) {
    try {
        YAML::Node config = YAML::LoadFile(filename);
        
        // Iterate through all top-level keys in the YAML
        for (const auto& category : config) {
            const std::string& category_name = category.first.as<std::string>();
            try {
                // For each entry in this category
                for (const auto& entry : category.second) {
                    m_data[{category_name, entry.first.as<int>()}] = entry.second.as<std::string>();
                }
            } catch (const YAML::Exception& e) {
                std::cerr << "Error parsing " << category_name << " in YAML: " << e.what() << std::endl;
            }
        }
    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading YAML file '" << filename << "': " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error accessing file '" << filename << "': " << e.what() << std::endl;
    }
}

std::optional<std::string> Labels::GetFromId(const std::string& what, int id) {
    auto it = m_data.find({what, id});
    if (it != m_data.end()) {
        return it->second;
    }
    return std::nullopt;
}

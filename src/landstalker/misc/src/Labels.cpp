#include <landstalker/misc/include/DefaultLabels.h>
#include <landstalker/misc/include/Labels.h>
#include <landstalker/misc/include/Utils.h>
#include <cwctype>
#include <codecvt>
#include <fstream>

std::map<std::pair<std::wstring, int>, std::wstring> Labels::m_data;

void Labels::InitDefaults()
{
    m_data = DefaultLabels::DEFAULT_LABELS;
}

void Labels::LoadData(const std::string& filename) {
    try {
        InitDefaults();
        YAML::Node config = YAML::LoadFile(filename);
        std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
        
        // Iterate through all top-level keys in the YAML
        for (const auto& category : config) {
            const std::wstring& category_name = cvt.from_bytes(category.first.as<std::string>());
            try {
                // For each entry in this category
                for (const auto& entry : category.second) {
                    m_data[{category_name, entry.first.as<int>()}] = cvt.from_bytes(entry.second.as<std::string>());
                }
            } catch (const YAML::Exception& e) {
                Debug("Error parsing " + cvt.to_bytes(category_name) + " in YAML: " + e.what());
            }
        }
    } catch (const YAML::Exception& e) {
        Debug("Error loading YAML file '" + filename + "': " + e.what());
    } catch (const std::exception& e) {
        Debug("Error accessing file '" + filename + "': " + e.what());
    }
}

void Labels::SaveData(const std::string& filename)
{
    std::wofstream ofs(filename, std::ios::binary | std::ios::out);
    ofs.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
    std::wstring category = L"";
    bool begin = true;
    for (const auto& label : m_data)
    {
        if (category != label.first.first)
        {
            category = label.first.first;
            if (begin)
            {
                begin = false;
            }
            else
            {
                ofs << std::endl;
            }
            ofs << category << L":" << std::endl;
        }
        ofs << L"    " << label.first.second << L": \"" << label.second << "\"" << std::endl;
    }
}

std::optional<std::wstring> Labels::Get(const std::wstring& what, int id) {
    auto it = m_data.find({what, id});
    if (it != m_data.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool Labels::Update(const std::wstring& what, int id, const std::wstring& updated)
{
    if (m_data.at({ what, id }) == updated)
    {
        // No update needed
        return true;
    }
    auto it = FindMapKey(m_data, updated);
    if (it != m_data.end())
    {
        Debug("Duplicate label found");
        return false;
    }
    m_data[{what, id}] = updated;
    return true;
}

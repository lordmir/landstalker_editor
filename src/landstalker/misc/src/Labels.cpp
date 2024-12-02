#include <landstalker/misc/include/DefaultLabels.h>
#include <landstalker/misc/include/Labels.h>
#include <landstalker/misc/include/Utils.h>
#include <cwctype>
#include <codecvt>
#include <fstream>
#include <unordered_map>

std::map<std::pair<std::wstring, int>, std::wstring> Labels::m_data;
static const std::unordered_map<wchar_t, char> ESCAPES
{
    {L' ', '_'}, {L'/', '@'}
};

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
                    if (!Update(category_name, entry.first.as<int>(), cvt.from_bytes(entry.second.as<std::string>())))
                    {
                        Debug("Adding label failed: " + cvt.to_bytes(category_name) + "," + std::to_string(entry.first.as<int>()) + ": " + entry.second.as<std::string>());
                    }
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
    if (!ToAsmFriendly(updated))
    {
        Debug("Invalid characters in name");
        return false;
    }
    if (m_data.find({ what, id }) == m_data.end())
    {
        Debug("Original string not found");
        return false;
    }
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

std::optional<std::string> Labels::ToAsmFriendly(const std::wstring& what)
{
    std::string output;
    for (const wchar_t chr : what)
    {
        if (std::iswalnum(chr))
        {
            output += static_cast<char>(chr);
        }
        else if (ESCAPES.find(chr) != ESCAPES.cend())
        {
            output += ESCAPES.at(chr);
        }
        else if (std::iswprint(chr))
        {
            output += StrPrintf("$%02X$", static_cast<unsigned int>(chr));
        }
        else
        {
            return std::nullopt;
        }
    }
    return output;
}

std::optional<std::wstring> Labels::FromAsmFriendly(const std::string& what)
{
    std::wstring output;
    std::string hex;
    std::size_t i = 0;
    while (i < what.size())
    {
        auto escape = FindMapKey(ESCAPES, what.at(i));
        wchar_t chr = static_cast<wchar_t>(what.at(i));
        if (escape != ESCAPES.cend())
        {
            output += escape->first;
            ++i;
        }
        else if(std::isalnum(chr))
        {
            output += chr;
            ++i;
        }
        else if (chr == L'$')
        {
            std::size_t end = what.find('$', i + 1);
            if (end == std::string::npos || end <= i + 2)
            {
                return std::nullopt;
            }
            try
            {
                wint_t code = static_cast<wint_t>(std::stoi(what.substr(i + 1, end - i - 1), nullptr, 16));
                if (std::iswprint(code))
                {
                    output += static_cast<wchar_t>(code);
                    i = end + 1;
                }
                else
                {
                    return std::nullopt;
                }
            }
            catch (const std::exception&)
            {
                return std::nullopt;
            }

        }
    }
    return output;
}

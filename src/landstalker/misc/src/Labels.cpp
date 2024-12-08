#include <landstalker/misc/include/Labels.h>
#include <landstalker/misc/include/DefaultLabels.h>
#include <landstalker/misc/include/Utils.h>
#include <cwctype>
#include <codecvt>
#include <fstream>
#include <functional>

std::map<std::pair<std::wstring, int>, std::wstring> Labels::m_data;

static const std::map<std::wstring, std::wstring> FORMAT_STRINGS
{
    {Labels::C_SPRITE_ANIMATIONS, L"0x%04X"},
    {Labels::C_SPRITE_FRAMES, L"0x%04X"},
    {Labels::C_BLOCKSETS, L"0x%06X"}
};

const std::wstring Labels::C_ROOMS(L"rooms");
const std::wstring Labels::C_BGMS(L"bgms");
const std::wstring Labels::C_SOUNDS(L"sounds");
const std::wstring Labels::C_ENTITIES(L"entities");
const std::wstring Labels::C_SPRITES(L"sprites");
const std::wstring Labels::C_SPRITE_FRAMES(L"sprite_frames");
const std::wstring Labels::C_SPRITE_ANIMATIONS(L"sprite_animations");
const std::wstring Labels::C_MAPS(L"maps");
const std::wstring Labels::C_ROOM_PALETTES(L"room_palettes");
const std::wstring Labels::C_HIGH_PALETTES(L"sprite_high_palettes");
const std::wstring Labels::C_LOW_PALETTES(L"sprite_low_palettes");
const std::wstring Labels::C_TILESETS(L"tilesets");
const std::wstring Labels::C_ANIM_TILESETS(L"animated_tilesets");
const std::wstring Labels::C_BLOCKSETS(L"blocksets");
const std::wstring Labels::C_FLAGS(L"flags");
const std::wstring Labels::C_BEHAVIOURS(L"behaviours");
const std::wstring Labels::C_SCRIPT(L"script");
const std::wstring Labels::C_CUTSCENE(L"cutscene");
const std::wstring Labels::C_CHARACTER(L"character");
const std::wstring Labels::C_GLOBAL_CHARACTER(L"global_character");

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
        const std::wstring fmt = (FORMAT_STRINGS.count(label.first.first) > 0) ? FORMAT_STRINGS.at(label.first.first) : L"%d";
        ofs << L"    " << StrWPrintf(fmt, label.first.second) << L": \"" << label.second << "\"" << std::endl;
    }
}

bool Labels::Exists(const std::wstring& what, int id)
{
    return m_data.find({what, id}) != m_data.cend() && IsExistingValid(m_data.at({what, id}));
}

std::optional<std::wstring> Labels::Get(const std::wstring& what, int id) {
    auto it = m_data.find({what, id});
    if (it != m_data.end() && IsExistingValid(it->second))
    {
        return it->second;
    }
    return std::nullopt;
}

bool Labels::IsValid(const std::wstring& what)
{
    if (!IsExistingValid(what))
    {
        return false;
    }
    for(const auto& lbl : m_data)
    {
        if (lbl.second == what)
        {
            Debug("Duplicate label found");
            return false;
        }
        if (lbl.second.length() > what.length() && lbl.second.rfind(what, 0) == 0)
        {
            if (lbl.second.at(what.length()) == L'/')
            {
                Debug("Label same name as subdirectory");
                return false;
            }
        }
    }
    return true;
}

bool Labels::Update(const std::wstring& what, int id, const std::wstring& updated)
{
    if (m_data.count({ what, id }) > 0 && m_data.at({ what, id }) == updated)
    {
        // No update needed
        return true;
    }
    if (!IsValid(updated))
    {
        return false;
    }
    m_data[{what, id}] = updated;
    return true;
}

bool Labels::IsExistingValid(const std::wstring& what)
{
    if (what.empty())
    {
        Debug("Empty string");
        return false;
    }
    if (what.front() == L'/' || what.back() == L'/')
    {
        Debug("Invalid Path");
        return false;
    }
    if (!std::all_of(what.cbegin(), what.cend(), std::iswprint))
    {
        Debug("Invalid characters");
        return false;
    }
    std::wstring tmp;
    std::wstringstream ss(what);
    std::vector<std::wstring> elems;
    while (std::getline(ss, tmp, L'/'))
    {
        elems.push_back(tmp);
    }
    for (const std::wstring& elem : elems)
    {
        if (elem.empty() || std::iswblank(elem.front()) || std::iswblank(elem.back()))
        {
            Debug("Invalid whitespace");
            return false;
        }
    }
    return true;
}

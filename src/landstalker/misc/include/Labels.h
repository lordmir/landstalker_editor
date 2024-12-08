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
    static void InitDefaults();
    static void LoadData(const std::string& filename);
    static void SaveData(const std::string& filename);
    static bool Exists(const std::wstring& what, int id);
    static std::optional<std::wstring> Get(const std::wstring& what, int id);
    static bool IsValid(const std::wstring& what);
    static bool Update(const std::wstring& category, int id, const std::wstring& updated);

    static const std::wstring C_ROOMS;
    static const std::wstring C_BGMS;
    static const std::wstring C_SOUNDS;
    static const std::wstring C_ENTITIES;
    static const std::wstring C_SPRITES;
    static const std::wstring C_SPRITE_FRAMES;
    static const std::wstring C_SPRITE_ANIMATIONS;
    static const std::wstring C_MAPS;
    static const std::wstring C_ROOM_PALETTES;
    static const std::wstring C_HIGH_PALETTES;
    static const std::wstring C_LOW_PALETTES;
    static const std::wstring C_TILESETS;
    static const std::wstring C_ANIM_TILESETS;
    static const std::wstring C_BLOCKSETS;
    static const std::wstring C_FLAGS;
    static const std::wstring C_BEHAVIOURS;
    static const std::wstring C_SCRIPT;
    static const std::wstring C_CUTSCENE;
    static const std::wstring C_CHARACTER;
    static const std::wstring C_GLOBAL_CHARACTER;
private:
    static bool IsExistingValid(const std::wstring& what);
    static std::map<std::pair<std::wstring, int>, std::wstring> m_data;
};

#endif // LABELS_H_

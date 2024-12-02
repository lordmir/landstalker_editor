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
    static std::optional<std::wstring> Get(const std::wstring& what, int id);
    static bool Update(const std::wstring& what, int id, const std::wstring& updated);
    static std::optional<std::string> ToAsmFriendly(const std::string& prefix, const std::wstring& what);
    static std::optional<std::wstring> FromAsmFriendly(const std::string& what);

private:
    static std::map<std::pair<std::wstring, int>, std::wstring> m_data;
};

#endif // LABELS_H_

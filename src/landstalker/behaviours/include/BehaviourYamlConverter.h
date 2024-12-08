#ifndef _BEHAVIOUR_YAML_CONVERTER_H_
#define _BEHAVIOUR_YAML_CONVERTER_H_

#include <landstalker/behaviours/include/Behaviours.h>
#include <string>
#include <vector>
#include <map>

class BehaviourYamlConverter
{
public:
    static std::vector<std::string> AllToYaml(const std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>>& behaviours);
    static std::string ToYaml(int id, const std::string& name, const std::vector<Behaviours::Command>& behaviour);
    static std::string ToYaml(const std::vector<Behaviours::Command>& behaviour);

    static std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>> AllFromYaml(const std::vector<std::string>& yamls);
    static std::vector<Behaviours::Command> FromYaml(const std::string& yaml, int& id, std::string& name);
    static std::vector<Behaviours::Command> FromYaml(const std::string& yaml);
};

#endif // _BEHAVIOUR_YAML_CONVERTER_H_

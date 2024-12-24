#include <landstalker/behaviours/include/BehaviourYamlConverter.h>
#include <landstalker/misc/include/Utils.h>
#include <landstalker/misc/include/Labels.h>
#include <yaml-cpp/yaml.h>

#include <sstream>
#include <numeric>
#include <iomanip>

std::string BehaviourYamlConverter::ToYaml(int id, const std::string& name, const std::vector<Behaviours::Command>& behaviour)
{
    std::ostringstream ss;
    ss << "Index: " << std::dec << id << std::endl;
    ss << "Name: " << name << std::endl;
    ss << ToYaml(behaviour);
    return ss.str();
}

static std::string ParamToYaml(const Behaviours::Parameter& param)
{
    const int intparam = std::holds_alternative<int>(std::get<1>(param)) ? std::get<int>(std::get<1>(param)) : 0;
    std::ostringstream ss;
    ss << std::setw(16) << std::left << (std::get<0>(param) + ": ");
    ss << std::setw(10) << std::left << std::visit([](const auto& v) { return std::to_string(v); }, std::get<1>(param));
    switch (std::get<2>(param))
    {
    case Behaviours::ParamType::SOUND:
        if (Labels::Exists(Labels::C_SOUNDS, intparam))
        {
            ss << "  # " << wstr_to_utf8(*Labels::Get(Labels::C_SOUNDS, intparam));
        }
        break;
    case Behaviours::ParamType::FLAG:
        if (Labels::Exists(Labels::C_FLAGS, intparam))
        {
            ss << "  # " << wstr_to_utf8(*Labels::Get(Labels::C_FLAGS, intparam));
        }
        break;
    case Behaviours::ParamType::LOW_CUTSCENE:
    case Behaviours::ParamType::HIGH_CUTSCENE:
        if (Labels::Exists(Labels::C_CUTSCENE, intparam))
        {
            ss << "  # " << wstr_to_utf8(*Labels::Get(Labels::C_CUTSCENE, intparam));
        }
        break;
    default:
        break;
    }
    return ss.str();
}

std::string BehaviourYamlConverter::ToYaml(const std::vector<Behaviours::Command>& behaviour)
{
    std::ostringstream ss;
    ss << "Script:" << std::endl;
    int line = 0;
    for (const auto& c : behaviour)
    {
        ss << "- " << std::setw(28) << std::left << (Behaviours::GetCommand(c.command).aliases[0] + (c.params.empty() ? "" : ":"));
        ss << "  " << "# <Command #" << std::to_string(++line) << ">" << std::endl;
        for (const auto& p : c.params)
        {
            ss << "    " << ParamToYaml(p) << std::endl;
        }
    }
    return ss.str();
}

std::vector<Behaviours::Command> BehaviourYamlConverter::FromYaml(const std::string& yaml, int& id, std::string& name)
{
    YAML::Node node = YAML::Load(yaml);
    id = node["Index"].as<int>();
    name = node["Name"].as<std::string>();
    return FromYaml(yaml);
}

std::vector<Behaviours::Command> BehaviourYamlConverter::FromYaml(const std::string& yaml)
{
    YAML::Node node = YAML::Load(yaml);
    if (node["Script"].IsDefined())
    {
        node = node["Script"];
    }
    std::vector<Behaviours::Command> cmds;
    int cmd_index = 1;
    for (const auto& c : node)
    {
        if (c.IsScalar())
        {
            const Behaviours::CommandDefinition cmddef = Behaviours::GetCommandByName(c.as<std::string>());
            if (!cmddef.params.empty())
            {
                std::string err("#" + std::to_string(cmd_index) + ": Expected parameters for command \"" + cmddef.aliases.front() + "\"");
                err += "\nThe following parameters are required: " + std::accumulate(cmddef.params.cbegin() + 1, cmddef.params.cend(),
                    std::string("\"") + cmddef.params.front().first + "\"", [](std::string r, const auto& s)
                    {
                        return std::move(r) + ", \"" + s.first + "\"";
                    });
                throw std::runtime_error(err);
            }
            cmds.push_back({ cmddef.id, {} });
        }
        else if (c.IsMap() && c.size() >= 1)
        {
            const Behaviours::CommandDefinition cmddef = Behaviours::GetCommandByName(c.begin()->first.as<std::string>());
            Behaviours::Command cmd;
            cmd.command = cmddef.id;
            std::vector<bool> params_set(cmddef.params.size());
            std::fill(params_set.begin(), params_set.end(), false);
            for (const auto& p : cmddef.params)
            {
                cmd.params.push_back({ p.first, -1, Behaviours::ParamType::NONE });
            }
            auto params = c.begin()->second;
            for (const auto& p : params)
            {
                const auto& pname = p.first.as<std::string>();
                auto pdef = std::find_if(cmddef.params.cbegin(), cmddef.params.cend(), [&pname](const auto& v)
                    {
                        return v.first == pname;
                    });
                if (pdef != cmddef.params.end())
                {
                    auto pindex = std::distance(cmddef.params.cbegin(), pdef);
                    params_set[pindex] = true;
                    std::get<2>(cmd.params[pindex]) = pdef->second;
                    switch (pdef->second)
                    {
                    case Behaviours::ParamType::COORDINATE:
                    case Behaviours::ParamType::LONG_COORDINATE:
                        std::get<1>(cmd.params[pindex]) = p.second.as<double>();
                        break;
                    default:
                        std::get<1>(cmd.params[pindex]) = p.second.as<int>();
                        break;
                    }
                }
                else
                {
                    std::string err("#" + std::to_string(cmd_index) + ": Bad parameter \"" + pname + "\" for command \"" + cmddef.aliases.front() + "\"");
                    err += "\nThe following parameters are required: " + std::accumulate(cmddef.params.cbegin() + 1, cmddef.params.cend(),
                        std::string("\"") + cmddef.params.front().first + "\"", [](std::string r, const auto& s)
                        {
                            return std::move(r) + ", \"" + s.first + "\"";
                        });
                    throw std::runtime_error(err);
                }
            }
            if (!std::all_of(params_set.begin(), params_set.end(), [](bool p) {return p; }))
            {
                std::string err("#" + std::to_string(cmd_index) + ": Expected " + std::to_string(cmddef.params.size()) +
                    " parameters for command \"" + cmddef.aliases.front() + "\"");
                err += "\nThe following parameters are required: " + std::accumulate(cmddef.params.cbegin() + 1, cmddef.params.cend(),
                    std::string("\"") + cmddef.params.front().first + "\"", [](std::string r, const auto& s)
                    {
                        return std::move(r) + ", \"" + s.first + "\"";
                    });
                throw std::runtime_error(err);
            }
            cmds.push_back(cmd);
        }
        else
        {
            throw std::runtime_error("#" + std::to_string(cmd_index) + ": Unexpected element in YAML: " + c.as<std::string>("?"));
        }
        ++cmd_index;
    }
    return cmds;
}

std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>> BehaviourYamlConverter::AllFromYaml(const std::vector<std::string>& yamls)
{
    std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>> behaviours;
    for (const auto& yaml : yamls)
    {
        int id = 0;
        std::string name;
        auto cmds = FromYaml(yaml, id, name);
        behaviours.emplace(id, std::make_pair(name, cmds));
    }
    return behaviours;
}

std::vector<std::string> BehaviourYamlConverter::AllToYaml(const std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>>& behaviours)
{
    std::vector<std::string> behaviours_yaml;

    for (const auto& b : behaviours)
    {
        behaviours_yaml.push_back(ToYaml(b.first, b.second.first, b.second.second));
    }
    return behaviours_yaml;
}

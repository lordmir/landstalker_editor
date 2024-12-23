#include <landstalker/script/include/ProgressFlags.h>
#include <landstalker/main/include/ScriptData.h>
#include <yaml-cpp/yaml.h>

ProgressFlags::Flags ProgressFlags::GetFlags(ScriptFunctionTable& table)
{
    Flags flags;
    auto func = table.GetMapping("GetFlagProgress");
    if (!func)
    {
        return flags;
    }
    uint8_t quest = 0;
    std::vector<uint16_t> flaglist;
    for (const auto& statement : func->statements)
    {
        if (std::holds_alternative<Statements::ProgressFlagMapping>(statement))
        {
            const auto& quest_progress = std::get<Statements::ProgressFlagMapping>(statement).mapping;
            for (const auto& flag : quest_progress)
            {
                if (flag.first > flaglist.size())
                {
                    flaglist.resize(flag.first, 0xFFFF);
                }
                flaglist[flag.first - 1] = flag.second;
            }
        }
        else if (std::holds_alternative<Statements::CustomAsm>(statement))
        {
            const auto& inss = std::get<Statements::CustomAsm>(statement);
            for (const auto& ins : inss.instructions)
            {
                if (ins.mnemonic == "addq" && ins.operands.size() == 2 && std::holds_alternative<AsmFile::Immediate>(ins.operands.at(0))
                    && std::holds_alternative<std::string>(ins.operands.at(1)))
                {
                    flags.emplace(quest, flaglist);
                    flaglist.clear();
                    const auto& reg = std::get<std::string>(ins.operands.at(1));
                    const auto& inc = std::get<AsmFile::Immediate>(ins.operands.at(0));
                    if (reg == "a0")
                    {
                        quest += static_cast<uint8_t>(inc);
                    }
                }
            }
        }
    }
    if (!flaglist.empty())
    {
        flags.emplace(quest, flaglist);
    }
    return flags;
}

ScriptFunctionTable ProgressFlags::MakeAsm(const ProgressFlags::Flags& flags)
{
    ScriptFunctionTable table;
    ScriptFunction func("GetFlagProgress", { Statements::CustomAsm(
        {
            AsmFile::Instruction("move", AsmFile::Width::L, {"a0", "-(sp)"}),
            AsmFile::Instruction("lea", AsmFile::Width::NONE, {"(g_GameFlagProgress1).l", "a0"})
        }) });

    uint8_t last_quest = flags.rbegin()->first;
    
    for (int quest = 0; quest <= last_quest; ++quest)
    {
        std::map<uint8_t, uint16_t, std::greater<uint8_t>> quest_flags;
        for (const auto& flag : flags)
        {
            if (flag.first == quest)
            {
                for (std::size_t i = 1; i <= flag.second.size(); ++i)
                {
                    if (flag.second[i - 1] != 0xFFFF)
                    {
                        quest_flags.emplace(static_cast<uint8_t>(i), flag.second[i - 1]);
                    }
                }
            }
        }
        func.statements.emplace_back(Statements::ProgressFlagMapping(std::move(quest_flags)));
        if (quest < last_quest)
        {
            func.statements.push_back(Statements::CustomAsm({AsmFile::Instruction("addq", AsmFile::Width::L, {AsmFile::Immediate(1), "a0"})}));
        }
    }
    func.statements.push_back(Statements::CustomAsm({ AsmFile::Instruction("movea", AsmFile::Width::L, {"(sp)+", "a0"}) }));
    func.statements.push_back(Statements::Rts());
    
    table.AddFunction(std::move(func));
    return table;
}

std::string ProgressFlags::ToYaml(ScriptFunctionTable& table)
{
    Flags flags(GetFlags(table));
    std::ostringstream ss;
    for (const auto& quest : flags)
    {
        ss << "- Quest: " << Hex(quest.first) << std::endl;
        ss << "  Flags: " << std::endl;
        for (std::size_t i = 0; i < quest.second.size(); ++i)
        {
            uint16_t flag = quest.second[i];
            ss << StrPrintf("  - 0x%04X   # %02d/%02d (%d%%) %s\n", flag, i + 1, quest.second.size(),
                static_cast<int>(static_cast<double>((i + 1) * 100)/static_cast<double>(quest.second.size())),
                    wstr_to_utf8(ScriptData::GetFlagDisplayName(flag)).c_str());
        }
    }
    return ss.str();
}

ScriptFunctionTable ProgressFlags::FromYaml(const std::string& yaml)
{
    Flags flags;
    YAML::Node node = YAML::Load(yaml);
    for (const auto& quest_flags : node)
    {
        uint8_t quest = quest_flags["Quest"].as<uint8_t>();
        std::vector<uint16_t> flaglist;
        for (const auto& f : quest_flags["Flags"])
        {
            flaglist.push_back(f.as<uint16_t>());
        }
        flags[quest] = flaglist;
    }
    return MakeAsm(flags);
}

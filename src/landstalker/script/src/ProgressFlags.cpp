#include <landstalker/script/include/ProgressFlags.h>

ProgressFlags::Flags ProgressFlags::GetFlags(ScriptFunctionTable& table)
{
    Flags flags;
    auto func = table.GetMapping("GetFlagProgress");
    if (!func)
    {
        return flags;
    }
    uint8_t quest = 0;
    for (const auto& statement : func->statements)
    {
        if (std::holds_alternative<Statements::ProgressFlagMapping>(statement))
        {
            const auto& quest_progress = std::get<Statements::ProgressFlagMapping>(statement).mapping;
            for (const auto& flag : quest_progress)
            {
                flags.emplace(Statements::ProgressList::QuestProgress(quest, flag.first), flag.second);
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
    return flags;
}

ScriptFunctionTable ProgressFlags::MakeAsm(const ProgressFlags::Flags& flags)
{
    ScriptFunctionTable table;
    ScriptFunction func("GetFlagProgress", {Statements::CustomAsm(
        "    move.l   a0,-(sp)\n"
        "    lea      (g_GameFlagProgress1).l,a0"
    )});

    uint8_t last_quest = flags.rbegin()->first.quest;
    
    for (int quest = 0; quest <= last_quest; ++quest)
    {
        std::map<uint8_t, uint16_t, std::greater<uint8_t>> quest_flags;
        for (const auto& flag : flags)
        {
            if (flag.first.quest == quest)
            {
                quest_flags.emplace(flag.first.progress, flag.second);
            }
        }
        func.statements.emplace_back(Statements::ProgressFlagMapping(std::move(quest_flags)));
        if (quest < last_quest)
        {
            func.statements.push_back(Statements::CustomAsm("    addq.l   #$01,a0"));
        }
    }
    func.statements.push_back(Statements::CustomAsm("    movea.l  (sp)+,a0"));
    func.statements.push_back(Statements::Rts());
    
    table.AddFunction(std::move(func));
    return table;
}

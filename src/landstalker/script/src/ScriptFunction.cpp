#include <landstalker/script/include/ScriptFunction.h>
#include <landstalker/misc/include/Literals.h>

YesNoPrompt::YesNoPrompt(AsmFile& file)
{
    AsmFile::ScriptAction s_prompt, s_on_yes, s_on_no;
    file >> s_prompt >> s_on_yes >> s_on_no;
    prompt = s_prompt;
    on_yes = s_on_yes;
    on_no = s_on_no;
}

YesNoPrompt::YesNoPrompt(const YAML::Node::const_iterator& it)
    : prompt((*it)["Question"].begin()),
      on_yes((*it)["OnYes"].begin()),
      on_no((*it)["OnNo"].begin())
{
}

void YesNoPrompt::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("bsr", AsmFile::Width::W, { "HandleYesNoPrompt" });
    prompt.ActionToAsm(file, 0);
    on_yes.ActionToAsm(file, 1);
    on_no.ActionToAsm(file, 2);
}

std::string YesNoPrompt::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- Question:" << std::endl;
    ss << prompt.ToYaml(indent + 6);
    ss << std::string(indent + 4, ' ') << "OnYes:" << std::endl;
    ss << on_yes.ToYaml(indent + 6);
    ss << std::string(indent + 4, ' ') << "OnNo:" << std::endl;
    ss << on_no.ToYaml(indent + 6);
    return ss.str();
}

std::string YesNoPrompt::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Yes/No Prompt:" << std::endl;
    ss << std::string(indent + 2, ' ') << "Prompt:" << std::endl;
    ss << prompt.Print(indent + 4);
    ss << std::string(indent + 2, ' ') << "On Yes:" << std::endl;
    ss << on_yes.Print(indent + 4);
    ss << std::string(indent + 2, ' ') << "On No:" << std::endl;
    ss << on_no.Print(indent + 4);
    return ss.str();
}

bool YesNoPrompt::IsEndOfFunction() const
{
    return false;
}

SetFlagOnTalk::SetFlagOnTalk(AsmFile& file)
{
    AsmFile::ScriptAction s_on_clear, s_on_set;
    file >> flag >> s_on_clear >> s_on_set;
    on_clear = s_on_clear;
    on_set = s_on_set;
}

SetFlagOnTalk::SetFlagOnTalk(const YAML::Node::const_iterator& it)
    : flag((*it)["SetFlag"].as<uint16_t>()),
      on_clear((*it)["OnClear"].begin()),
      on_set((*it)["OnSet"].begin())
{
}

void SetFlagOnTalk::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("bsr", AsmFile::Width::W, { "SetFlagBitOnTalking" }) << flag;
    on_clear.ActionToAsm(file, 1);
    on_set.ActionToAsm(file, 2);
}

std::string SetFlagOnTalk::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- SetFlag: " << Hex(flag) << std::endl;
    ss << std::string(indent + 4, ' ') << "OnClear:" << std::endl;
    ss << on_clear.ToYaml(indent + 6);
    ss << std::string(indent + 4, ' ') << "OnSet:" << std::endl;
    ss << on_set.ToYaml(indent + 6);
    return ss.str();
}

std::string SetFlagOnTalk::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Set Flag On Talk:" << std::endl;
    ss << std::string(indent + 2, ' ') << "Flag: " << Hex(flag) << std::endl;
    ss << std::string(indent + 2, ' ') << "On Clear:" << std::endl;
    ss << on_clear.Print(indent + 4);
    ss << std::string(indent + 2, ' ') << "On Set:" << std::endl;
    ss << on_set.Print(indent + 4);
    return ss.str();
}

bool SetFlagOnTalk::IsEndOfFunction() const
{
    return false;
}

IsFlagSet::IsFlagSet(AsmFile& file)
{
    AsmFile::ScriptAction s_on_clear, s_on_set;
    file >> flag >> s_on_set >> s_on_clear;
    on_clear = s_on_clear;
    on_set = s_on_set;
}

IsFlagSet::IsFlagSet(const YAML::Node::const_iterator& it)
    : flag((*it)["CheckFlag"].as<uint16_t>()),
      on_set((*it)["OnSet"].begin()),
      on_clear((*it)["OnClear"].begin())
{
}

void IsFlagSet::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("bsr", AsmFile::Width::W, { "CheckFlagAndDisplayMessage" }) << flag;
    on_set.ActionToAsm(file, 1);
    on_clear.ActionToAsm(file, 2);
}

std::string IsFlagSet::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- CheckFlag: " << Hex(flag) << std::endl;
    ss << std::string(indent + 4, ' ') << "OnClear:" << std::endl;
    ss << on_clear.ToYaml(indent + 6);
    ss << std::string(indent + 4, ' ') << "OnSet:" << std::endl;
    ss << on_set.ToYaml(indent + 6);
    return ss.str();
}

std::string IsFlagSet::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Check Flag:" << std::endl;
    ss << std::string(indent + 2, ' ') << "Flag: " << Hex(flag) << std::endl;
    ss << std::string(indent + 2, ' ') << "On Set:" << std::endl;
    ss << on_set.Print(indent + 4);
    ss << std::string(indent + 2, ' ') << "On Clear:" << std::endl;
    ss << on_clear.Print(indent + 4);
    return ss.str();
}

bool IsFlagSet::IsEndOfFunction() const
{
    return false;
}

PlaySound::PlaySound(AsmFile& file)
{
    file >> sound;
}

PlaySound::PlaySound(const YAML::Node::const_iterator& it)
    : sound((*it)["PlaySound"].as<uint16_t>())
{
}

void PlaySound::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("trap", AsmFile::Width::NONE, { "#$00" }) << sound;
}

std::string PlaySound::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- PlaySound: " << Hex(sound) << std::endl;
    return ss.str();
}

std::string PlaySound::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Play Sound: " << Hex(sound) << std::endl;
    return ss.str();
}

bool PlaySound::IsEndOfFunction() const
{
    return false;
}

CustomItemScript::CustomItemScript(AsmFile& file)
{
    file >> custom_item_script;
}

CustomItemScript::CustomItemScript(const YAML::Node::const_iterator& it)
    : custom_item_script((*it)["CustomItemAction"].as<uint16_t>())
{
}

void CustomItemScript::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("trap", AsmFile::Width::NONE, { "#$02" }) << custom_item_script;
}

std::string CustomItemScript::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- CustomItemAction: " << Hex(custom_item_script) << std::endl;
    return ss.str();
}

std::string CustomItemScript::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Custom Item Script: " << Hex(custom_item_script) << std::endl;
    return ss.str();
}

bool CustomItemScript::IsEndOfFunction() const
{
    return false;
}

Sleep::Sleep(AsmFile& file)
{
    file >> ticks;
}

Sleep::Sleep(const YAML::Node::const_iterator& it)
    : ticks((*it)["Sleep"].as<uint16_t>())
{
}

void Sleep::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("bsr", AsmFile::Width::W, { "Sleep_0" }) << ticks;
}

std::string Sleep::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- Sleep: " << ticks << std::endl;
    return ss.str();
}

std::string Sleep::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Sleep: " << ticks << " frames" << std::endl;
    return ss.str();
}

bool Sleep::IsEndOfFunction() const
{
    return false;
}

DisplayPrice::DisplayPrice(AsmFile& file)
{
    AsmFile::ScriptAction s_display_price;
    file >> s_display_price;
    display_price = s_display_price;
}

DisplayPrice::DisplayPrice(const YAML::Node::const_iterator& it)
    : display_price((*it)["DisplayPriceMessage"].begin())
{
}

void DisplayPrice::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("bsr", AsmFile::Width::W, { "Sleep_0" });
    display_price.ActionToAsm(file, 0);
}

std::string DisplayPrice::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Check Flag:" << std::endl;
    ss << display_price.ToYaml(indent + 4);
    return ss.str();
}

std::string DisplayPrice::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Display Price:" << std::endl;
    ss << std::string(indent + 2, ' ') << "Prompt:" << std::endl;
    ss << display_price.Print(indent + 4);
    return ss.str();
}

bool DisplayPrice::IsEndOfFunction() const
{
    return true;
}

Branch::Branch(const AsmFile::Instruction& ins)
{
    if (ins.mnemonic != "bra" || ins.operands.size() != 1 || !std::holds_alternative<std::string>(ins.operands.at(1)))
    {
        throw std::runtime_error("Branch: Unexpected instruction " + Trim(ins.ToLine()));
    }
    this->label = std::get<std::string>(ins.operands.at(0));
    this->wide = ins.width == AsmFile::Width::W;
}

Branch::Branch(const YAML::Node::const_iterator& it)
    : label((*it)["Branch"].as<std::string>()),
      wide((*it)["Wide"].IsDefined() && (*it)["Wide"].as<bool>())
{
}

void Branch::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("bra", wide ? AsmFile::Width::W : AsmFile::Width::S, { label });
}

std::string Branch::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- Branch: " << label << std::endl;
    if (wide)
    {
        ss << std::string(indent + 4, ' ') << "Wide: True" << std::endl;
    }
    return ss.str();
}

std::string Branch::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Branch: " << label << "  " << (wide ? "[Wide]" : "[Short]") << std::endl;
    return ss.str();
}

bool Branch::IsEndOfFunction() const
{
    return true;
}

ShopInteraction::ShopInteraction(AsmFile& file)
{
    AsmFile::ScriptAction s_on_sale_prompt, s_on_sale_confirm, s_on_no_money, s_on_sale_decline;
    file >> s_on_sale_prompt >> s_on_sale_confirm >> s_on_no_money >> s_on_sale_decline;
    on_sale_prompt = s_on_sale_prompt;
    on_sale_confirm = s_on_sale_confirm;
    on_no_money = s_on_no_money;
    on_sale_decline = s_on_sale_decline;
}

ShopInteraction::ShopInteraction(const YAML::Node::const_iterator& it)
    : on_sale_prompt((*it)["OnSalePrompt"].begin()),
      on_sale_confirm((*it)["OnSaleConfirm"].begin()),
      on_no_money((*it)["OnNoMoney"].begin()),
      on_sale_decline((*it)["OnSaleDecline"].begin())
{
}

void ShopInteraction::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("bsr", AsmFile::Width::W, { "HandleShopInterraction" });
    on_sale_prompt.ActionToAsm(file, 0);
    on_sale_confirm.ActionToAsm(file, 1);
    on_no_money.ActionToAsm(file, 2);
    on_sale_decline.ActionToAsm(file, 3);
}

std::string ShopInteraction::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- Shop:" <<  std::endl;
    ss << std::string(indent + 4, ' ') << "OnSalePrompt:" << std::endl;
    ss << on_sale_prompt.ToYaml(indent + 6);
    ss << std::string(indent + 4, ' ') << "OnSaleConfirm:" << std::endl;
    ss << on_sale_confirm.ToYaml(indent + 6);
    ss << std::string(indent + 4, ' ') << "OnNoMoney:" << std::endl;
    ss << on_no_money.ToYaml(indent + 6);
    ss << std::string(indent + 4, ' ') << "OnSaleDecline:" << std::endl;
    ss << on_sale_decline.ToYaml(indent + 6);
    return ss.str();
}

std::string ShopInteraction::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Shop:" << std::endl;
    ss << std::string(indent + 2, ' ') << "On Sale Prompt:" << std::endl;
    ss << on_sale_prompt.Print(indent + 4);
    ss << std::string(indent + 2, ' ') << "On Sale Confirm:" << std::endl;
    ss << on_sale_confirm.Print(indent + 4);
    ss << std::string(indent + 2, ' ') << "On Sale Decline:" << std::endl;
    ss << on_sale_decline.Print(indent + 4);
    ss << std::string(indent + 2, ' ') << "On Not Enough Money:" << std::endl;
    ss << on_no_money.Print(indent + 4);
    return ss.str();
}

bool ShopInteraction::IsEndOfFunction() const
{
    return true;
}

ChurchInteraction::ChurchInteraction(AsmFile& file)
{
    AsmFile::ScriptAction s_script_normal_priest, s_script_skeleton_priest;
    file >> s_script_normal_priest >> s_script_skeleton_priest;
    script_normal_priest = s_script_normal_priest;
    script_skeleton_priest = s_script_skeleton_priest;
}

ChurchInteraction::ChurchInteraction(const YAML::Node::const_iterator& it)
    : script_normal_priest((*it)["NormalPriest"].begin()),
      script_skeleton_priest((*it)["SkeletonPriest"].begin())
{
}

void ChurchInteraction::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("bsr", AsmFile::Width::S, { "HandleChurchInterraction" });
    script_normal_priest.ActionToAsm(file, 0);
    script_skeleton_priest.ActionToAsm(file, 1);
}

std::string ChurchInteraction::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- Church:" << std::endl;
    ss << std::string(indent + 4, ' ') << "NormalPriest:" << std::endl;
    ss << script_normal_priest.ToYaml(indent + 6);
    ss << std::string(indent + 4, ' ') << "SkeletonPriest:" << std::endl;
    ss << script_skeleton_priest.ToYaml(indent + 6);
    return ss.str();
}

std::string ChurchInteraction::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Church:" << std::endl;
    ss << std::string(indent + 2, ' ') << "Normal Priest:" << std::endl;
    ss << script_normal_priest.Print(indent + 4);
    ss << std::string(indent + 2, ' ') << "Skeleton Priest:" << std::endl;
    ss << script_skeleton_priest.Print(indent + 4);
    return ss.str();
}

bool ChurchInteraction::IsEndOfFunction() const
{
    return false;
}

Rts::Rts()
{
}

Rts::Rts(const YAML::Node::const_iterator& /*it*/)
{
}

void Rts::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("rts");
}

std::string Rts::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- Return" << std::endl;
    return ss.str();
}

std::string Rts::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Return" << std::endl;
    return ss.str();
}

bool Rts::IsEndOfFunction() const
{
    return true;
}

CustomAsm::CustomAsm(const AsmFile::Instruction& ins)
{
    Append(ins);
}

CustomAsm::CustomAsm(const YAML::Node::const_iterator& it)
{
    std::stringstream ss((*it)["Asm"].as<std::string>());
    std::string line;
    while (std::getline(ss, line))
    {
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }
        instructions.push_back(AsmFile::Instruction::FromLine("\t" + line));
    }
}

void CustomAsm::Append(const AsmFile::Instruction& ins)
{
    instructions.push_back(ins);
}

void CustomAsm::ToAsm(AsmFile& file) const
{
    for (const auto& ins : instructions)
    {
        file << ins;
    }
}

std::string CustomAsm::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- Asm: |" << std::endl;
    for (const auto& line : instructions)
    {
        ss << std::string(indent + 6, ' ') << Trim(line.ToLine()) << std::endl;
    }
    return ss.str();
}

std::string CustomAsm::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Custom Assembler:" << std::endl;
    for (const auto& line : instructions)
    {
        ss << std::string(indent + 2, ' ') << Trim(line.ToLine()) << std::endl;
    }
    return ss.str();
}

bool CustomAsm::IsEndOfFunction() const
{
    return false;
}

ProgressList::ProgressList(AsmFile& file)
{
    AsmFile::ScriptAction action;
    uint8_t quest = 0;
    uint8_t progress_count = 0;
    while (true)
    {
        file >> quest >> progress_count;
        if (quest == 0xFF)
        {
            break;
        }
        file >> action;
        progress.emplace(std::pair{Flag(quest, progress_count), action });
    }
}

ProgressList::ProgressList(const YAML::Node::const_iterator& it)
{
    for (const auto& elem : (*it)["ProgressDependent"])
    {
        uint8_t quest = elem["Quest"].as<uint8_t>();
        uint8_t prog = elem["Progress"].as<uint8_t>();
        auto action = Action(elem["Action"].begin());
        progress.insert({ Flag(quest, prog), action});
    }
}

void ProgressList::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("bsr", AsmFile::Width::W, { "HandleProgressDependentDialogue" });
    std::size_t offset = 1;
    for (const auto& p : progress)
    {
        file << std::vector<uint8_t>{p.first.quest, p.first.progress};
        p.second.ActionToAsm(file, offset);
        offset += 2;
    }
    file << 0xFFFF_u16;
}

std::string ProgressList::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- ProgressDependent:" << std::endl;
    for (const auto& line : progress)
    {
        ss << std::string(indent + 4, ' ') << "- Quest: " << static_cast<int>(line.first.quest) << std::endl;
        ss << std::string(indent + 6, ' ') << "Progress: " << static_cast<int>(line.first.progress) << std::endl;
        ss << std::string(indent + 6, ' ') << "Action:" << std::endl;
        ss << line.second.ToYaml(indent + 6);
    }
    return ss.str();
}

std::string ProgressList::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "ProgressList:" << std::endl;
    for (const auto& prog : progress)
    {
        ss << std::string(indent + 2, ' ') << "Quest: " << static_cast<int>(prog.first.quest)
           << ", Progress: " << static_cast<int>(prog.first.progress) << ":" << std::endl;
        ss << prog.second.Print(indent + 4);
    }
    return ss.str();
}

bool ProgressList::IsEndOfFunction() const
{
    return true;
}

ProgressFlagMapping::ProgressFlagMapping(AsmFile& file)
{
    uint16_t flag;
    uint8_t dummy, progress;
    while (true)
    {
        file >> flag;
        if (flag == 0xFFFF)
        {
            break;
        }
        file >> progress >> dummy;
        this->mapping[progress] = flag;
    }
}

ProgressFlagMapping::ProgressFlagMapping(const YAML::Node::const_iterator& it)
{
    for (const auto& elem : (*it)["QuestProgress"])
    {
        uint16_t flag = elem["OnFlagSet"].as<uint16_t>();
        uint8_t progress = elem["SetProgress"].as<uint8_t>();
        mapping.insert({ progress, flag });
    }
}

void ProgressFlagMapping::ToAsm(AsmFile& file) const
{
    for (const auto& p : mapping)
    {
        file << std::vector<uint16_t>{p.second, static_cast<uint16_t>(p.first << 8)};
    }
    file << 0xFFFF_u16;
}

std::string ProgressFlagMapping::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- QuestProgress:" << std::endl;
    for (const auto& line : mapping)
    {
        ss << std::string(indent + 4, ' ') << "- OnFlagSet: " << Hex(line.second) << std::endl;
        ss << std::string(indent + 6, ' ') << "SetProgress: " << static_cast<int>(line.first) << std::endl;
    }
    return ss.str();
}

std::string ProgressFlagMapping::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Progress Flag Mapping:" << std::endl;
    for (const auto& flag : mapping)
    {
        ss << std::string(indent + 2, ' ') << "Progress: " << static_cast<int>(flag.first)
           << " -> Flag: " << Hex(flag.second) << std::endl;
    }
    return ss.str();
}

bool ProgressFlagMapping::IsEndOfFunction() const
{
    return false;
}

ActionTable::ActionTable(AsmFile& file)
{
    AsmFile::ScriptAction action;
    while (file.Read(action))
    {
        actions.push_back(action);
        if (file.IsLabel())
        {
            break;
        }
    }
}

ActionTable::ActionTable(const YAML::Node::const_iterator& it)
{
    for (auto ait = (*it)["ActionTable"].begin(); it != (*it)["ActionTable"].end(); ++ait)
    {
        actions.push_back(Action(ait));
    }
}

void ActionTable::ToAsm(AsmFile& file) const
{
    std::size_t counter = 0;
    for (const auto& action : actions)
    {
        action.ActionToAsm(file, counter++);
    }
}

std::string ActionTable::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent + 2, ' ') << "- ActionTable:" << std::endl;
    for (const auto& action : actions)
    {
        ss << action.ToYaml(indent + 4);
    }
    return ss.str();
}

std::string ActionTable::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "ActionTable:" << std::endl;
    for (const auto& action : actions)
    {
        ss << action.Print(indent + 2);
    }
    return ss.str();
}

bool ActionTable::IsEndOfFunction() const
{
    return true;
}

Action::Action(const AsmFile::ScriptAction& scriptaction)
{
    std::visit([&](const auto& arg)
        {
            action = arg;
        }, *scriptaction);
    offset = static_cast<std::size_t>(std::visit([](const auto& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, AsmFile::ScriptId>)
            {
                return arg.offset;
            }
            else if constexpr (std::is_same_v<T, AsmFile::ScriptJump>)
            {
                return arg.offset;
            }
        }, *scriptaction));
}

Action::Action(AsmFile& file)
{
    AsmFile::ScriptAction s_action;
    file >> s_action;
    std::visit([&](const auto& arg)
        {
            action = arg;
        }, *s_action);
}

Action::Action(const YAML::Node::const_iterator& it)
{
    std::string type = it->begin()->first.as<std::string>();
    if (type == "ScriptID")
    {
        action = AsmFile::ScriptId(it->begin()->second.as<unsigned int>());
    }
    else if (type == "Jump")
    {
        action = AsmFile::ScriptJump(it->begin()->second.as<std::string>());
    }
    else if (type == "Function")
    {
        action = ScriptFunction(it);
    }
}

Action::operator AsmFile::ScriptAction() const
{
    if (!action)
    {
        return std::nullopt;
    }
    return std::visit([](const auto& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ScriptFunction>)
            {
                return AsmFile::ScriptAction();
            }
            else if constexpr (std::is_same_v<T, AsmFile::ScriptId>)
            {
                return AsmFile::ScriptAction(arg);
            }
            else if constexpr (std::is_same_v<T, AsmFile::ScriptJump>)
            {
                return AsmFile::ScriptAction(arg);
            }
        }, *action);
}

void Action::ToAsm(AsmFile& file) const
{
    file << AsmFile::Instruction("trap", AsmFile::Width::NONE, { "#$01" });
    ActionToAsm(file, 0);
}

void Action::ActionToAsm(AsmFile& file, int p_offset) const
{
    if (!action)
    {
        throw std::runtime_error("Invalid Action");
    }
    return std::visit([&](const auto& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::pair<std::string, ScriptFunction>>)
            {
                file << AsmFile::ScriptAction();
            }
            else if constexpr (std::is_same_v<T, AsmFile::ScriptId>)
            {
                file << AsmFile::ScriptAction(AsmFile::ScriptId(arg.script_id, p_offset));
            }
            else if constexpr (std::is_same_v<T, AsmFile::ScriptJump>)
            {
                file << AsmFile::ScriptAction(AsmFile::ScriptJump(arg.func, p_offset));
            }
        }, *action);
}

std::string Action::ToYaml(int indent) const
{
    std::ostringstream ss;
    if (!action)
    {
        ss << std::endl;
    }
    std::visit([&](const auto& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ScriptFunction>)
            {
                ss << arg.ToYaml(indent);
            }
            else if constexpr (std::is_same_v<T, AsmFile::ScriptId>)
            {
                ss << std::string(indent, ' ') << "- ScriptID: " << arg.script_id << std::endl;
            }
            else if constexpr (std::is_same_v<T, AsmFile::ScriptJump>)
            {
                ss << std::string(indent, ' ') << "- Jump: " << arg.func << std::endl;
            }
        }, *action);
    return ss.str();
}

std::string Action::Print(int indent) const
{
    std::ostringstream ss;
    if (!action)
    {
        ss << std::string(indent, ' ') << "Unknown Action" << std::endl;
    }
    else
    {
        std::visit([&](const auto& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, AsmFile::ScriptId>)
                {
                    ss << std::string(indent, ' ') << StrPrintf("Load Script ID %d", arg.script_id) << std::endl;
                }
                else if constexpr (std::is_same_v<T, AsmFile::ScriptJump>)
                {
                    ss << std::string(indent, ' ') << "Jump to function " << arg.func << std::endl;
                }
                else if constexpr (std::is_same_v<T, ScriptFunction>)
                {
                    ss << arg.Print(indent);
                }
            }, *action);
    }
    return ss.str();
}

bool Action::IsEndOfFunction() const
{
    return (action && !std::holds_alternative<AsmFile::ScriptId>(*action));
}

ScriptFunction::ScriptFunction(const std::string& p_name, const std::vector<ScriptStatement>& p_statements)
    : name(p_name), statements(p_statements)
{
}

ScriptFunction::ScriptFunction(AsmFile& file)
{
    while (file.IsGood())
    {
        AsmFile::Instruction ins;
        AsmFile::ScriptAction action;
        if (file.IsLabel())
        {
            if (!name.empty())
            {
                break;
            }
            name = file.ReadLabel();
        }
        if (file.Read(ins))
        {
            if (name.empty())
            {
                throw std::runtime_error("Instruction outside function:\n" + Trim(ins.ToLine()));
            }
            if (!ProcessScriptFunction(ins, file) && !ProcessScriptTrap(ins, file) && !ProcessScriptMiscInstructions(ins))
            {
                throw std::runtime_error("Unable to process instruction:\n" + Trim(ins.ToLine()));
            }
        }
        else if (std::holds_alternative<AsmFile::ScriptId>(file.Peek()) || std::holds_alternative<AsmFile::ScriptJump>(file.Peek()))
        {
            statements.push_back(ActionTable(file));
        }
        if (std::visit([](const auto& arg) { return arg.IsEndOfFunction(); }, statements.back()))
        {
            break;
        }
    }
}

ScriptFunction::ScriptFunction(const YAML::Node::const_iterator& it)
{
    name = (*it)["Function"].as<std::string>();
    for (auto statement_it = (*it)["Statements"].begin(); statement_it != (*it)["Statements"].end(); ++statement_it)
    {
        std::string statement_type;

        if (statement_it->IsScalar())
        {
            statement_type = statement_it->as<std::string>();
        }
        else if (statement_it->IsMap())
        {
            statement_type = statement_it->begin()->first.as<std::string>();
        }

        if (statement_type == "PlaySound")
        {
            statements.push_back(PlaySound(statement_it));
        }
        else if (statement_type == "Action")
        {
            statements.push_back(Action(statement_it));
        }
        else if (statement_type == "CustomItemAction")
        {
            statements.push_back(CustomItemScript(statement_it));
        }
        else if (statement_type == "Sleep")
        {
            statements.push_back(Sleep(statement_it));
        }
        else if (statement_type == "DisplayPriceMessage")
        {
            statements.push_back(DisplayPrice(statement_it));
        }
        else if (statement_type == "Branch")
        {
            statements.push_back(Branch(statement_it));
        }
        else if (statement_type == "ActionTable")
        {
            statements.push_back(ActionTable(statement_it));
        }
        else if (statement_type == "Question")
        {
            statements.push_back(YesNoPrompt(statement_it));
        }
        else if (statement_type == "ProgressDependent")
        {
            statements.push_back(ProgressList(statement_it));
        }
        else if (statement_type == "SetFlag")
        {
            statements.push_back(SetFlagOnTalk(statement_it));
        }
        else if (statement_type == "CheckFlag")
        {
            statements.push_back(IsFlagSet(statement_it));
        }
        else if (statement_type == "Shop")
        {
            statements.push_back(ShopInteraction(statement_it));
        }
        else if (statement_type == "Church")
        {
            statements.push_back(ChurchInteraction(statement_it));
        }
        else if (statement_type == "Return")
        {
            statements.push_back(Rts());
        }
        else if (statement_type == "Asm")
        {
            statements.push_back(CustomAsm(statement_it));
        }
        else if (statement_type == "QuestProgress")
        {
            statements.push_back(ProgressFlagMapping(statement_it));
        }
        else
        {
            throw std::runtime_error(std::string("Unexpected label ") + statement_type);
        }
    }
}

void ScriptFunction::ToAsm(AsmFile& file) const
{
    file << AsmFile::Label(name);
    for (const auto& statement : statements)
    {
        std::visit([&file](const auto& arg)
            {
                arg.ToAsm(file);
            }, statement);
    }
}

std::string ScriptFunction::ToYaml(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << "Function: " << name << std::endl;
    for (const auto& statement : statements)
    {
        std::visit([&](const auto& arg)
            {
                ss << arg.ToYaml(indent + 2);
            }, statement);
    }
    return ss.str();
}

std::string ScriptFunction::Print(int indent) const
{
    std::ostringstream ss;
    ss << std::string(indent, ' ') << name << std::endl;
    for (const auto& statement : statements)
    {
        std::visit([&](const auto& arg)
            {
                ss << arg.Print(indent + 2);
            }, statement);
    }
    return ss.str();
}

bool ScriptFunction::ProcessScriptFunction(const AsmFile::Instruction& ins, AsmFile& file)
{
    if (!(ins.mnemonic == "bsr" && ins.operands.size() == 1 && std::holds_alternative<std::string>(ins.operands.at(0))))
    {
        return false;
    }
    std::string funcname = std::get<std::string>(ins.operands.at(0));

    if (funcname == "HandleYesNoPrompt")
    {
        statements.push_back(YesNoPrompt(file));
        return true;
    }
    else if (funcname == "HandleProgressDependentDialogue")
    {
        statements.push_back(ProgressList(file));
        return true;
    }
    else if (funcname == "SetFlagBitOnTalking")
    {
        statements.push_back(SetFlagOnTalk(file));
        return true;
    }
    else if (funcname == "CheckFlagAndDisplayMessage")
    {
        statements.push_back(IsFlagSet(file));
        return true;
    }
    else if (funcname == "Sleep_0")
    {
        statements.push_back(Sleep(file));
        return true;
    }
    else if (funcname == "DisplayItemPriceMessage")
    {
        statements.push_back(DisplayPrice(file));
        return true;
    }
    else if (funcname == "HandleShopInterraction")
    {
        statements.push_back(ShopInteraction(file));
        return true;
    }
    else if (funcname == "HandleChurchInterraction")
    {
        statements.push_back(ChurchInteraction(file));
        return true;
    }
    else if (funcname == "PickValueBasedOnFlags")
    {
        statements.push_back(ProgressFlagMapping(file));
        return true;
    }
    return false;
}

bool ScriptFunction::ProcessScriptTrap(const AsmFile::Instruction& ins, AsmFile& file)
{
    if (!(ins.mnemonic == "trap" && ins.operands.size() == 1 && std::holds_alternative<std::string>(ins.operands.front())))
    {
        return false;
    }

    std::string trap = Trim(std::get<std::string>(ins.operands.front()));
    int trap_number = -1;
    if (trap.size() > 0 && trap[0] == '#')
    {
        trap_number = static_cast<int>(AsmFile::ParseValue(trap.substr(1), file.GetDefines()));
    }
    switch (trap_number)
    {
    case 0:
        statements.push_back(PlaySound(file));
        return true;
    case 1:
        statements.push_back(Action(file));
        return true;
    case 2:
        statements.push_back(CustomItemScript(file));
        return true;
    default:
        return false;
    }
}

bool ScriptFunction::ProcessScriptMiscInstructions(const AsmFile::Instruction& ins)
{
    if (ins.mnemonic == "bra" && ins.operands.size() == 1 && std::holds_alternative<std::string>(ins.operands[0]))
    {
        statements.push_back(Branch(ins));
        return true;
    }
    else if (ins.mnemonic == "rts")
    {
        statements.push_back(Rts());
        return true;
    }
    else
    {
        if (!statements.empty() && std::holds_alternative<CustomAsm>(statements.back()))
        {
            std::get<CustomAsm>(statements.back()).Append(ins);
        }
        else
        {
            statements.push_back(CustomAsm(ins));
        }
        return true;
    }
}

ScriptFunctionTable::ScriptFunctionTable(AsmFile& file)
{
    ReadAsm(file);
}

ScriptFunctionTable::ScriptFunctionTable(const std::string& yaml)
{
    YAML::Node node = YAML::Load(yaml);
    const auto& funclist = node.begin()->second;
    for (auto it = funclist.begin(); it != funclist.end(); ++it)
    {
        ScriptFunction func(it);
        if (function_mapping.find(func.name) != function_mapping.cend())
        {
            throw std::runtime_error("Duplicate Label: " + func.name);
        }
        function_mapping.insert({ func.name, func });
        funcnames.push_back(func.name);
    }
    Consolidate();
}

bool ScriptFunctionTable::ReadAsm(AsmFile& file)
{
    while (file.IsGood())
    {
        ScriptFunction func(file);
        if (function_mapping.find(func.name) != function_mapping.cend())
        {
            throw std::runtime_error("Duplicate Label: " + func.name);
        }
        function_mapping.insert({ func.name, func });
        funcnames.push_back(func.name);
    }
    Consolidate();
    return true;
}

bool ScriptFunctionTable::WriteAsm(AsmFile& file)
{
    Unconsolidate();
    for (const auto& funcname : funcnames)
    {
        function_mapping.at(funcname).ToAsm(file);
    }
    Consolidate();
    return true;
}

std::string ScriptFunctionTable::Print() const
{
    std::ostringstream ss;
    ss << "Function Mapping:" << std::endl;
    for (const auto& funcname : funcnames)
    {
        function_mapping.at(funcname).Print(2);
    }
    return ss.str();
}

std::string ScriptFunctionTable::ToYaml(const std::string& name) const
{
    std::ostringstream ss;
    ss << name << ":" << std::endl;
    for (const auto& funcname : funcnames)
    {
        function_mapping.at(funcname).ToYaml(2);
    }
    return ss.str();
}

void ScriptFunctionTable::Consolidate()
{
    std::map<std::string, int> func_call_counts;
    std::set<std::string> non_relocatable_funcs;
    auto Increment = [&func_call_counts](const Action& action)
    {
        if (action.action.has_value() && std::holds_alternative<AsmFile::ScriptJump>(*action.action))
        {
            func_call_counts[std::get<AsmFile::ScriptJump>(*action.action).func]++;
        }
    };
    auto MarkNonRelocatable = [&](const std::string& funcname)
    {
        non_relocatable_funcs.insert(funcname);
        // We also can't relocate the function that is fallen into
        auto next = std::find(funcnames.cbegin(), funcnames.cend(), funcname);
        next = next != funcnames.cend() ? std::next(next) : next;
        if (next != funcnames.cend())
        {
            non_relocatable_funcs.insert(*next);
        }
    };
    for (const auto& func : function_mapping)
    {
        for (const auto& statement : func.second.statements)
        {
            std::visit([&](const auto& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, Action>)
                    {
                        Increment(arg);
                    }
                    else if constexpr (std::is_same_v<T, ActionTable>)
                    {
                        for (const auto& action : arg.actions)
                        {
                            Increment(action);
                        }
                    }
                    else if constexpr (std::is_same_v<T, ProgressList>)
                    {
                        for (const auto& flagaction : arg.progress)
                        {
                            Increment(flagaction.second);
                        }
                    }
                    else if constexpr (std::is_same_v<T, YesNoPrompt>)
                    {
                        Increment(arg.prompt);
                        Increment(arg.on_yes);
                        Increment(arg.on_no);
                    }
                    else if constexpr (std::is_same_v<T, SetFlagOnTalk>)
                    {
                        Increment(arg.on_set);
                        Increment(arg.on_clear);
                    }
                    else if constexpr (std::is_same_v<T, IsFlagSet>)
                    {
                        Increment(arg.on_set);
                        Increment(arg.on_clear);
                    }
                    else if constexpr (std::is_same_v<T, DisplayPrice>)
                    {
                        Increment(arg.display_price);
                    }
                    else if constexpr (std::is_same_v<T, ShopInteraction>)
                    {
                        Increment(arg.on_sale_prompt);
                        Increment(arg.on_sale_confirm);
                        Increment(arg.on_no_money);
                        Increment(arg.on_sale_decline);
                    }
                    else if constexpr (std::is_same_v<T, ChurchInteraction>)
                    {
                        Increment(arg.script_normal_priest);
                        Increment(arg.script_skeleton_priest);
                    }
                }, statement);
        }
    }
    for (const auto& func : function_mapping)
    {
        if (func.second.statements.size() == 0)
        {
            continue;
        }
        const auto& last_statement = func.second.statements.back();
        std::visit([&](const auto& arg)
            {
                if (!arg.IsEndOfFunction())
                {
                    MarkNonRelocatable(func.first);
                }
            }, last_statement);
    }
    std::vector<std::pair<std::string, Action&>> action_list;
    auto Replace = [&](Action& action)
    {
        if (action.action.has_value() && std::holds_alternative<AsmFile::ScriptJump>(*action.action))
        {
            std::string func_name = std::get<AsmFile::ScriptJump>(*action.action).func;
            if (func_call_counts.find(func_name) != func_call_counts.cend() && func_call_counts.at(func_name) == 1
                && function_mapping.find(func_name) != function_mapping.cend()
                && non_relocatable_funcs.find(func_name) == non_relocatable_funcs.cend())
            {
                action.action = function_mapping.at(func_name);
                action_list.push_back({ func_name, action });
            }
        }
    };
    do
    {
        action_list.clear();
        for (auto& func : function_mapping)
        {
            for (auto& statement : func.second.statements)
            {
                std::visit([&](auto& arg)
                    {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, Action>)
                        {
                            Replace(arg);
                        }
                        else if constexpr (std::is_same_v<T, ActionTable>)
                        {
                            for (auto& action : arg.actions)
                            {
                                Replace(action);
                            }
                        }
                        else if constexpr (std::is_same_v<T, ProgressList>)
                        {
                            for (auto& flagaction : arg.progress)
                            {
                                Replace(flagaction.second);
                            }
                        }
                        else if constexpr (std::is_same_v<T, YesNoPrompt>)
                        {
                            Replace(arg.prompt);
                            Replace(arg.on_yes);
                            Replace(arg.on_no);
                        }
                        else if constexpr (std::is_same_v<T, SetFlagOnTalk>)
                        {
                            Replace(arg.on_set);
                            Replace(arg.on_clear);
                        }
                        else if constexpr (std::is_same_v<T, IsFlagSet>)
                        {
                            Replace(arg.on_set);
                            Replace(arg.on_clear);
                        }
                        else if constexpr (std::is_same_v<T, DisplayPrice>)
                        {
                            Replace(arg.display_price);
                        }
                        else if constexpr (std::is_same_v<T, ShopInteraction>)
                        {
                            Replace(arg.on_sale_prompt);
                            Replace(arg.on_sale_confirm);
                            Replace(arg.on_no_money);
                            Replace(arg.on_sale_decline);
                        }
                        else if constexpr (std::is_same_v<T, ChurchInteraction>)
                        {
                            Replace(arg.script_normal_priest);
                            Replace(arg.script_skeleton_priest);
                        }
                    }, statement);
            }
        }
        for (const auto& action : action_list)
        {
            action.second.action = ScriptFunction(action.first, function_mapping.at(action.first).statements);
        }
        for (const auto& action : action_list)
        {
            function_mapping.erase(action.first);
            funcnames.erase(std::find(funcnames.cbegin(), funcnames.cend(), action.first));
        }
    } while (!action_list.empty());
}

void ScriptFunctionTable::Unconsolidate()
{
    std::map<std::string, ScriptFunction> insert_list;
    std::vector<std::string> new_funcnames;
    auto Insert = [&](Action& action)
    {
        if (action.action.has_value() && std::holds_alternative<ScriptFunction>(*action.action))
        {
            ScriptFunction consolidated_func = std::get<ScriptFunction>(*action.action);
            std::string func_name = consolidated_func.name;
            action.action = AsmFile::ScriptJump(func_name, action.offset);
            if (std::find(new_funcnames.cbegin(), new_funcnames.cend(), func_name) == new_funcnames.cend() &&
                std::find(funcnames.cbegin(), funcnames.cend(), func_name) == funcnames.cend())
            {
                new_funcnames.push_back(func_name);
                insert_list.insert({ func_name, consolidated_func });
            }
        }
    };
    while (true)
    {
        for (auto& funcname : funcnames)
        {
            new_funcnames.push_back(funcname);
            for (auto& statement : function_mapping.at(funcname).statements)
            {
                std::visit([&](auto& arg)
                    {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, Action>)
                        {
                            Insert(arg);
                        }
                        else if constexpr (std::is_same_v<T, ActionTable>)
                        {
                            for (auto& action : arg.actions)
                            {
                                Insert(action);
                            }
                        }
                        else if constexpr (std::is_same_v<T, ProgressList>)
                        {
                            for (auto& flagaction : arg.progress)
                            {
                                Insert(flagaction.second);
                            }
                        }
                        else if constexpr (std::is_same_v<T, YesNoPrompt>)
                        {
                            Insert(arg.prompt);
                            Insert(arg.on_yes);
                            Insert(arg.on_no);
                        }
                        else if constexpr (std::is_same_v<T, SetFlagOnTalk>)
                        {
                            Insert(arg.on_set);
                            Insert(arg.on_clear);
                        }
                        else if constexpr (std::is_same_v<T, IsFlagSet>)
                        {
                            Insert(arg.on_set);
                            Insert(arg.on_clear);
                        }
                        else if constexpr (std::is_same_v<T, DisplayPrice>)
                        {
                            Insert(arg.display_price);
                        }
                        else if constexpr (std::is_same_v<T, ShopInteraction>)
                        {
                            Insert(arg.on_sale_prompt);
                            Insert(arg.on_sale_confirm);
                            Insert(arg.on_no_money);
                            Insert(arg.on_sale_decline);
                        }
                        else if constexpr (std::is_same_v<T, ChurchInteraction>)
                        {
                            Insert(arg.script_normal_priest);
                            Insert(arg.script_skeleton_priest);
                        }
                    }, statement);
            }
        }
        if (insert_list.empty())
        {
            break;
        }
        for (const auto& item : insert_list)
        {
            function_mapping.emplace(item);
        }
        insert_list.clear();
        funcnames = new_funcnames;
        new_funcnames.clear();
    }
}

std::ostream& operator<<(std::ostream& lhs, const Statement& rhs)
{
    lhs << rhs.Print();
    return lhs;
}

std::ostream& operator<<(std::ostream& lhs, const ScriptFunction& rhs)
{
    lhs << rhs.Print();
    return lhs;
}

std::ostream& operator<<(std::ostream& lhs, const ScriptFunctionTable& rhs)
{
    lhs << rhs.Print();
    return lhs;
}

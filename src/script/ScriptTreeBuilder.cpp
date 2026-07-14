#include <script/ScriptTreeNode.h>
#include <script/ScriptTreeDataViewModel.h>

#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>

#include <algorithm>
#include <sstream>

using namespace Landstalker;

void AddFuncBody(ScriptTreeNode& node, const ScriptFunction& func, std::shared_ptr<GameData> gd)
{
    for (const auto& s : *func.statements)
    {
        std::visit([&node, &gd](const auto& e)
            {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, Statements::Action>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::ACTION, "Script Action:" });
                    AddAction(node.children.back(), e, gd);
                }
                else if constexpr (std::is_same_v<T, Statements::YesNoPrompt>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::QUESTION, "Yes/No Question:" });
                    auto& gchild = node.children.back();
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::Q_PROMPT, "Prompt:" });
                    AddAction(gchild.children.back(), e.prompt, gd, "Prompt:");
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::Q_ON_YES, "OnYes:" });
                    AddAction(gchild.children.back(), e.on_yes, gd, "OnYes:");
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::Q_ON_NO, "OnNo:" });
                    AddAction(gchild.children.back(), e.on_no, gd, "OnNo:");
                }
                else if constexpr (std::is_same_v<T, Statements::SetFlagOnTalk>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SET_FLAG, "Set Flag " + Hex(e.flag) + " on Talk:" });
                    auto& gchild = node.children.back();
                    gchild.numeric_value = e.flag;
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SF_ON_CLEAR, "OnClear:" });
                    AddAction(gchild.children.back(), e.on_clear, gd, "OnClear:");
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SF_ON_SET, "OnSet:" });
                    AddAction(gchild.children.back(), e.on_set, gd, "OnSet:");
                }
                else if constexpr (std::is_same_v<T, Statements::IsFlagSet>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::CHECK_FLAG, "Check Flag " + Hex(e.flag) + ":" });
                    auto& gchild = node.children.back();
                    gchild.numeric_value = e.flag;
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::CF_ON_SET, "OnSet:" });
                    AddAction(gchild.children.back(), e.on_set, gd, "OnSet:");
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::CF_ON_CLEAR, "OnClear:" });
                    AddAction(gchild.children.back(), e.on_clear, gd, "OnClear:");
                }
                else if constexpr (std::is_same_v<T, Statements::ProgressList>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::PROG_DEP_TABLE, "Progress Dependent:" });
                    auto& gchild = node.children.back();
                    for (const auto& a : e.progress)
                    {
                        gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::PROG_DEP_ACTION, StrPrintf("On Quest %d, Progress %d:", a.first.quest, a.first.progress) });
                        auto& ggchild = gchild.children.back();
                        ggchild.numeric_value = a.first.quest;
                        ggchild.numeric_value2 = a.first.progress;
                        ggchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::ACTION, "Script Action:" });
                        AddAction(ggchild.children.back(), a.second, gd);
                    }
                }
                else if constexpr (std::is_same_v<T, Statements::ShopInteraction>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SHOP, "Shop:" });
                    auto& gchild = node.children.back();
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SHOP_ON_SALE_PROMPT, "OnSalePrompt:" });
                    AddAction(gchild.children.back(), e.on_sale_prompt, gd, "OnSalePrompt:");
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SHOP_ON_SALE_CONFIRM, "OnSaleConfirm:" });
                    AddAction(gchild.children.back(), e.on_sale_confirm, gd, "OnSaleConfirm:");
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SHOP_ON_SALE_NO_MONEY, "OnNoMoney:" });
                    AddAction(gchild.children.back(), e.on_no_money, gd, "OnNoMoney:");
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SHOP_ON_SALE_CANCEL, "OnSaleDecline:" });
                    AddAction(gchild.children.back(), e.on_sale_decline, gd, "OnSaleDecline:");
                }
                else if constexpr (std::is_same_v<T, Statements::ChurchInteraction>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::CHURCH, "Church:" });
                    auto& gchild = node.children.back();
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::CHURCH_NORMAL_PRIEST, "NormalPriest:" });
                    AddAction(gchild.children.back(), e.script_normal_priest, gd, "NormalPriest:");
                    gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::CHURCH_SKELETON_PRIEST, "SkeletonPriest:" });
                    AddAction(gchild.children.back(), e.script_skeleton_priest, gd, "SkeletonPriest:");
                }
                else if constexpr (std::is_same_v<T, Statements::DisplayPrice>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::DISPLAY_PRICE, "Display Item Price:" });
                    AddAction(node.children.back(), e.display_price, gd, "Display Item Price:");
                }
                else if constexpr (std::is_same_v<T, Statements::ActionTable>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::TABLE, "Action Table:" });
                    auto& gchild = node.children.back();
                    for (std::size_t i = 0; i < e.actions.size(); ++i)
                    {
                        const wxString label = StrPrintf("Entry %d:", i);
                        gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::TABLE_ENTRY, label });
                        AddAction(gchild.children.back(), e.actions.at(i), gd, label);
                    }
                }
                else if constexpr (std::is_same_v<T, Statements::Branch>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::BRANCH, "Branch: " + e.label + (e.wide ? " W" : "") });
                    auto& gchild = node.children.back();
                    gchild.text_value = e.label;
                    gchild.bool_value = e.wide;
                }
                else if constexpr (std::is_same_v<T, Statements::PlaySound>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::PLAY_SOUND, StrWPrintf("Play Sound: %d (%ls)", e.sound, Labels::Get(Labels::C_SOUNDS, e.sound).value_or(L"").c_str()) });
                    node.children.back().numeric_value = e.sound;
                }
                else if constexpr (std::is_same_v<T, Statements::CustomItemScript>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::CUSTOM_SHOP_ACTION, StrWPrintf("Custom Item Script: %d", e.custom_item_script) });
                    node.children.back().numeric_value = e.custom_item_script;
                }
                else if constexpr (std::is_same_v<T, Statements::Sleep>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SLEEP, StrWPrintf("Sleep: %d frames", e.ticks) });
                    node.children.back().numeric_value = e.ticks;
                }
                else if constexpr (std::is_same_v<T, Statements::Rts>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::RETURN, "Return" });
                }
                else if constexpr (std::is_same_v<T, Statements::CustomAsm>)
                {
                    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::CUSTOM_ASM, "Custom ASM"});
                    auto& gchild = node.children.back();
                    for (const auto& line : e.instructions)
                    {
                        gchild.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::CUSTOM_ASM_LINE, Trim(line.ToLine()) });
                    }
                }
            }, s);
    }
}

void AddFunc(ScriptTreeNode& node, const ScriptFunction& func, std::shared_ptr<GameData> gd)
{
    node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::FUNCTION, "Function: " + func.name});
    // The authoritative copy of the name - everything downstream (sync, rename, reference
    // analysis) reads this rather than parsing it back out of the display text.
    node.children.back().text_value = func.name;
    AddFuncBody(node.children.back(), func, gd);
}
void AddAction(ScriptTreeNode& node, const Statements::Action& action, std::shared_ptr<GameData> gd, const wxString& label)
{
    node.action_label = label;
    node.text_value.clear();
    node.numeric_value = 0;
    if (std::holds_alternative<std::monostate>(action.action))
    {
        node.name = label + " Unknown";
    }
    else
    {
        std::visit([&node, &gd, &label](const auto& e)
            {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, AsmFile::ScriptId>)
                {
                    node.name = label + StrPrintf(" Script ID %04X", e.script_id);
                    // The raw ID, so sync/validity/editors read the value instead of the label.
                    node.numeric_value = static_cast<uint16_t>(e.script_id);
                    // Bounds-checked rather than relying on GetScriptLine() throwing once we run
                    // past the table: a chain that never hits an "end" marker (e.g. a freshly
                    // added entry defaulting to script ID 0) must not throw here, since this is
                    // called directly from the initial tree build with no surrounding try/catch
                    // (unlike ScriptTreeDataViewModel::PopulateScriptAction's edit-time equivalent).
                    const std::size_t line_count = gd->GetScriptData()->GetScript()->GetScriptLineCount();
                    uint16_t line = e.script_id;
                    while (line < line_count)
                    {
                        const ScriptTableEntry& entry = gd->GetScriptData()->GetScript()->GetScriptLine(line);
                        node.children.push_back(ScriptTreeNode{ ScriptTreeNodeType::SCRIPT_ENTRY, StrWPrintf("%04X - %ls", line,
                            entry.ToString(gd).c_str())});
                        node.children.back().numeric_value = line;
                        if (entry.end)
                        {
                            break;
                        }
                        ++line;
                    }
                }
                else if constexpr (std::is_same_v<T, AsmFile::ScriptJump>)
                {
                    node.name = label + " Function: " + e.func;
                    // ScriptTreeDataViewModel::BuildAction() reads this to sync the row as a jump-by-name
                    // rather than falling through to the raw script-ID case.
                    node.text_value = e.func;
                }
                else if constexpr (std::is_same_v<T, std::shared_ptr<ScriptFunction>>)
                {
                    node.name = e ? (label + " Function: " + e->name) : (label + " Function:");
                    if (e)
                    {
                        node.text_value = e->name;
                        AddFuncBody(node, *e, gd);
                    }
                }
            }, action.action);
    }
}


std::optional<std::string> GetJumpTarget(const ScriptTreeNode& node)
{
    if (node.type == ScriptTreeNodeType::BRANCH)
    {
        // The label lives in text_value (bool_value carries the wide flag separately).
        if (node.text_value.empty())
        {
            return std::nullopt;
        }
        return node.text_value;
    }

    if (node.type == ScriptTreeNodeType::FUNCTION)
    {
        return std::nullopt;
    }

    // A standalone "Script Action:" node, a merged top-level slot (e.g. "On Pay: Function: X"),
    // or any other action merged the same way at a deeper nesting level (e.g. "Display Item
    // Price: Function: X", "OnSet: Function: X") all carry the referenced/embedded function's
    // name in text_value - AddAction() sets it for every kind of function reference, but only
    // AddMappingActionSlot()/PopulateScriptAction() additionally set embedded_function_name (on
    // the outer slot node), so checking text_value here (rather than embedded_function_name, or
    // parsing "Function: <name>" back out of the display text) is what actually catches nested
    // merges too. It's only a *reference* worth surfacing as a "related function" candidate when
    // the body isn't already embedded right here (no children) - otherwise it's already fully
    // shown in place, and treating it as a reference too would make CollectFunctionReferences()
    // see it as an additional, separate "related" candidate and re-add it as a redundant sibling.
    if (!node.text_value.empty() && node.children.empty())
    {
        return node.text_value;
    }

    return std::nullopt;
}

void CollectFunctionReferences(const ScriptTreeNode& node, const std::set<std::string>& function_names, std::vector<std::string>& targets)
{
    if (auto target = GetJumpTarget(node))
    {
        targets.push_back(*target);
    }
    else if (node.type == ScriptTreeNodeType::CUSTOM_ASM_LINE)
    {
        const std::string line = node.name.ToStdString();
        for (const auto& name : function_names)
        {
            if (MentionsLabel(line, name))
            {
                targets.push_back(name);
            }
        }
    }

    for (const auto& child : node.children)
    {
        CollectFunctionReferences(child, function_names, targets);
    }
}

std::optional<std::string> GetFunctionNodeName(const ScriptTreeNode& node)
{
    if (node.type == ScriptTreeNodeType::FUNCTION)
    {
        // The name is kept in text_value by AddFunc()/PopulateFixedChildren()/renames.
        if (node.text_value.empty())
        {
            return std::nullopt;
        }
        return node.text_value;
    }

    // A merged top-level slot (e.g. "On Pay: Function: X") that embeds X's full body directly -
    // recognised the same way a standalone ScriptTreeNodeType::FUNCTION node would be.
    if (!node.embedded_function_name.empty())
    {
        return node.embedded_function_name;
    }

    // Any other node whose resolved action is embedded directly at a deeper nesting level (e.g.
    // "Display Item Price: Function: X", "OnSet: Function: X") - has both text_value and a body -
    // is likewise already displaying that function's full definition here, not merely referencing
    // it. Without this, CollectDisplayFunctionNames() would fail to count X as "already shown"
    // for such nodes, and the related-function search could re-add X as a redundant sibling.
    if (!node.text_value.empty() && !node.children.empty())
    {
        return node.text_value;
    }

    return std::nullopt;
}

void CollectDisplayFunctionNames(const ScriptTreeNode& node, std::set<std::string>& names)
{
    if (auto name = GetFunctionNodeName(node))
    {
        names.insert(*name);
    }
    for (const auto& child : node.children)
    {
        CollectDisplayFunctionNames(child, names);
    }
}

std::map<std::string, std::set<std::string>> BuildFunctionReferenceSources(std::shared_ptr<ScriptFunctionTable> data, std::shared_ptr<GameData> gd)
{
    std::map<std::string, std::set<std::string>> sources;
    for (const auto& name : data->GetFunctionNames())
    {
        const ScriptFunction* func = data->GetMapping(name);
        if (!func)
        {
            continue;
        }

        ScriptTreeNode temp{ ScriptTreeNodeType::ROOT, "Temp" };
        AddFunc(temp, *func, gd);
        std::vector<std::string> targets;
        CollectFunctionReferences(temp, std::set<std::string>(data->GetFunctionNames().cbegin(), data->GetFunctionNames().cend()), targets);
        for (const auto& target : targets)
        {
            sources[target].insert(name);
        }
    }
    return sources;
}

bool FunctionFallsThrough(const ScriptFunction& func)
{
    if (!func.statements || func.statements->empty())
    {
        return false;
    }

    return !std::visit([](const auto& statement) { return statement.IsEndOfFunction(); }, func.statements->back());
}

std::map<std::string, std::string> BuildFallthroughTargets(std::shared_ptr<ScriptFunctionTable> data)
{
    std::map<std::string, std::string> targets;
    const auto& names = data->GetFunctionNames();
    for (std::size_t i = 0; i + 1 < names.size(); ++i)
    {
        const ScriptFunction* func = data->GetMapping(names.at(i));
        if (func && FunctionFallsThrough(*func))
        {
            targets[names.at(i)] = names.at(i + 1);
        }
    }
    return targets;
}
void AddFallthroughReferenceSources(std::map<std::string, std::set<std::string>>& ref_sources, const std::map<std::string, std::string>& fallthrough_targets)
{
    for (const auto& fallthrough : fallthrough_targets)
    {
        ref_sources[fallthrough.second].insert(fallthrough.first);
    }
}

struct FunctionRefTally
{
    int action_refs = 0;
    int other_refs = 0;
    // Implicit reference: the previous function in table order falls through into this one.
    // Kept separate from other_refs because it blocks inline embedding but shouldn't count
    // toward the "[Shared xN]" marker (there's no visible reference site for it).
    int fallthrough_refs = 0;
};

void TallyNodeReferences(const ScriptTreeNode& node, const std::set<std::string>& function_names, std::map<std::string, FunctionRefTally>& tally)
{
    if (node.type == ScriptTreeNodeType::CUSTOM_ASM_LINE)
    {
        const std::string line = node.name.ToStdString();
        for (const auto& name : function_names)
        {
            if (MentionsLabel(line, name))
            {
                ++tally[name].other_refs;
            }
        }
    }
    else if (auto target = GetJumpTarget(node))
    {
        if (node.type == ScriptTreeNodeType::BRANCH)
        {
            ++tally[*target].other_refs;
        }
        else
        {
            ++tally[*target].action_refs;
        }
    }

    for (const auto& child : node.children)
    {
        TallyNodeReferences(child, function_names, tally);
    }
}

// A "pool" is the set of function tables that share a single namespace. Every function file
// (character / cutscene / shop / custom-item) can reference any other's functions, so the whole
// program is treated as one pool for reference analysis, while the tables themselves stay
// separate so each still writes back to its own ASM file.
using FunctionPool = std::vector<std::shared_ptr<ScriptFunctionTable>>;

FunctionPool BuildFullPool(std::shared_ptr<GameData> gd)
{
    auto sd = gd->GetScriptData();
    return { sd->GetCharFuncs(), sd->GetCutsceneFuncs(), sd->GetShopFuncs(), sd->GetItemFuncs() };
}

const ScriptFunction* PoolGetMapping(const FunctionPool& pool, const std::string& name)
{
    for (const auto& table : pool)
    {
        if (const ScriptFunction* func = table ? table->GetMapping(name) : nullptr)
        {
            return func;
        }
    }
    return nullptr;
}

// Counts, for every pool function, how many times it is referenced by a plain Script Action jump
// versus by anything else (a Branch, a mention inside a custom ASM block, or being fallen into
// from the previous function). A jump/branch/ASM-mention from any function in the pool to any
// other counts - so a custom-item function jumping to a shop function is tallied too. Fallthrough
// stays per-table (a function can only fall into the next one in the same file).
std::map<std::string, FunctionRefTally> TallyPoolReferences(const FunctionPool& pool, std::shared_ptr<GameData> gd)
{
    std::set<std::string> all_names;
    for (const auto& table : pool)
    {
        for (const auto& name : table->GetFunctionNames())
        {
            all_names.insert(name);
        }
    }

    std::map<std::string, FunctionRefTally> tally;
    for (const auto& table : pool)
    {
        for (const auto& name : table->GetFunctionNames())
        {
            const ScriptFunction* func = table->GetMapping(name);
            if (!func)
            {
                continue;
            }
            ScriptTreeNode temp{ ScriptTreeNodeType::ROOT, "Temp" };
            AddFunc(temp, *func, gd);
            TallyNodeReferences(temp, all_names, tally);
        }
        for (const auto& fallthrough : BuildFallthroughTargets(table))
        {
            ++tally[fallthrough.second].fallthrough_refs;
        }
    }
    return tally;
}

// Visits every slot action across all four script tables (the shop/item tables hold their
// actions in fixed-size arrays, the character/cutscene tables in flat vectors, so this is
// generic over the element container).
template <typename Visitor>
void ForEachSlotAction(std::shared_ptr<GameData> gd, Visitor&& visit)
{
    auto sd = gd->GetScriptData();
    for (const auto& action : *sd->GetCharTable())
    {
        visit(action);
    }
    for (const auto& action : *sd->GetCutsceneTable())
    {
        visit(action);
    }
    for (const auto& shop : *sd->GetShopTable())
    {
        for (const auto& action : shop.actions)
        {
            visit(action);
        }
    }
    for (const auto& item : *sd->GetItemTable())
    {
        for (const auto& action : item.actions)
        {
            visit(action);
        }
    }
}

// The set of pool-function names referenced by any slot in any script table.
std::set<std::string> CollectAllSlotReferences(const FunctionPool& pool, std::shared_ptr<GameData> gd)
{
    std::set<std::string> refs;
    ForEachSlotAction(gd, [&](const ScriptTable::Action& action)
    {
        if (const std::string* name = std::get_if<std::string>(&action))
        {
            if (PoolGetMapping(pool, *name))
            {
                refs.insert(*name);
            }
        }
    });
    return refs;
}

// [Shared xN] counts: every slot reference across all script tables, plus every function-to-
// function reference across the pool (fallthrough excluded - it has no visible reference site).
std::map<std::string, int> BuildPoolRefCounts(const FunctionPool& pool,
    const std::map<std::string, FunctionRefTally>& tally, std::shared_ptr<GameData> gd)
{
    std::map<std::string, int> ref_counts;
    ForEachSlotAction(gd, [&](const ScriptTable::Action& action)
    {
        if (const std::string* name = std::get_if<std::string>(&action))
        {
            if (PoolGetMapping(pool, *name))
            {
                ++ref_counts[*name];
            }
        }
    });
    for (const auto& t : tally)
    {
        if (PoolGetMapping(pool, t.first))
        {
            ref_counts[t.first] += t.second.action_refs + t.second.other_refs;
        }
    }
    return ref_counts;
}

void CollectBareActionRefSites(ScriptTreeNode& node, const std::string& name, std::vector<ScriptTreeNode*>& sites)
{
    // A bare (childless) Script Action node still referencing the function by name only - the
    // kind of node AddAction() creates for an AsmFile::ScriptJump. Branch nodes also carry
    // their target in text_value but are not embed sites.
    const bool is_bare_action_ref = node.type != ScriptTreeNodeType::BRANCH
        && node.type != ScriptTreeNodeType::FUNCTION && node.embedded_function_name.empty()
        && node.text_value == name && node.children.empty();
    if (is_bare_action_ref)
    {
        sites.push_back(&node);
    }
    for (auto& child : node.children)
    {
        CollectBareActionRefSites(child, name, sites);
    }
}

// Embeds each remaining single-use function's body directly beneath the one Script Action node
// that references it (mirroring how ScriptTreeDataViewModel::PopulateScriptAction() displays a just-selected or
// just-created function), instead of leaving a bare reference plus a separate sibling
// "Function:" header node with its own copy of the body. This is purely a display-tree transform:
// the function stays a standalone entry in the underlying table, and the embedded node syncs its
// edits back to that entry (see ScriptTreeDataViewModel::CollectFunctionNodes()).
// Only done when it can't mislead: the function must be referenced exactly once in the whole
// table, by a plain script action (not a branch or ASM label, which need a standalone definition
// to jump to), must not fall through into the next function and must not be fallen into (both of
// which pin it to its position in the table order).
void InlineSingleUseFunctions(ScriptTreeNode& owner, const std::map<std::string, FunctionRefTally>& tally, std::shared_ptr<ScriptFunctionTable> data, std::set<std::string>& funcs, std::shared_ptr<GameData> gd)
{
    bool changed = false;
    do
    {
        changed = false;
        // Iterate a snapshot - embedding erases from funcs, and an embedded body can itself
        // contain the sole reference site for another candidate (handled by the outer loop).
        for (const auto& name : std::vector<std::string>(funcs.cbegin(), funcs.cend()))
        {
            const auto refs = tally.find(name);
            if (refs == tally.cend() || refs->second.action_refs != 1 || refs->second.other_refs != 0
                || refs->second.fallthrough_refs != 0)
            {
                continue;
            }
            const ScriptFunction* func = data->GetMapping(name);
            if (!func || !func->statements || func->statements->empty() || FunctionFallsThrough(*func))
            {
                continue;
            }
            std::vector<ScriptTreeNode*> sites;
            CollectBareActionRefSites(owner, name, sites);
            if (sites.empty())
            {
                continue;
            }
            // The function's single referencing action can be displayed more than once - once per
            // table slot sharing its containing function (see the "[Shared xN]" marker) - so embed
            // the body at every display site, exactly as the sharing slots each already display
            // their own copy of the containing function's body.
            for (ScriptTreeNode* site : sites)
            {
                site->embedded_function_name = name;
                AddFuncBody(*site, *func, gd);
                site->SetParent(site->parent);
            }
            funcs.erase(name);
            changed = true;
        }
    }
    while (changed);
}
void AddSingleUseRelatedFunctions(ScriptTreeNode& owner, const ScriptTreeNode& source, std::shared_ptr<ScriptFunctionTable> data, std::set<std::string>& funcs, const std::map<std::string, std::set<std::string>>& ref_sources, const std::map<std::string, std::string>& fallthrough_targets, std::shared_ptr<GameData> gd)
{
    const std::set<std::string> function_names(data->GetFunctionNames().cbegin(), data->GetFunctionNames().cend());
    std::set<std::string> group_names;
    CollectDisplayFunctionNames(owner, group_names);
    if (auto source_name = GetFunctionNodeName(source))
    {
        group_names.insert(*source_name);
    }

    auto collect_targets_for_function = [&](const std::string& name)
    {
        std::vector<std::string> targets;
        const ScriptFunction* func = data->GetMapping(name);
        if (func)
        {
            ScriptTreeNode temp{ ScriptTreeNodeType::ROOT, "Temp" };
            AddFunc(temp, *func, gd);
            CollectFunctionReferences(temp, function_names, targets);
        }

        const auto fallthrough = fallthrough_targets.find(name);
        if (fallthrough != fallthrough_targets.end())
        {
            targets.push_back(fallthrough->second);
        }
        return targets;
    };

    auto add_related = [&](const std::string& name) -> bool
    {
        auto remaining = funcs.find(name);
        if (remaining == funcs.end())
        {
            return false;
        }

        const ScriptFunction* related = data->GetMapping(name);
        if (!related)
        {
            return false;
        }

        AddFunc(owner, *related, gd);
        funcs.erase(remaining);
        group_names.insert(name);
        return true;
    };

    auto referenced_only_by_group_or_candidates = [&](const std::string& name, const std::set<std::string>& candidates)
    {
        const auto sources = ref_sources.find(name);
        return sources != ref_sources.end()
            && !sources->second.empty()
            && std::all_of(sources->second.cbegin(), sources->second.cend(), [&](const auto& source_name)
            {
                return group_names.find(source_name) != group_names.end() || candidates.find(source_name) != candidates.end();
            });
    };

    bool changed = false;
    do
    {
        changed = false;
        std::set<std::string> candidates;
        bool candidate_changed = false;
        do
        {
            candidate_changed = false;
            std::set<std::string> sources(group_names);
            sources.insert(candidates.cbegin(), candidates.cend());
            for (const auto& source_name : sources)
            {
                for (const auto& target : collect_targets_for_function(source_name))
                {
                    if (funcs.find(target) != funcs.end() && group_names.find(target) == group_names.end() && candidates.insert(target).second)
                    {
                        candidate_changed = true;
                    }
                }
            }
        }
        while (candidate_changed);

        bool pruned = false;
        do
        {
            pruned = false;
            for (auto it = candidates.begin(); it != candidates.end();)
            {
                if (!referenced_only_by_group_or_candidates(*it, candidates))
                {
                    it = candidates.erase(it);
                    pruned = true;
                }
                else
                {
                    ++it;
                }
            }
        }
        while (pruned);

        for (const auto& name : data->GetFunctionNames())
        {
            if (candidates.find(name) != candidates.end())
            {
                changed = add_related(name) || changed;
            }
        }
    }
    while (changed);
}

void AddFunctionAnchor(std::set<std::string>& anchors, std::shared_ptr<ScriptFunctionTable> data, const ScriptTable::Action& action)
{
    if (std::holds_alternative<std::string>(action))
    {
        const std::string& name = std::get<std::string>(action);
        if (data->GetMapping(name))
        {
            anchors.insert(name);
        }
    }
}

std::set<std::string> BuildLocationAnchors(std::shared_ptr<ScriptFunctionTable> data, std::shared_ptr<std::vector<ScriptTable::Action>> table)
{
    std::set<std::string> anchors;
    for (const auto& action : *table)
    {
        AddFunctionAnchor(anchors, data, action);
    }
    return anchors;
}

std::set<std::string> BuildLocationAnchors(std::shared_ptr<ScriptFunctionTable> data, std::shared_ptr<std::vector<ScriptTable::Shop>> table)
{
    std::set<std::string> anchors;
    for (const auto& shop : *table)
    {
        for (const auto& action : shop.actions)
        {
            AddFunctionAnchor(anchors, data, action);
        }
    }
    return anchors;
}

std::set<std::string> BuildLocationAnchors(std::shared_ptr<ScriptFunctionTable> data, std::shared_ptr<std::vector<ScriptTable::Item>> table)
{
    std::set<std::string> anchors;
    for (const auto& item : *table)
    {
        for (const auto& action : item.actions)
        {
            AddFunctionAnchor(anchors, data, action);
        }
    }
    return anchors;
}

void AddLocationRelatedFunctions(ScriptTreeNode& owner, const std::string& owner_func, std::shared_ptr<ScriptFunctionTable> data, std::set<std::string>& funcs, const std::set<std::string>& anchors, std::shared_ptr<GameData> gd)
{
    bool in_owner_run = false;
    for (const auto& name : data->GetFunctionNames())
    {
        if (name == owner_func)
        {
            in_owner_run = true;
            continue;
        }
        if (!in_owner_run)
        {
            continue;
        }
        if (anchors.find(name) != anchors.end())
        {
            break;
        }

        auto remaining = funcs.find(name);
        if (remaining == funcs.end())
        {
            continue;
        }

        if (const ScriptFunction* related = data->GetMapping(name))
        {
            AddFunc(owner, *related, gd);
            funcs.erase(remaining);
        }
    }
}
// Adds a top-level table slot (e.g. "On Talk:", "On Pay:") as a single node combining the
// slot's own label with its resolved action - a raw Script ID, an unresolved function-name
// reference, or (when the name is a function known in this same table) the function merged in
// directly with its body as this node's children. *merged_function is set to true only in that
// last case, so the caller can attach location/single-use related functions after it.
ScriptTreeNode* AddMappingActionSlot(ScriptTreeNode& parent, ScriptTreeNodeType type, const wxString& label, const ScriptTable::Action& entry,
    const FunctionPool& pool, std::set<std::string>& funcs,
    const std::map<std::string, int>& ref_counts, std::set<std::string>& nested_in_object,
    std::function<void(const ScriptTable::Action&)> write_back, std::shared_ptr<GameData> gd, bool& merged_function)
{
    ScriptTreeNode* slot = parent.AddChild(type, label);
    merged_function = false;
    if (std::holds_alternative<uint16_t>(entry))
    {
        AddAction(*slot, AsmFile::ScriptAction(std::get<uint16_t>(entry)), gd, label);
        slot->write_back = write_back;
    }
    else if (std::holds_alternative<std::string>(entry))
    {
        const std::string& name = std::get<std::string>(entry);
        // Resolve against the whole pool: a slot in any table can reference a function defined
        // in any function file. funcs.erase() below only affects this table's orphan candidates
        // (a function owned by another file isn't in funcs anyway).
        const ScriptFunction* func = PoolGetMapping(pool, name);
        if (func)
        {
            funcs.erase(name);
            slot->write_back = write_back;

            // Multiple table rows can legitimately share one function (e.g. several shops
            // reusing a common price display script) - mark it so it's clear that editing this
            // copy affects every other place it's referenced too.
            const auto ref_count_it = ref_counts.find(name);
            const int ref_count = (ref_count_it != ref_counts.end()) ? ref_count_it->second : 1;
            const wxString marker = (ref_count > 1) ? StrPrintf(" [Shared x%d]", ref_count) : wxString();
            slot->action_label = label + marker;

            if (nested_in_object.insert(name).second)
            {
                // First time this function is referenced within this object (shop/character/
                // etc.) - nest its body here.
                slot->name = label + marker + " Function: " + func->name;
                slot->embedded_function_name = func->name;
                slot->text_value = func->name;
                AddFuncBody(*slot, *func, gd);
                merged_function = true;
            }
            else
            {
                // Already nested elsewhere within this same object - reference it instead of
                // repeating the identical body right next to the first copy. The target still
                // goes in text_value so the editor/validity logic can read it without parsing
                // the display text. (This can't turn the slot into an inline-embed site: the
                // function was erased from `funcs` above, so InlineSingleUseFunctions() never
                // considers it.)
                slot->name = label + marker + " Function: " + func->name;
                slot->text_value = func->name;
            }
        }
        else
        {
            AddAction(*slot, AsmFile::ScriptAction(name), gd, label);
            slot->write_back = write_back;
        }
    }
    return slot;
}

void AddChrMapping(ScriptTreeNode* root, std::shared_ptr<ScriptFunctionTable> data, std::shared_ptr<std::vector<ScriptTable::Action>> table, const std::string& desc, std::shared_ptr<GameData> gd)
{
    // All function files share one namespace - resolve, tally, count references and attribute
    // orphans against the whole pool (see BuildFullPool).
    const FunctionPool pool = BuildFullPool(gd);

    auto chr_root = root->AddChild(ScriptTreeNodeType::ROOT, desc);
    std::set<std::string> funcs(data->GetFunctionNames().cbegin(), data->GetFunctionNames().cend());
    auto location_anchors = BuildLocationAnchors(data, table);

    const auto tally = TallyPoolReferences(pool, gd);
    const auto ref_counts = BuildPoolRefCounts(pool, tally, gd);

    // Deferred to a second pass below - see the comment there for why.
    std::vector<std::pair<ScriptTreeNode*, std::string>> pending_related;

    int chr_idx = 0;
    for (const auto& entry : *table)
    {
        auto chr = chr_root->AddChild(ScriptTreeNodeType::ROOT, StrWPrintf("%ls (%03d)", gd->GetStringData()->GetCharacterDisplayName(chr_idx).c_str(), chr_idx));
        std::set<std::string> nested_in_object;
        bool merged_function = false;
        AddMappingActionSlot(*chr, ScriptTreeNodeType::ROOT, "On Talk:", entry, pool, funcs, ref_counts, nested_in_object,
            [table, chr_idx](const ScriptTable::Action& a) { table->at(chr_idx) = a; }, gd, merged_function);
        if (merged_function)
        {
            // Related functions attach to the entry itself, not the slot - bare "Function:"
            // headers always sit at the entry's top level, below the slots.
            pending_related.emplace_back(chr, std::get<std::string>(entry));
        }
        ++chr_idx;
    }

    // Embed single-use functions beneath their referencing Script Action node first, so the
    // related-function search below doesn't instead attach them as separate sibling
    // "Function:" header nodes at the slot level.
    InlineSingleUseFunctions(*chr_root, tally, data, funcs, gd);

    // Pull in related helper functions only after every entry's own, direct action-string match
    // has already been claimed above - otherwise an entry processed later (e.g. after being
    // reordered via the UI) could lose its own function to an earlier entry's related-function
    // search purely because of iteration order, even though the underlying data didn't change.
    for (const auto& [slot, owner_func] : pending_related)
    {
        AddLocationRelatedFunctions(*slot, owner_func, data, funcs, location_anchors, gd);
    }
    // The related functions just attached can themselves contain the sole reference site for a
    // still-unclaimed single-use function - embed those too.
    InlineSingleUseFunctions(*chr_root, tally, data, funcs, gd);

    // A function this file owns but that a slot in another table references is displayed in that
    // other tab, so it isn't an orphan here; only functions unreferenced by every table's slots
    // land under "Other Functions", in their owning file's tab.
    const auto pool_slot_refs = CollectAllSlotReferences(pool, gd);
    auto orphans = chr_root->AddChild(ScriptTreeNodeType::ROOT, "Other Functions");
    for (const auto& f : funcs)
    {
        if (pool_slot_refs.find(f) == pool_slot_refs.end())
        {
            AddFunc(*orphans, *data->GetMapping(f), gd);
        }
    }
}

void AddCsMapping(ScriptTreeNode* root, std::shared_ptr<ScriptFunctionTable> data, std::shared_ptr<std::vector<ScriptTable::Action>> table, const std::string& desc, std::shared_ptr<GameData> gd)
{
    // Shares one function namespace with every other file (see AddChrMapping / BuildFullPool).
    const FunctionPool pool = BuildFullPool(gd);

    auto chr_root = root->AddChild(ScriptTreeNodeType::ROOT, desc);
    std::set<std::string> funcs(data->GetFunctionNames().cbegin(), data->GetFunctionNames().cend());
    auto location_anchors = BuildLocationAnchors(data, table);

    const auto tally = TallyPoolReferences(pool, gd);
    const auto ref_counts = BuildPoolRefCounts(pool, tally, gd);

    std::vector<std::pair<ScriptTreeNode*, std::string>> pending_related;

    int cs_idx = 0;
    for (const auto& entry : *table)
    {
        auto chr = chr_root->AddChild(ScriptTreeNodeType::ROOT, StrWPrintf("Cutscene Script %03d", cs_idx));
        std::set<std::string> nested_in_object;
        bool merged_function = false;
        AddMappingActionSlot(*chr, ScriptTreeNodeType::ROOT, "On Start:", entry, pool, funcs, ref_counts, nested_in_object,
            [table, cs_idx](const ScriptTable::Action& a) { table->at(cs_idx) = a; }, gd, merged_function);
        if (merged_function)
        {
            // Bare "Function:" headers always sit at the entry's top level, below the slots.
            pending_related.emplace_back(chr, std::get<std::string>(entry));
        }
        ++cs_idx;
    }

    // See AddChrMapping's comments above - embed single-use functions first, then attach
    // related functions, then embed any newly exposed single-use reference sites.
    InlineSingleUseFunctions(*chr_root, tally, data, funcs, gd);
    for (const auto& [slot, owner_func] : pending_related)
    {
        AddLocationRelatedFunctions(*slot, owner_func, data, funcs, location_anchors, gd);
    }
    InlineSingleUseFunctions(*chr_root, tally, data, funcs, gd);

    // Orphans excluded if referenced by any table's slots (shown in that tab) - see AddChrMapping.
    const auto pool_slot_refs = CollectAllSlotReferences(pool, gd);
    auto orphans = chr_root->AddChild(ScriptTreeNodeType::ROOT, "Other Functions");
    for (const auto& f : funcs)
    {
        if (pool_slot_refs.find(f) == pool_slot_refs.end())
        {
            AddFunc(*orphans, *data->GetMapping(f), gd);
        }
    }
}

void AddShopMapping(ScriptTreeNode* root, std::shared_ptr<ScriptFunctionTable> data, std::shared_ptr<std::vector<ScriptTable::Shop>> table, const std::string& desc, std::shared_ptr<GameData> gd)
{
    // Shares one function namespace with every other file (see AddChrMapping / BuildFullPool).
    const FunctionPool pool = BuildFullPool(gd);

    auto chr_root = root->AddChild(ScriptTreeNodeType::ROOT, desc);
    std::set<std::string> funcs(data->GetFunctionNames().cbegin(), data->GetFunctionNames().cend());
    auto ref_sources = BuildFunctionReferenceSources(data, gd);
    auto fallthrough_targets = BuildFallthroughTargets(data);
    AddFallthroughReferenceSources(ref_sources, fallthrough_targets);

    const auto tally = TallyPoolReferences(pool, gd);
    const auto ref_counts = BuildPoolRefCounts(pool, tally, gd);

    std::vector<ScriptTreeNode*> pending_related;

    int shop_idx = 0;
    for (const auto& entry : *table)
    {
        auto chr = chr_root->AddChild(ScriptTreeNodeType::SHOP, StrWPrintf("Shop %02d: %03d - Markup: %lf%%, Lifestock: %lf%%", shop_idx, entry.room, (entry.markup * 100.0) / 16.0 - 100.0, (entry.lifestock_markup * 100.0) / 16.0 - 100.0));
        std::vector<std::string> entries = { "On Enter:", "On Exit:", "On Pick Up:", "On Pay:", "On Steal:" };
        std::set<std::string> nested_in_object;
        for (std::size_t i = 0; i < entry.actions.size(); ++i)
        {
            const auto& action = entry.actions.at(i);
            bool merged_function = false;
            AddMappingActionSlot(*chr, ScriptTreeNodeType::SHOP_TABLE, entries.at(i), action, pool, funcs, ref_counts, nested_in_object,
                [table, shop_idx, i](const ScriptTable::Action& a) { table->at(shop_idx).actions[i] = a; }, gd, merged_function);
            // Related functions attach to the entry itself, not the slot - bare "Function:"
            // headers always sit at the entry's top level, below the slots.
            if (merged_function && (pending_related.empty() || pending_related.back() != chr))
            {
                pending_related.push_back(chr);
            }
        }
        ++shop_idx;
    }

    // Embed single-use functions beneath their referencing Script Action node first, so the
    // related-function search below doesn't instead attach them as separate sibling
    // "Function:" header nodes at the slot level.
    InlineSingleUseFunctions(*chr_root, tally, data, funcs, gd);

    // Pull in "single use" related functions only after every slot's own, direct action-string
    // match has already been claimed above - otherwise a shop processed later (e.g. after being
    // reordered via the UI) could lose its own function to an earlier shop's related-function
    // search purely because of iteration order, even though the underlying data didn't change.
    for (ScriptTreeNode* gchr : pending_related)
    {
        AddSingleUseRelatedFunctions(*gchr, *gchr, data, funcs, ref_sources, fallthrough_targets, gd);
    }
    // The related functions just attached can themselves contain the sole reference site for a
    // still-unclaimed single-use function - embed those too.
    InlineSingleUseFunctions(*chr_root, tally, data, funcs, gd);

    // A function this tab owns but that a slot in another table references is displayed over in
    // that tab, not here - so it isn't an orphan (that's why ShopMap_07 no longer shows up loose
    // in the Shops tab). Truly unreferenced functions still land here, in their owning file's tab.
    const auto pool_slot_refs = CollectAllSlotReferences(pool, gd);
    auto orphans = chr_root->AddChild(ScriptTreeNodeType::ROOT, "Other Functions");
    for (const auto& f : funcs)
    {
        if (pool_slot_refs.find(f) == pool_slot_refs.end())
        {
            AddFunc(*orphans, *data->GetMapping(f), gd);
        }
    }
}

void AddItemMapping(ScriptTreeNode* root, std::shared_ptr<ScriptFunctionTable> data, std::shared_ptr<std::vector<ScriptTable::Item>> table, const std::string& desc, std::shared_ptr<GameData> gd)
{
    // Shares one function namespace with every other file (see AddChrMapping / BuildFullPool).
    const FunctionPool pool = BuildFullPool(gd);

    auto chr_root = root->AddChild(ScriptTreeNodeType::ROOT, desc);
    std::set<std::string> funcs(data->GetFunctionNames().cbegin(), data->GetFunctionNames().cend());
    auto location_anchors = BuildLocationAnchors(data, table);

    const auto tally = TallyPoolReferences(pool, gd);
    const auto ref_counts = BuildPoolRefCounts(pool, tally, gd);

    std::vector<std::pair<ScriptTreeNode*, std::string>> pending_related;

    int item_idx = 0;
    for (const auto& entry : *table)
    {
        auto chr = chr_root->AddChild(ScriptTreeNodeType::CUSTOM_ITEM_SHOP_TABLE, StrWPrintf("Item Script %02d: Item %02d (%ls), Shop: %03d%ls",
            item_idx, entry.item, gd->GetStringData()->GetItemDisplayName(entry.item).c_str(), entry.shop, entry.other ? StrWPrintf("Additional Data %04X", *entry.other).c_str() : L""));
        std::vector<std::string> entries = { "On Pick Up:", "On Pay:", "On Steal:"};
        std::set<std::string> nested_in_object;
        for (std::size_t i = 0; i < entry.actions.size(); ++i)
        {
            const auto& action = entry.actions.at(i);
            wxString label = (i < entries.size()) ? wxString(entries.at(i)) : StrPrintf("Custom Action %d:", i + 1);
            bool merged_function = false;
            AddMappingActionSlot(*chr, ScriptTreeNodeType::CUSTOM_ITEM_SHOP_TABLE, label, action, pool, funcs, ref_counts, nested_in_object,
                [table, item_idx, i](const ScriptTable::Action& a) { table->at(item_idx).actions[i] = a; }, gd, merged_function);
            // Bare "Function:" headers always sit at the entry's top level, below the slots.
            if (merged_function)
            {
                pending_related.emplace_back(chr, std::get<std::string>(action));
            }
        }
        ++item_idx;
    }

    // See AddChrMapping's comments above - embed single-use functions first, then attach
    // related functions, then embed any newly exposed single-use reference sites.
    InlineSingleUseFunctions(*chr_root, tally, data, funcs, gd);
    for (const auto& [slot, owner_func] : pending_related)
    {
        AddLocationRelatedFunctions(*slot, owner_func, data, funcs, location_anchors, gd);
    }
    InlineSingleUseFunctions(*chr_root, tally, data, funcs, gd);

    // Functions this file owns but that a slot in another table references are shown in that tab,
    // so they aren't orphans here (mirror of the exclusion in AddShopMapping - fixes Shop_08 etc.
    // being displayed loose here as well as broken in Shops).
    const auto pool_slot_refs = CollectAllSlotReferences(pool, gd);
    auto orphans = chr_root->AddChild(ScriptTreeNodeType::ROOT, "Other Functions");
    for (const auto& f : funcs)
    {
        if (pool_slot_refs.find(f) == pool_slot_refs.end())
        {
            AddFunc(*orphans, *data->GetMapping(f), gd);
        }
    }
}


ScriptTreeNode ScriptTreeNode::BuildTree(std::shared_ptr<GameData> gd)
{
    ScriptTreeNode root{ScriptTreeNodeType::ROOT, "Root", nullptr};
    AddShopMapping(&root, gd->GetScriptData()->GetShopFuncs(), gd->GetScriptData()->GetShopTable(), "Shops", gd);
    AddItemMapping(&root, gd->GetScriptData()->GetItemFuncs(), gd->GetScriptData()->GetItemTable(), "Shops : Custom Items", gd);
    AddChrMapping(&root, gd->GetScriptData()->GetCharFuncs(), gd->GetScriptData()->GetCharTable(), "Characters", gd);
    AddCsMapping(&root, gd->GetScriptData()->GetCutsceneFuncs(), gd->GetScriptData()->GetCutsceneTable(), "Cutscenes", gd);
    root.SetParent();
    std::cerr << "### BuildTree" << std::endl;
    root.PrintChildren();
    return root;
}

ScriptTreeNode ScriptTreeNode::BuildCategoryTree(std::shared_ptr<GameData> gd, int category_index)
{
    ScriptTreeNode root{ ScriptTreeNodeType::ROOT, "Root", nullptr };
    switch (category_index)
    {
    case 0:
        AddShopMapping(&root, gd->GetScriptData()->GetShopFuncs(), gd->GetScriptData()->GetShopTable(), "Shops", gd);
        break;
    case 1:
        AddItemMapping(&root, gd->GetScriptData()->GetItemFuncs(), gd->GetScriptData()->GetItemTable(), "Shops : Custom Items", gd);
        break;
    case 2:
        AddChrMapping(&root, gd->GetScriptData()->GetCharFuncs(), gd->GetScriptData()->GetCharTable(), "Characters", gd);
        break;
    case 3:
        AddCsMapping(&root, gd->GetScriptData()->GetCutsceneFuncs(), gd->GetScriptData()->GetCutsceneTable(), "Cutscenes", gd);
        break;
    default:
        break;
    }
    root.SetParent();
    return root.children.empty() ? ScriptTreeNode{} : std::move(root.children.front());
}

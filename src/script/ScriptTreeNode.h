#ifndef _SCRIPT_TREE_NODE_H_
#define _SCRIPT_TREE_NODE_H_

#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <wx/string.h>
#include <wx/dataview.h>

#include <landstalker/main/GameData.h>
#include <landstalker/script/ScriptFunctionTable.h>
#include <landstalker/script/ScriptStatements.h>
#include <landstalker/script/ScriptTable.h>

enum class ScriptTreeNodeType
{
    INVALID,
    ROOT,
    TABLE,
    TABLE_ENTRY,
    ACTION,
    FUNCTION,
    SCRIPT_ENTRY,
    SCRIPT_TEXT,
    SCRIPT_SET_ITEM,
    SCRIPT_SET_GLOBAL_CHAR,
    SCRIPT_SET_NUM,
    SCRIPT_GIVE_ITEM,
    SCRIPT_GIVE_MONEY,
    SCRIPT_SET_FLAG,
    SCRIPT_INIT_CUTSCENE,
    SCRIPT_PLAY_BGM,
    SCRIPT_SET_SPEAKER,
    SCRIPT_SET_GLOBAL_SPEAKER,
    SCRIPT_CUSTOM,
    PROG_DEP_TABLE,
    PROG_DEP_ACTION,
    QUESTION,
    Q_ON_YES,
    Q_ON_NO,
    Q_PROMPT,
    CHECK_FLAG,
    CF_ON_CLEAR,
    CF_ON_SET,
    SET_FLAG,
    SF_ON_CLEAR,
    SF_ON_SET,
    DISPLAY_PRICE,
    SHOP,
    SHOP_ON_SALE_PROMPT,
    SHOP_ON_SALE_CONFIRM,
    SHOP_ON_SALE_CANCEL,
    SHOP_ON_SALE_NO_MONEY,
    CHURCH,
    CHURCH_NORMAL_PRIEST,
    CHURCH_SKELETON_PRIEST,
    SHOP_TABLE,
    CUSTOM_ITEM_SHOP_TABLE,
    SHOP_TABLE_ON_ENTER,
    SHOP_TABLE_ON_EXIT,
    SHOP_TABLE_ON_PICKUP,
    SHOP_TABLE_ON_PURCHASE,
    SHOP_TABLE_CUSTOM,
    BRANCH,
    PLAY_SOUND,
    CUSTOM_SHOP_ACTION,
    SLEEP,
    CUSTOM_ASM,
    CUSTOM_ASM_LINE,
    RETURN
};

struct ScriptTreeNode
{
    ScriptTreeNodeType type = ScriptTreeNodeType::INVALID;
    wxString name;
    ScriptTreeNode* parent = nullptr;
    std::list<ScriptTreeNode> children;
    // Set only on nodes representing a raw top-level table slot (Character/Cutscene/Shop/Item
    // entry); writes the edited value back into the underlying ScriptTable vector so that it
    // gets picked up by the project save ("Output ASM").
    std::function<void(const Landstalker::ScriptTable::Action&)> write_back;
    // Set only on a Custom Item Shop Action container (the "Item Script XX: ..." top-level entry)
    // - rebuilds the underlying ScriptTable::Item's whole actions vector from this container's
    // current children, in order. Unlike write_back above (which only ever overwrites a single
    // already-existing table position), this container's children are structurally user-managed -
    // added, removed and reordered - so persisting them needs to replace the whole vector, not one
    // slot in it.
    std::function<void(const std::vector<Landstalker::ScriptTable::Action>&)> write_back_actions;

    // Generic typed payload, interpreted per node type - the authoritative copy of each row's
    // value. Sync, validity checks and editors all read these; the display text is
    // presentation only and is never parsed back:
    //   SCRIPT_ENTRY: numeric_value = script line/id
    //   SET_FLAG / CHECK_FLAG: numeric_value = flag number
    //   PROG_DEP_ACTION: numeric_value = quest, numeric_value2 = progress
    //   PLAY_SOUND: numeric_value = sound id
    //   CUSTOM_SHOP_ACTION: numeric_value = custom item script id
    //   SLEEP: numeric_value = ticks
    //   BRANCH: text_value = label, bool_value = wide
    //   FUNCTION: text_value = function name
    //   action rows (anything with an action_label): text_value = referenced function name
    //     (empty for a raw script-ID action), numeric_value = starting script ID
    uint16_t numeric_value = 0;
    uint16_t numeric_value2 = 0;
    std::string text_value;
    bool bool_value = false;

    // The human-readable prefix an editable Script-Action node was built with (e.g. "On Pay:",
    // "Prompt:", "Script Action:"). AddAction()/PopulateScriptAction() read this to rebuild the
    // full display text ("<action_label> Script ID XXXX") without losing the slot's own label.
    wxString action_label;
    // Set when this node's own text already embeds a fully resolved function body (i.e. a
    // "<Label> Function: X" node whose children are X's statements, not a bare reference to X).
    // Lets GetFunctionNodeName() recognise merged slots the same way it recognises a standalone
    // FUNCTION node, so duplicate-detection for related/orphan functions still works.
    std::string embedded_function_name;

    wxDataViewItem ToItem() const
    {
        return wxDataViewItem(const_cast<void*>(reinterpret_cast<const void*>(this)));
    }

    wxDataViewItemArray GetChildren() const
    {
        wxDataViewItemArray carr;
        for (const auto& c : children)
        {
            carr.push_back(wxDataViewItem(const_cast<void*>(reinterpret_cast<const void*>(&c))));
        }
        return carr;
    }

    ScriptTreeNode* AddChild(ScriptTreeNodeType p_type, wxString string)
    {
        children.push_back(ScriptTreeNode{ p_type, string });
        return &children.back();
    }

    static ScriptTreeNode BuildTree(std::shared_ptr<Landstalker::GameData> gd);
    // Rebuilds just one top-level category (0=Shops, 1=Custom Items, 2=Characters, 3=Cutscenes),
    // matching BuildTree()'s per-category mapping calls - used to refresh a single category after
    // an entry is added/removed/moved without paying the cost of rebuilding every category.
    static ScriptTreeNode BuildCategoryTree(std::shared_ptr<Landstalker::GameData> gd, int category_index);

    void SetParent(ScriptTreeNode* p_parent = nullptr)
    {
        parent = p_parent;
        for (auto& c : children)
        {
            c.SetParent(this);
        }
    }

    void PrintChildren(int indent = 0) const
    {
        std::cerr << std::string(indent, ' ') << name << std::endl;
        for (const auto& c : children)
        {
            c.PrintChildren(indent + 2);
        }
    };
};

struct ScriptTreeAddOption
{
    ScriptTreeNodeType type;
    wxString label;
    wxString default_name;
};

// The Custom Item Shop table's fixed leading action slots, in order - shared between
// ScriptTreeBuilder (initial construction from ScriptTable::Item::actions) and
// ScriptTreeDataViewModel (numbering/positioning newly added entries the same way, and
// protecting these three from being removed or reordered). Only entries beyond these first
// three are the user-managed, sequentially numbered "Custom Action N" slots.
inline const std::array<const char*, 3> kCustomItemShopActionFixedLabels = { "On Pick Up:", "On Pay:", "On Steal:" };

void AddAction(ScriptTreeNode& node, const Landstalker::Statements::Action& action,
               std::shared_ptr<Landstalker::GameData> gd, const wxString& label = "Script Action:");
void AddFuncBody(ScriptTreeNode& node, const Landstalker::ScriptFunction& func,
                 std::shared_ptr<Landstalker::GameData> gd);
// Collects the names of every function whose body is displayed (not merely referenced) somewhere
// within the given subtree - standalone "Function:" nodes, merged slots and embedded actions.
void CollectDisplayFunctionNames(const ScriptTreeNode& node, std::set<std::string>& names);

#endif // _SCRIPT_TREE_NODE_H_

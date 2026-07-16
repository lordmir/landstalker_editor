#include <script/ScriptTreeDataViewModel.h>

#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>

#include <algorithm>
#include <sstream>

using namespace Landstalker;

void ScriptTreeDataViewModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int /*col*/) const
{
    auto id = reinterpret_cast<intptr_t>(item.GetID());
    const ScriptTreeNode* node;
    if (id == 0)
    {
        node = &root;
    }
    else
    {
        node = reinterpret_cast<const ScriptTreeNode*>(id);
    }
    variant = node->name;
}

bool ScriptTreeDataViewModel::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int /*col*/)
{
    const ScriptTreeNode* target = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    if (!target)
    {
        return false;
    }
    const std::string text = Trim(variant.GetString().ToStdString());

    // Most editors commit a plain payload; the node's own type determines what it means (see
    // ScriptActionRenderer::GetValueFromEditorCtrl, which builds these).
    switch (target->type)
    {
    case ScriptTreeNodeType::FUNCTION:
        // A bare function header isn't a script action - committing an edit renames it.
        return RenameDisplayedFunction(item, text);
    case ScriptTreeNodeType::BRANCH:
        // The branch label, optionally with the display convention's trailing " W".
        return ApplyBranchLabel(item, text);
    case ScriptTreeNodeType::PROG_DEP_ACTION:
        // "<quest> <progress>", from the twin spin-control editor of a Progress Entry row.
        try
        {
            std::size_t next = 0;
            const uint16_t quest = static_cast<uint16_t>(std::stoul(text, &next));
            const uint16_t progress = static_cast<uint16_t>(std::stoul(text.substr(next)));
            return ApplyQuestProgress(item, quest, progress);
        }
        catch (...)
        {
            return false;
        }
    case ScriptTreeNodeType::PLAY_SOUND:
    case ScriptTreeNodeType::SET_FLAG:
    case ScriptTreeNodeType::CHECK_FLAG:
    case ScriptTreeNodeType::SLEEP:
    case ScriptTreeNodeType::CUSTOM_SHOP_ACTION:
        // A single decimal number from the spin-control editor.
        try
        {
            return ApplyNumericValue(item, static_cast<uint16_t>(std::stoul(text)));
        }
        catch (...)
        {
            return false;
        }
    default:
        break;
    }

    // Everything else is an action row; the Script Action editor's payload carries its own
    // mode tag, since "function reference"/"new function"/"raw script ID" can't be told apart
    // from the node type alone.
    const std::string new_func_prefix = "New Function: ";
    const std::string func_prefix = "Function: ";
    const std::string id_prefix = "Script ID: ";
    if (text.rfind(new_func_prefix, 0) == 0)
    {
        const std::string name = Trim(text.substr(new_func_prefix.size()));
        if (!IsValidNewFunctionName(name))
        {
            return false;
        }
        CreateFunction(name);
        return ApplyScriptAction(item, true, name, 0);
    }
    if (text.rfind(func_prefix, 0) == 0)
    {
        const std::string name = Trim(text.substr(func_prefix.size()));
        // Free-typed in the editor's combo box, so gate on label syntax: a name that isn't an
        // existing function must at least be a plausible ASM label, or the reference would
        // produce garbage ASM output.
        if (!HasFunction(name) && !IsValidLabelSyntax(name))
        {
            return false;
        }
        return ApplyScriptAction(item, true, name, 0);
    }
    if (text.rfind(id_prefix, 0) == 0)
    {
        try
        {
            uint16_t id = static_cast<uint16_t>(std::stoul(Trim(text.substr(id_prefix.size())), nullptr, 16));
            return ApplyScriptAction(item, false, "", id);
        }
        catch (...)
        {
            return false;
        }
    }
    return false;
}

bool ScriptTreeDataViewModel::ApplyNumericValue(const wxDataViewItem& item, uint16_t value)
{
    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!node)
    {
        return false;
    }

    // Rebuild the display text in exactly the format AddFuncBody() builds it with, so the
    // label always reflects the new value.
    switch (node->type)
    {
    case ScriptTreeNodeType::PLAY_SOUND:
        node->name = StrWPrintf("Play Sound: %d (%ls)", value, Labels::Get(Labels::C_SOUNDS, value).value_or(L"").c_str());
        break;
    case ScriptTreeNodeType::SET_FLAG:
        node->name = "Set Flag " + Hex(value) + " on Talk:";
        break;
    case ScriptTreeNodeType::CHECK_FLAG:
        node->name = "Check Flag " + Hex(value) + ":";
        break;
    case ScriptTreeNodeType::SLEEP:
        node->name = StrWPrintf("Sleep: %d frames", value);
        break;
    case ScriptTreeNodeType::CUSTOM_SHOP_ACTION:
        node->name = StrWPrintf("Custom Item Script: %d", value);
        break;
    default:
        return false;
    }
    node->numeric_value = value;
    ItemChanged(item);
    SyncToScriptTable();
    return true;
}

bool ScriptTreeDataViewModel::ApplyBranchLabel(const wxDataViewItem& item, const std::string& text)
{
    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!node || node->type != ScriptTreeNodeType::BRANCH)
    {
        return false;
    }

    // The display convention marks a wide branch with a trailing " W" - accept the same in the
    // edited text, so the flag can be toggled from the editor too.
    std::string label = text;
    bool wide = false;
    const std::string wide_suffix = " W";
    if (label.size() > wide_suffix.size()
        && label.compare(label.size() - wide_suffix.size(), wide_suffix.size(), wide_suffix) == 0)
    {
        wide = true;
        label = Trim(label.substr(0, label.size() - wide_suffix.size()));
    }
    // A branch target is a single ASM label - anything else either isn't a jump target at all
    // or (e.g. "[X") would throw mid-parse when SyncToScriptTable() re-reads the "- Branch:"
    // YAML this serializes to.
    if (!HasFunction(label) && !IsValidLabelSyntax(label))
    {
        return false;
    }

    node->text_value = label;
    node->bool_value = wide;
    node->name = "Branch: " + label + (wide ? " W" : "");
    ItemChanged(item);
    SyncToScriptTable();
    return true;
}

bool ScriptTreeDataViewModel::ApplyQuestProgress(const wxDataViewItem& item, uint16_t quest, uint16_t progress)
{
    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!node || node->type != ScriptTreeNodeType::PROG_DEP_ACTION)
    {
        return false;
    }

    node->numeric_value = quest;
    node->numeric_value2 = progress;
    node->name = StrPrintf("On Quest %d, Progress %d:", quest, progress);
    ItemChanged(item);
    SyncToScriptTable();
    return true;
}

wxDataViewItem ScriptTreeDataViewModel::GetCustomAsmBlock(const wxDataViewItem& item) const
{
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    if (!node)
    {
        return wxDataViewItem();
    }
    if (node->type == ScriptTreeNodeType::CUSTOM_ASM)
    {
        return item;
    }
    if (node->type == ScriptTreeNodeType::CUSTOM_ASM_LINE && node->parent && node->parent->type == ScriptTreeNodeType::CUSTOM_ASM)
    {
        return node->parent->ToItem();
    }
    return wxDataViewItem();
}

wxString ScriptTreeDataViewModel::GetCustomAsmText(const wxDataViewItem& item) const
{
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    if (!node || node->type != ScriptTreeNodeType::CUSTOM_ASM)
    {
        return wxString();
    }
    wxString text;
    for (const auto& child : node->children)
    {
        if (child.type == ScriptTreeNodeType::CUSTOM_ASM_LINE)
        {
            if (!text.empty())
            {
                text += "\n";
            }
            text += child.name;
        }
    }
    return text;
}

bool ScriptTreeDataViewModel::ApplyCustomAsm(const wxDataViewItem& item, const wxString& text)
{
    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!node || node->type != ScriptTreeNodeType::CUSTOM_ASM)
    {
        return false;
    }

    // Validate before touching the node: every non-blank line must parse as one instruction,
    // and at least one must remain (an empty block can't be serialized).
    std::vector<std::string> lines;
    try
    {
        std::stringstream ss(text.ToStdString());
        std::string line;
        while (std::getline(ss, line))
        {
            line = Trim(line);
            if (line.empty())
            {
                continue;
            }
            const AsmFile::Instruction ins = AsmFile::Instruction::FromLine("\t" + line);
            // Instructions the script reader interprets structurally can't live inside a
            // custom ASM block: on reload they'd become separate statements ("rts" a Return,
            // "bra" a Branch, traps 0-2 sound/action/item-script statements), silently
            // restructuring the function - or ending it early, orphaning what follows.
            if (ins.mnemonic == "rts" || ins.mnemonic == "bra")
            {
                return false;
            }
            if (ins.mnemonic == "trap" && !ins.operands.empty()
                && std::holds_alternative<AsmFile::Immediate>(ins.operands.front()))
            {
                const int trap_number = static_cast<int>(std::get<AsmFile::Immediate>(ins.operands.front()));
                if (trap_number >= 0 && trap_number <= 2)
                {
                    return false;
                }
            }
            lines.push_back(Trim(ins.ToLine()));
        }
    }
    catch (...)
    {
        return false;
    }
    if (lines.empty())
    {
        return false;
    }

    wxDataViewItemArray old_children = node->GetChildren();
    node->children.clear();
    for (const auto& line : lines)
    {
        node->AddChild(ScriptTreeNodeType::CUSTOM_ASM_LINE, wxString(line));
    }
    node->SetParent(node->parent);

    const wxDataViewItem node_item = node->ToItem();
    if (!old_children.empty())
    {
        ItemsDeleted(node_item, old_children);
    }
    wxDataViewItemArray new_children = node->GetChildren();
    if (!new_children.empty())
    {
        ItemsAdded(node_item, new_children);
    }
    ItemChanged(node_item);
    SyncToScriptTable();
    return true;
}

namespace
{
    bool NodeAffectsFunctionReferences(const ScriptTreeNode& node)
    {
        // Function displays/definitions, branches, ASM label mentions and named action
        // references all feed the reference analysis ([Shared xN] counts, "Other Functions"
        // membership, single-use inlining); plain value statements (Return, Sleep, sounds,
        // flags with raw script-ID actions, ...) don't.
        if (node.type == ScriptTreeNodeType::FUNCTION || node.type == ScriptTreeNodeType::BRANCH
            || node.type == ScriptTreeNodeType::CUSTOM_ASM || node.type == ScriptTreeNodeType::CUSTOM_ASM_LINE
            || !node.embedded_function_name.empty() || !node.text_value.empty())
        {
            return true;
        }
        for (const auto& child : node.children)
        {
            if (NodeAffectsFunctionReferences(child))
            {
                return true;
            }
        }
        return false;
    }
}

bool ScriptTreeDataViewModel::SubtreeAffectsReferences(const wxDataViewItem& item) const
{
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    return node && NodeAffectsFunctionReferences(*node);
}

bool ScriptTreeDataViewModel::IsInsideStandaloneFunction(const wxDataViewItem& item) const
{
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    if (!node)
    {
        return false;
    }
    while (node->parent && node->parent != &root)
    {
        node = node->parent;
    }
    return node->parent == &root && node->type == ScriptTreeNodeType::FUNCTION;
}

bool ScriptTreeDataViewModel::HasFunction(const std::string& name) const
{
    return FindOwningTable(name) != nullptr;
}

ScriptFunctionTable* ScriptTreeDataViewModel::FindOwningTable(const std::string& name) const
{
    for (const auto& table : m_pool)
    {
        if (table && table->GetMapping(name) != nullptr)
        {
            return table.get();
        }
    }
    return nullptr;
}

bool ScriptTreeDataViewModel::IsValidLabelSyntax(const std::string& name)
{
    if (name.empty() || name.size() > 50)
    {
        return false;
    }
    if (name.front() >= '0' && name.front() <= '9')
    {
        return false;
    }
    for (char c : name)
    {
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok)
        {
            return false;
        }
    }
    return true;
}

bool ScriptTreeDataViewModel::IsValidNewFunctionName(const std::string& name) const
{
    return IsValidLabelSyntax(name) && !HasFunction(name);
}

void ScriptTreeDataViewModel::CreateFunction(const std::string& name)
{
    if (!m_functions || HasFunction(name))
    {
        return;
    }
    // Position the new function directly after the last primary-table function this entry
    // displays, keeping it inside the entry's location run so the next category rebuild
    // redisplays it under this entry (see the matching logic in SyncToScriptTable()).
    std::string insert_anchor;
    std::vector<ScriptTreeNode*> function_nodes;
    CollectFunctionNodes(root, function_nodes);
    for (const ScriptTreeNode* node : function_nodes)
    {
        const std::string displayed = GetFunctionName(*node);
        if (m_functions->GetMapping(displayed) != nullptr)
        {
            insert_anchor = displayed;
        }
    }
    Statements::ScriptStatementVector statements;
    statements.push_back(Statements::Rts());
    m_functions->InsertFunctionAfter(ScriptFunction(name, statements), insert_anchor);
}

void ScriptTreeDataViewModel::RenameTableReferences(const std::string& old_name, const std::string& new_name)
{
    auto sd = m_gd ? m_gd->GetScriptData() : nullptr;
    if (!sd)
    {
        return;
    }
    auto rename_action = [&](ScriptTable::Action& action)
    {
        if (std::string* name = std::get_if<std::string>(&action))
        {
            if (*name == old_name)
            {
                *name = new_name;
            }
        }
    };
    // Update every script table that can reference this pool's functions. The shop and custom-
    // item tables share one function pool and reference each other's functions, so a rename in
    // either must be followed in both tables.
    for (const auto& table : m_pool)
    {
        if (table == sd->GetCharFuncs())
        {
            for (auto& action : *sd->GetCharTable())
            {
                rename_action(action);
            }
        }
        else if (table == sd->GetCutsceneFuncs())
        {
            for (auto& action : *sd->GetCutsceneTable())
            {
                rename_action(action);
            }
        }
        else if (table == sd->GetShopFuncs())
        {
            for (auto& shop : *sd->GetShopTable())
            {
                for (auto& action : shop.actions)
                {
                    rename_action(action);
                }
            }
        }
        else if (table == sd->GetItemFuncs())
        {
            for (auto& item : *sd->GetItemTable())
            {
                for (auto& action : item.actions)
                {
                    rename_action(action);
                }
            }
        }
    }
}

bool ScriptTreeDataViewModel::RenameDisplayedFunction(const wxDataViewItem& item, const std::string& new_name)
{
    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!node || !m_functions)
    {
        return false;
    }

    const std::string old_name = GetFunctionName(*node);
    if (new_name == old_name)
    {
        return false;
    }
    // Rename in whichever pool table actually defines the function - in a shared pool the
    // displayed function may be owned by a different table than this tab's primary one (e.g.
    // Shop_08, defined in the custom-item file, displayed via a shop-table slot).
    ScriptFunctionTable* owner = FindOwningTable(old_name);
    if (!IsValidNewFunctionName(new_name) || !owner || !owner->RenameFunction(old_name, new_name))
    {
        return false;
    }
    // RenameFunction only rewrites references within the owning table; references held by the
    // OTHER pool tables' functions (cross-file jumps/branches) must be fixed too.
    for (const auto& table : m_pool)
    {
        if (table && table.get() != owner)
        {
            table->RenameReferences(old_name, new_name);
        }
    }
    // The table slots referencing the function by name must follow the rename, or they'd
    // resolve to nothing on the next rebuild.
    RenameTableReferences(old_name, new_name);
    // ...as must the auto-removal tracking, or the next sync would see old_name as "no longer
    // displayed" and try to delete the renamed function.
    if (m_known_function_names.erase(old_name) > 0)
    {
        m_known_function_names.insert(new_name);
    }

    node->name = "Function: " + new_name;
    node->text_value = new_name;
    ItemChanged(item);
    // Deliberately no SyncToScriptTable() here: other nodes in this model still display the old
    // name in their reference rows, and syncing from that stale display would undo the rename
    // just performed on the data. The category rebuild that follows every edit refreshes all
    // displays from the (already consistent) data instead.
    return true;
}

bool ScriptTreeDataViewModel::IsValidScriptId(uint16_t id) const
{
    return m_gd && m_gd->GetScriptData() && m_gd->GetScriptData()->GetScript()
        && id < m_gd->GetScriptData()->GetScript()->GetScriptLineCount();
}

wxString ScriptTreeDataViewModel::GetScriptIdPreview(uint16_t id) const
{
    if (!IsValidScriptId(id))
    {
        return wxString();
    }

    constexpr int max_preview_lines = 20;
    const std::size_t line_count = m_gd->GetScriptData()->GetScript()->GetScriptLineCount();

    wxString preview;
    uint16_t line = id;
    int lines_shown = 0;
    bool hit_end = false;
    while (line < line_count && lines_shown < max_preview_lines)
    {
        const ScriptTableEntry& entry = m_gd->GetScriptData()->GetScript()->GetScriptLine(line);
        if (lines_shown > 0)
        {
            preview += "\n";
        }
        preview += wxString(StrWPrintf("%04X - %ls", line, entry.ToString(m_gd).c_str()));
        ++lines_shown;
        if (entry.end)
        {
            hit_end = true;
            break;
        }
        ++line;
    }
    if (!hit_end)
    {
        preview += "\n...";
    }
    return preview;
}

wxString ScriptTreeDataViewModel::GetFunctionPreview(const std::string& name) const
{
    const ScriptFunctionTable* owner = FindOwningTable(name);
    const ScriptFunction* func = owner ? owner->GetMapping(name) : nullptr;
    if (!func)
    {
        return wxString();
    }

    constexpr int max_preview_lines = 20;
    std::istringstream iss(func->Print());
    std::string line;
    std::string preview;
    int lines_shown = 0;
    bool truncated = false;
    while (std::getline(iss, line))
    {
        if (lines_shown >= max_preview_lines)
        {
            truncated = true;
            break;
        }
        if (lines_shown > 0)
        {
            preview += "\n";
        }
        preview += line;
        ++lines_shown;
    }
    if (truncated)
    {
        preview += "\n...";
    }
    return wxString(preview);
}

std::vector<wxString> ScriptTreeDataViewModel::GetFunctionNameChoices() const
{
    // Offer every function in the shared pool - a shop slot can legitimately reference a
    // custom-item function and vice versa.
    std::vector<wxString> choices;
    std::set<std::string> seen;
    for (const auto& table : m_pool)
    {
        if (!table)
        {
            continue;
        }
        for (const auto& name : table->GetFunctionNames())
        {
            if (seen.insert(name).second)
            {
                choices.push_back(wxString(name));
            }
        }
    }
    return choices;
}

void ScriptTreeDataViewModel::PopulateScriptAction(ScriptTreeNode& node, bool is_function, const std::string& function_name, uint16_t script_id) const
{
    node.children.clear();
    node.embedded_function_name.clear();
    node.text_value.clear();
    node.numeric_value = 0;
    const wxString label = node.action_label.empty() ? wxString("Script Action:") : node.action_label;
    if (is_function)
    {
        // If the typed name matches a real function, embed its body directly (as the initial
        // tree build does for a table slot resolving to a known function) instead of always
        // falling back to a bare, childless reference - otherwise confirming an edit that keeps
        // (or retypes) the same function name would silently drop its child statements.
        // BUT only if the function's body isn't already displayed somewhere else in this entry's
        // tree (another slot, or an enclosing display of the function itself when making a
        // recursive reference). Embedding a second copy would hand SyncToScriptTable() two
        // divergent "definitions" of the same function, and whichever syncs last - usually the
        // stale copy - would silently overwrite the edit. A plain reference is also what the
        // post-edit category rebuild displays for this case, so this keeps the transient
        // edit-time view consistent with it. (This node's own former display doesn't count -
        // its state was cleared above - so re-confirming the same name still re-embeds.)
        const ScriptFunctionTable* owner = FindOwningTable(function_name);
        const ScriptFunction* func = owner ? owner->GetMapping(function_name) : nullptr;
        std::set<std::string> displayed;
        CollectDisplayFunctionNames(root, displayed);
        if (func && displayed.find(func->name) == displayed.end())
        {
            node.name = label + " Function: " + func->name;
            node.embedded_function_name = func->name;
            node.text_value = func->name;
            AddFuncBody(node, *func, m_gd);
        }
        else
        {
            AddAction(node, AsmFile::ScriptAction(function_name), m_gd, label);
        }
    }
    else if (IsValidScriptId(script_id))
    {
        try
        {
            AddAction(node, AsmFile::ScriptAction(script_id), m_gd, label);
        }
        catch (const std::exception&)
        {
            // The script ID chain ran past the end of the table before hitting
            // an entry marked as "end" - fall back to an unresolved display.
            node.children.clear();
            node.name = label + StrPrintf(" Script ID %04X", script_id);
            node.numeric_value = script_id;
        }
    }
    else
    {
        node.name = label + StrPrintf(" Script ID %04X", script_id);
        // Kept even though the ID is out of range, so the value survives further edits and
        // the row's red (invalid) styling reflects what was actually entered.
        node.numeric_value = script_id;
    }
    node.SetParent(node.parent);

    if (node.write_back)
    {
        node.write_back(is_function ? ScriptTable::Action(function_name) : ScriptTable::Action(script_id));
    }
}

void ScriptTreeDataViewModel::MoveAnchoredFunctionToGroupHead(const std::string& name)
{
    if (!m_functions || m_functions->GetMapping(name) == nullptr)
    {
        return;
    }
    const auto& order = m_functions->GetFunctionNames();
    std::map<std::string, std::size_t> index_of;
    for (std::size_t i = 0; i < order.size(); ++i)
    {
        index_of.emplace(order.at(i), i);
    }

    // The entry's standalone function headers (root children), with their table positions.
    std::list<ScriptTreeNode>& siblings = root.children;
    auto anchor_it = siblings.end();
    std::set<std::size_t> displayed_indices;
    for (auto it = siblings.begin(); it != siblings.end(); ++it)
    {
        if (it->type != ScriptTreeNodeType::FUNCTION)
        {
            continue;
        }
        const auto idx = index_of.find(GetFunctionName(*it));
        if (idx == index_of.end())
        {
            // Owned by another pool table - positioned in a different file entirely.
            continue;
        }
        displayed_indices.insert(idx->second);
        if (GetFunctionName(*it) == name)
        {
            anchor_it = it;
        }
    }
    if (anchor_it == siblings.end())
    {
        return;
    }

    // Walk down the contiguous run of displayed table positions ending at the anchor - the
    // anchor must lead exactly this run. Never reach across a gap: the functions beyond it
    // aren't part of this group, and dragging the anchor past them would reorder (and
    // potentially break the fall-through pairing of) unrelated functions.
    const std::size_t anchor_index = index_of.at(name);
    std::size_t low = anchor_index;
    while (low > 0 && displayed_indices.find(low - 1) != displayed_indices.end())
    {
        --low;
    }
    if (low == anchor_index)
    {
        return;
    }

    // Move the anchor's display node directly before the group head's; the sync that follows
    // (SetFunctionOrder() from display order) applies the same move to the table itself.
    for (auto it = siblings.begin(); it != siblings.end(); ++it)
    {
        if (it->type != ScriptTreeNodeType::FUNCTION)
        {
            continue;
        }
        const auto idx = index_of.find(GetFunctionName(*it));
        if (idx != index_of.end() && idx->second == low)
        {
            const wxDataViewItem moved = anchor_it->ToItem();
            siblings.splice(it, siblings, anchor_it);
            ItemDeleted(wxDataViewItem(), moved);
            ItemAdded(wxDataViewItem(), moved);
            return;
        }
    }
}

bool ScriptTreeDataViewModel::ApplyScriptAction(const wxDataViewItem& item, bool is_function, const std::string& function_name, uint16_t script_id)
{
    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!node)
    {
        return false;
    }

    wxDataViewItemArray old_children = node->GetChildren();

    PopulateScriptAction(*node, is_function, function_name, script_id);
    if (node->write_back && is_function)
    {
        // The slot now references this function, making it the entry's location anchor.
        MoveAnchoredFunctionToGroupHead(function_name);
    }
    SyncToScriptTable();

    const wxDataViewItem node_item = node->ToItem();
    if (!old_children.empty())
    {
        ItemsDeleted(node_item, old_children);
    }

    wxDataViewItemArray new_children = node->GetChildren();
    if (!new_children.empty())
    {
        ItemsAdded(node_item, new_children);
    }

    ItemChanged(node_item);
    return true;
}

bool ScriptTreeDataViewModel::IsEnabled(const wxDataViewItem& /*item*/, unsigned int /*col*/) const
{
    return true;
}

wxDataViewItem ScriptTreeDataViewModel::GetParent(const wxDataViewItem& item) const
{
    auto node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!node)
    {
        return wxDataViewItem();
    }
    return (node->parent == &root) ? wxDataViewItem() : wxDataViewItem(reinterpret_cast<void*>(node->parent));
}

bool ScriptTreeDataViewModel::IsContainer(const wxDataViewItem& item) const
{
    const ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!node)
    {
        node = &root;
    }
    return !node->children.empty();
}

unsigned int ScriptTreeDataViewModel::GetChildren(const wxDataViewItem& parent, wxDataViewItemArray& array) const
{
    const ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(parent.GetID());
    if (!node)
    {
        node = &root;
    }
    for (const auto& c : node->children)
    {
        array.push_back(wxDataViewItem(const_cast<void*>(reinterpret_cast<const void*>(&c))));
    }
    return array.size();
}

bool ScriptTreeDataViewModel::IsNodeValueValid(const ScriptTreeNode& node) const
{
    if (node.type == ScriptTreeNodeType::BRANCH)
    {
        return HasFunction(node.text_value);
    }
    // Action rows (anything built with an action_label): a function reference resolves when
    // the target exists (a displayed body means it necessarily does); a raw script ID when
    // it's in range. All read from the node's typed payload, never the display text.
    if (!node.action_label.empty())
    {
        if (!node.embedded_function_name.empty())
        {
            return true;
        }
        if (!node.text_value.empty())
        {
            return !node.children.empty() || HasFunction(node.text_value);
        }
        return IsValidScriptId(node.numeric_value);
    }
    return true;
}

bool ScriptTreeDataViewModel::GetAttr(const wxDataViewItem& item, unsigned int /*col*/, wxDataViewItemAttr& attr) const
{
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    if (!node)
    {
        return false;
    }

    ApplyClassAttr(*node, attr);

    // An unresolved value (unknown function/branch target, out-of-range script ID) overrides
    // the class colour but keeps the class font, so e.g. an invalid "On..." slot still reads
    // as bold, just in red instead of dark red.
    if (!IsNodeValueValid(*node))
    {
        attr.SetColour(wxColour(200, 0, 0));
    }
    return true;
}

void ScriptTreeDataViewModel::ApplyClassAttr(const ScriptTreeNode& node, wxDataViewItemAttr& attr) const
{
    // Top-level script action / table slots ("On Pay:", "On Talk:", ...) - the entry root's
    // direct children, except standalone function headers (which fall through to the default).
    if (node.parent == &root && node.type != ScriptTreeNodeType::FUNCTION)
    {
        attr.SetBold(true);
        attr.SetColour(wxColour(139, 0, 0));
        return;
    }

    switch (node.type)
    {
    // Nested script action slots ("OnYes:", "OnSalePrompt:", "Prompt:", "Script Action:", ...).
    case ScriptTreeNodeType::ACTION:
    case ScriptTreeNodeType::TABLE_ENTRY:
    case ScriptTreeNodeType::Q_PROMPT:
    case ScriptTreeNodeType::Q_ON_YES:
    case ScriptTreeNodeType::Q_ON_NO:
    case ScriptTreeNodeType::SF_ON_CLEAR:
    case ScriptTreeNodeType::SF_ON_SET:
    case ScriptTreeNodeType::CF_ON_CLEAR:
    case ScriptTreeNodeType::CF_ON_SET:
    case ScriptTreeNodeType::DISPLAY_PRICE:
    case ScriptTreeNodeType::SHOP_ON_SALE_PROMPT:
    case ScriptTreeNodeType::SHOP_ON_SALE_CONFIRM:
    case ScriptTreeNodeType::SHOP_ON_SALE_CANCEL:
    case ScriptTreeNodeType::SHOP_ON_SALE_NO_MONEY:
    case ScriptTreeNodeType::CHURCH_NORMAL_PRIEST:
    case ScriptTreeNodeType::CHURCH_SKELETON_PRIEST:
    case ScriptTreeNodeType::SHOP_TABLE:
    case ScriptTreeNodeType::CUSTOM_ITEM_SHOP_TABLE:
    case ScriptTreeNodeType::SHOP_TABLE_ON_ENTER:
    case ScriptTreeNodeType::SHOP_TABLE_ON_EXIT:
    case ScriptTreeNodeType::SHOP_TABLE_ON_PICKUP:
    case ScriptTreeNodeType::SHOP_TABLE_ON_PURCHASE:
    case ScriptTreeNodeType::SHOP_TABLE_CUSTOM:
        attr.SetBold(true);
        attr.SetColour(wxColour(102, 0, 153));
        return;

    // Uneditable reference/listing rows (resolved script lines, inline ASM, bare jump labels).
    case ScriptTreeNodeType::SCRIPT_ENTRY:
    case ScriptTreeNodeType::SCRIPT_TEXT:
    case ScriptTreeNodeType::SCRIPT_SET_ITEM:
    case ScriptTreeNodeType::SCRIPT_SET_GLOBAL_CHAR:
    case ScriptTreeNodeType::SCRIPT_SET_NUM:
    case ScriptTreeNodeType::SCRIPT_GIVE_ITEM:
    case ScriptTreeNodeType::SCRIPT_GIVE_MONEY:
    case ScriptTreeNodeType::SCRIPT_SET_FLAG:
    case ScriptTreeNodeType::SCRIPT_INIT_CUTSCENE:
    case ScriptTreeNodeType::SCRIPT_PLAY_BGM:
    case ScriptTreeNodeType::SCRIPT_SET_SPEAKER:
    case ScriptTreeNodeType::SCRIPT_SET_GLOBAL_SPEAKER:
    case ScriptTreeNodeType::SCRIPT_CUSTOM:
    case ScriptTreeNodeType::CUSTOM_ASM_LINE:
        attr.SetItalic(true);
        attr.SetColour(wxColour(96, 96, 96));
        return;

    // Parameterised (editable) non-action statements ("Set Flag 1234 on Talk:", "Sleep: 60", ...).
    case ScriptTreeNodeType::SET_FLAG:
    case ScriptTreeNodeType::CHECK_FLAG:
    case ScriptTreeNodeType::PROG_DEP_ACTION:
    case ScriptTreeNodeType::BRANCH:
    case ScriptTreeNodeType::PLAY_SOUND:
    case ScriptTreeNodeType::CUSTOM_SHOP_ACTION:
    case ScriptTreeNodeType::SLEEP:
        attr.SetBold(true);
        attr.SetColour(wxColour(0, 0, 139));
        return;

    // Fixed (parameterless) function statements ("Return", "Shop:", "Yes/No Question:", ...).
    case ScriptTreeNodeType::RETURN:
    case ScriptTreeNodeType::QUESTION:
    case ScriptTreeNodeType::SHOP:
    case ScriptTreeNodeType::CHURCH:
    case ScriptTreeNodeType::PROG_DEP_TABLE:
    case ScriptTreeNodeType::TABLE:
    case ScriptTreeNodeType::CUSTOM_ASM:
        attr.SetBold(true);
        attr.SetColour(wxColour(0, 100, 0));
        return;

    // Everything else (function headers, structural nodes) - default colour, bold.
    default:
        attr.SetBold(true);
        return;
    }
}







std::vector<ScriptTreeAddOption> ScriptTreeDataViewModel::GetChildOptions(const ScriptTreeNode& node) const
{
    // A node whose own children ARE a function body - either a standalone ScriptTreeNodeType::FUNCTION
    // (e.g. under "Other Functions") or a top-level slot merged with a resolved function's body
    // (e.g. "On Pay: Function: X") - offers the same set of statement types regardless of its
    // own ScriptTreeNodeType, since structurally its children list is a function body either way.
    if (node.type == ScriptTreeNodeType::FUNCTION || !node.embedded_function_name.empty())
    {
        return {
            { ScriptTreeNodeType::ACTION, "Script Action", "Script Action:" },
            { ScriptTreeNodeType::QUESTION, "Yes/No Question", "Yes/No Question:" },
            { ScriptTreeNodeType::SET_FLAG, "Set Flag on Talk", "Set Flag 0000 on Talk:" },
            { ScriptTreeNodeType::CHECK_FLAG, "Check Flag", "Check Flag 0000:" },
            { ScriptTreeNodeType::PROG_DEP_TABLE, "Progress Dependent List", "Progress Dependent:" },
            { ScriptTreeNodeType::SHOP, "Shop Interaction", "Shop:" },
            { ScriptTreeNodeType::CHURCH, "Church Interaction", "Church:" },
            { ScriptTreeNodeType::DISPLAY_PRICE, "Display Item Price", "Display Item Price:" },
            { ScriptTreeNodeType::TABLE, "Action Table", "Action Table:" },
            { ScriptTreeNodeType::BRANCH, "Branch", "Branch: Label" },
            { ScriptTreeNodeType::PLAY_SOUND, "Play Sound", "Play Sound: 0" },
            { ScriptTreeNodeType::CUSTOM_SHOP_ACTION, "Custom Item Script", "Custom Item Script: 0" },
            { ScriptTreeNodeType::SLEEP, "Sleep", "Sleep: 0 frames" },
            { ScriptTreeNodeType::CUSTOM_ASM, "Custom ASM Block", "Custom ASM" },
            { ScriptTreeNodeType::RETURN, "Return", "Return" }
        };
    }

    std::vector<ScriptTreeAddOption> options;
    switch (node.type)
    {
    case ScriptTreeNodeType::PROG_DEP_TABLE:
        options = { { ScriptTreeNodeType::PROG_DEP_ACTION, "Progress Entry", "On Quest 0, Progress 0:" } };
        break;
    case ScriptTreeNodeType::TABLE:
        options = { { ScriptTreeNodeType::TABLE_ENTRY, "Table Entry", "Entry:" } };
        break;
    // NOTE: deliberately no option for CUSTOM_ASM - its instruction lines are edited (added,
    // removed, reordered) wholesale through the multi-line ASM dialog, not the add menu.
    case ScriptTreeNodeType::CUSTOM_ITEM_SHOP_TABLE:
        options = { { ScriptTreeNodeType::CUSTOM_ITEM_SHOP_TABLE, "Custom Item Shop Action", "Custom Action:" } };
        break;
    default:
        break;
    }
    // Bare functions always live at the entry's top level, so only the model root offers the
    // "Function" option (reachable via Add Sibling on any top-level row). This also covers the
    // "Other Functions" entry, whose root is the model root too.
    if (&node == &root)
    {
        options.push_back({ ScriptTreeNodeType::FUNCTION, "Function", "Function: NewFunction" });
    }
    return options;
}

bool ScriptTreeDataViewModel::IsChildOptionType(const ScriptTreeNode& parent, ScriptTreeNodeType type) const
{
    for (const auto& option : GetChildOptions(parent))
    {
        if (option.type == type)
        {
            return true;
        }
    }
    return false;
}


std::string ScriptTreeDataViewModel::GetFunctionName(const ScriptTreeNode& node) const
{
    // Merged/embedded slots carry the name in embedded_function_name; FUNCTION nodes and
    // inline function displays carry it in text_value. Never parsed out of the display text.
    if (!node.embedded_function_name.empty())
    {
        return node.embedded_function_name;
    }
    return node.text_value;
}

Statements::Action ScriptTreeDataViewModel::BuildAction(const ScriptTreeNode& node) const
{
    const ScriptTreeNode* action_node = &node;
    // A Progress Dependent entry still nests its own "Script Action:" wrapper child (kept
    // separate from its Quest/Progress numbers, unlike every other slot type, which merges the
    // action directly into the slot itself) - descend into it to find the real action data.
    if (action_node->text_value.empty() && action_node->children.size() == 1
        && action_node->children.front().type == ScriptTreeNodeType::ACTION)
    {
        action_node = &action_node->children.front();
    }

    // Any kind of function display or reference - a merged/embedded body, an inline display,
    // or a bare reference by name - becomes a plain jump. The function's own definition is
    // synced separately (CollectFunctionNodes() picks the displaying node up as a definition),
    // which is what keeps the internal table flat: functions are only ever nested by the YAML
    // exporter, never here.
    const std::string function_name = GetFunctionName(*action_node);
    if (!function_name.empty())
    {
        return Statements::Action(AsmFile::ScriptAction(AsmFile::ScriptJump(function_name)));
    }

    return Statements::Action(AsmFile::ScriptAction(AsmFile::ScriptId(action_node->numeric_value)));
}

std::optional<Statements::ScriptStatement> ScriptTreeDataViewModel::BuildStatement(const ScriptTreeNode& node) const
{
    auto find_child = [&node](ScriptTreeNodeType type) -> const ScriptTreeNode*
    {
        for (const auto& child : node.children)
        {
            if (child.type == type)
            {
                return &child;
            }
        }
        return nullptr;
    };
    auto action_child = [&](ScriptTreeNodeType type) -> Statements::Action
    {
        // A missing fixed slot falls back to script ID 0, the same default a freshly added
        // statement's slots start with (see AddDefaultScriptAction()).
        const ScriptTreeNode* child = find_child(type);
        return child ? BuildAction(*child) : Statements::Action(AsmFile::ScriptAction(AsmFile::ScriptId(0)));
    };

    switch (node.type)
    {
    case ScriptTreeNodeType::ACTION:
        return BuildAction(node);
    case ScriptTreeNodeType::QUESTION:
        return Statements::YesNoPrompt(action_child(ScriptTreeNodeType::Q_PROMPT),
            action_child(ScriptTreeNodeType::Q_ON_YES), action_child(ScriptTreeNodeType::Q_ON_NO));
    case ScriptTreeNodeType::SET_FLAG:
        return Statements::SetFlagOnTalk(node.numeric_value,
            action_child(ScriptTreeNodeType::SF_ON_CLEAR), action_child(ScriptTreeNodeType::SF_ON_SET));
    case ScriptTreeNodeType::CHECK_FLAG:
        return Statements::IsFlagSet(node.numeric_value,
            action_child(ScriptTreeNodeType::CF_ON_SET), action_child(ScriptTreeNodeType::CF_ON_CLEAR));
    case ScriptTreeNodeType::PROG_DEP_TABLE:
    {
        std::vector<std::pair<Statements::ProgressList::QuestProgress, Statements::Action>> progress;
        for (const auto& child : node.children)
        {
            if (child.type == ScriptTreeNodeType::PROG_DEP_ACTION)
            {
                progress.emplace_back(
                    Statements::ProgressList::QuestProgress(
                        static_cast<uint8_t>(child.numeric_value), static_cast<uint8_t>(child.numeric_value2)),
                    BuildAction(child));
            }
        }
        return Statements::ProgressList(std::move(progress));
    }
    case ScriptTreeNodeType::SHOP:
        return Statements::ShopInteraction(action_child(ScriptTreeNodeType::SHOP_ON_SALE_PROMPT),
            action_child(ScriptTreeNodeType::SHOP_ON_SALE_CONFIRM), action_child(ScriptTreeNodeType::SHOP_ON_SALE_NO_MONEY),
            action_child(ScriptTreeNodeType::SHOP_ON_SALE_CANCEL));
    case ScriptTreeNodeType::CHURCH:
        return Statements::ChurchInteraction(action_child(ScriptTreeNodeType::CHURCH_NORMAL_PRIEST),
            action_child(ScriptTreeNodeType::CHURCH_SKELETON_PRIEST));
    case ScriptTreeNodeType::DISPLAY_PRICE:
    {
        // Multi-variant form (FR/DE article forms): the actions live in TABLE_ENTRY children.
        // Single form: the node itself is the merged action (see ScriptTreeBuilder).
        std::vector<Statements::Action> actions;
        for (const auto& child : node.children)
        {
            if (child.type == ScriptTreeNodeType::TABLE_ENTRY)
            {
                actions.push_back(BuildAction(child));
            }
        }
        if (!actions.empty())
        {
            return Statements::DisplayPrice(std::move(actions));
        }
        return Statements::DisplayPrice(BuildAction(node));
    }
    case ScriptTreeNodeType::TABLE:
    {
        std::vector<Statements::Action> actions;
        for (const auto& child : node.children)
        {
            if (child.type == ScriptTreeNodeType::TABLE_ENTRY)
            {
                actions.push_back(BuildAction(child));
            }
        }
        return Statements::ActionTable(std::move(actions));
    }
    case ScriptTreeNodeType::BRANCH:
        return Statements::Branch(node.text_value, node.bool_value);
    case ScriptTreeNodeType::PLAY_SOUND:
        return Statements::PlaySound(node.numeric_value);
    case ScriptTreeNodeType::CUSTOM_SHOP_ACTION:
        return Statements::CustomItemScript(node.numeric_value);
    case ScriptTreeNodeType::SLEEP:
        return Statements::Sleep(node.numeric_value);
    case ScriptTreeNodeType::CUSTOM_ASM:
    {
        // The lines were validated/normalized on entry (ApplyCustomAsm(), or the original
        // Instruction::ToLine() round trip at tree-build time), so they parse back cleanly.
        std::vector<AsmFile::Instruction> instructions;
        for (const auto& child : node.children)
        {
            if (child.type == ScriptTreeNodeType::CUSTOM_ASM_LINE)
            {
                instructions.push_back(AsmFile::Instruction::FromLine("\t" + child.name.ToStdString()));
            }
        }
        return Statements::CustomAsm(instructions);
    }
    case ScriptTreeNodeType::RETURN:
        return Statements::Rts();
    default:
        // Structural/display-only nodes (script listings, ASM lines, slot children) don't
        // themselves form statements.
        return std::nullopt;
    }
}

ScriptFunction ScriptTreeDataViewModel::BuildFunction(const ScriptTreeNode& node) const
{
    Statements::ScriptStatementVector statements;
    for (const auto& child : node.children)
    {
        if (auto statement = BuildStatement(child))
        {
            statements.push_back(std::move(*statement));
        }
    }
    return ScriptFunction(GetFunctionName(node), statements);
}

void ScriptTreeDataViewModel::CollectFunctionNodes(ScriptTreeNode& node, std::vector<ScriptTreeNode*>& functions) const
{
    // A node "is" a function definition to sync back to m_functions if it's a standalone
    // ScriptTreeNodeType::FUNCTION (e.g. under "Other Functions"), any node merged with a resolved
    // function's body - a top-level slot ("On Pay: Function: X") or a nested Script Action
    // displaying its target inline ("OnYes: Function: X") - or an inline (consolidated)
    // function display with no standalone entry of its own (text_value + a body, e.g. loaded
    // from nested YAML; BuildAction() references these by name, so their definitions must be
    // synced from here or they'd never reach the table). Otherwise edits made to a displayed
    // body are silently never persisted, and get discarded on the next category rebuild.
    const bool is_function_node = !node.embedded_function_name.empty()
        || (node.type == ScriptTreeNodeType::FUNCTION && (!node.parent || node.parent->type != ScriptTreeNodeType::ACTION))
        || (node.type != ScriptTreeNodeType::FUNCTION && !node.text_value.empty() && !node.children.empty()
            && node.type != ScriptTreeNodeType::BRANCH);
    if (is_function_node)
    {
        functions.push_back(&node);
    }
    for (auto& child : node.children)
    {
        CollectFunctionNodes(child, functions);
    }
}

void ScriptTreeDataViewModel::CollectFunctionNames(const ScriptTreeNode& node, std::set<std::string>& names) const
{
    // Deliberately standalone-ScriptTreeNodeType::FUNCTION-only (unlike CollectFunctionNodes below) - this
    // seeds m_known_function_names, which SyncToScriptTable() uses to decide what to auto-remove
    // when no longer displayed. A merged/embedded slot's function must never be a removal
    // candidate just because THIS entry stopped showing it, since the same function can
    // legitimately be embedded by multiple different table slots (see the "[Shared xN]" marker).
    if (node.type == ScriptTreeNodeType::FUNCTION && (!node.parent || node.parent->type != ScriptTreeNodeType::ACTION))
    {
        names.insert(GetFunctionName(node));
    }
    for (const auto& child : node.children)
    {
        CollectFunctionNames(child, names);
    }
}

void ScriptTreeDataViewModel::SyncToScriptTable()
{
    if (!m_functions)
    {
        return;
    }

    std::vector<ScriptTreeNode*> function_nodes;
    CollectFunctionNodes(root, function_nodes);
    std::set<std::string> current_names;
    std::vector<std::string> current_order;
    // Table-position anchor for brand-new functions: the last synced function that lives in
    // the primary table. Inserting there (rather than appending at the table's end) keeps a
    // new function inside this entry's location run, so the category rebuild's location-based
    // related-function attribution redisplays it under THIS entry - appended at the end it
    // would silently reappear under whichever entry owns the table's last anchored run.
    std::string insert_anchor;

    for (ScriptTreeNode* node : function_nodes)
    {
        // Built straight from the node tree - the tree IS the source of truth for what's
        // displayed, and it syncs into the flat internal table without any intermediate
        // serialization format.
        ScriptFunction built = BuildFunction(*node);
        if (built.name.empty())
        {
            continue;
        }
        // Route the edit to whichever pool table already defines this function (= its output
        // file); a function that doesn't exist yet is a brand-new one and goes into this tab's
        // own (primary) table.
        const std::string name = built.name;
        ScriptFunctionTable* owner = FindOwningTable(name);
        if (owner)
        {
            *owner->GetMapping(name) = std::move(built);
        }
        else
        {
            m_functions->InsertFunctionAfter(std::move(built), insert_anchor);
            owner = m_functions.get();
        }
        if (owner == m_functions.get())
        {
            insert_anchor = name;
        }
        // Ordering and auto-removal are only ever applied to this tab's primary table, and only
        // for standalone definitions it owns - see CollectFunctionNames() for why a merged /
        // embedded slot's function (or one owned by another pool table) must not be a candidate.
        if (owner == m_functions.get() && node->type == ScriptTreeNodeType::FUNCTION)
        {
            current_names.insert(name);
            current_order.push_back(name);
        }
    }

    for (const auto& name : m_known_function_names)
    {
        if (current_names.find(name) == current_names.end())
        {
            m_functions->RemoveFunction(name);
        }
    }
    m_functions->SetFunctionOrder(current_order);
    m_known_function_names = current_names;
}
void ScriptTreeDataViewModel::AddDefaultScriptAction(ScriptTreeNode& node) const
{
    PopulateScriptAction(node, false, "", 0);
}
void ScriptTreeDataViewModel::PopulateFixedChildren(ScriptTreeNode& node) const
{
    auto add_action_child = [this, &node](ScriptTreeNodeType type, const wxString& name)
    {
        // A labeled slot whose own label is merged with its resolved action (e.g. "OnClear: ...").
        ScriptTreeNode* child = node.AddChild(type, name);
        child->action_label = child->name;
        AddDefaultScriptAction(*child);
    };

    auto merge_action_self = [this](ScriptTreeNode& target)
    {
        // The node itself is the action (e.g. a Progress entry's nested Script Action, a Table
        // Entry, Display Price) - its existing name becomes its action_label.
        target.action_label = target.name;
        AddDefaultScriptAction(target);
    };

    switch (node.type)
    {
    case ScriptTreeNodeType::FUNCTION:
    {
        // The default display name is "Function: <name>" (see GetChildOptions()) - keep the
        // authoritative bare name in text_value, like every other FUNCTION node, and make it
        // unique (NewFunction, NewFunction2, ...): the sync that follows creates a function
        // under this name, and a duplicate would silently overwrite the existing function's
        // body with this empty one.
        const std::string prefix = "Function: ";
        const std::string display = node.name.ToStdString();
        std::string name = (display.rfind(prefix, 0) == 0) ? display.substr(prefix.size()) : display;
        if (HasFunction(name))
        {
            int suffix = 2;
            while (HasFunction(name + std::to_string(suffix)))
            {
                ++suffix;
            }
            name += std::to_string(suffix);
        }
        node.text_value = name;
        node.name = prefix + name;
        node.AddChild(ScriptTreeNodeType::RETURN, "Return");
        break;
    }
    case ScriptTreeNodeType::BRANCH:
        // The label lives in text_value (that's what BuildStatement() reads) - start with the
        // same placeholder the "Branch: Label" default display name shows, so a freshly added
        // Branch never syncs with an empty target.
        node.text_value = "Label";
        break;
    case ScriptTreeNodeType::CUSTOM_ASM:
        // A new block starts with a placeholder instruction; the multi-line ASM editor dialog
        // (opened on activation) is the only way to change the lines from here.
        node.AddChild(ScriptTreeNodeType::CUSTOM_ASM_LINE, "nop");
        break;
    case ScriptTreeNodeType::ACTION:
    case ScriptTreeNodeType::TABLE_ENTRY:
        merge_action_self(node);
        break;
    case ScriptTreeNodeType::PROG_DEP_ACTION:
        add_action_child(ScriptTreeNodeType::ACTION, "Script Action:");
        break;
    case ScriptTreeNodeType::CUSTOM_ITEM_SHOP_TABLE:
        if (node.parent && node.parent->type == ScriptTreeNodeType::CUSTOM_ITEM_SHOP_TABLE)
        {
            merge_action_self(node);
        }
        break;
    case ScriptTreeNodeType::QUESTION:
        add_action_child(ScriptTreeNodeType::Q_PROMPT, "Prompt:");
        add_action_child(ScriptTreeNodeType::Q_ON_YES, "OnYes:");
        add_action_child(ScriptTreeNodeType::Q_ON_NO, "OnNo:");
        break;
    case ScriptTreeNodeType::SET_FLAG:
        add_action_child(ScriptTreeNodeType::SF_ON_CLEAR, "OnClear:");
        add_action_child(ScriptTreeNodeType::SF_ON_SET, "OnSet:");
        break;
    case ScriptTreeNodeType::CHECK_FLAG:
        add_action_child(ScriptTreeNodeType::CF_ON_SET, "OnSet:");
        add_action_child(ScriptTreeNodeType::CF_ON_CLEAR, "OnClear:");
        break;
    case ScriptTreeNodeType::SHOP:
        add_action_child(ScriptTreeNodeType::SHOP_ON_SALE_PROMPT, "OnSalePrompt:");
        add_action_child(ScriptTreeNodeType::SHOP_ON_SALE_CONFIRM, "OnSaleConfirm:");
        add_action_child(ScriptTreeNodeType::SHOP_ON_SALE_NO_MONEY, "OnNoMoney:");
        add_action_child(ScriptTreeNodeType::SHOP_ON_SALE_CANCEL, "OnSaleDecline:");
        break;
    case ScriptTreeNodeType::CHURCH:
        add_action_child(ScriptTreeNodeType::CHURCH_NORMAL_PRIEST, "NormalPriest:");
        add_action_child(ScriptTreeNodeType::CHURCH_SKELETON_PRIEST, "SkeletonPriest:");
        break;
    case ScriptTreeNodeType::DISPLAY_PRICE:
        merge_action_self(node);
        break;
    default:
        break;
    }
}

std::vector<ScriptTreeAddOption> ScriptTreeDataViewModel::GetAddChildOptions(const wxDataViewItem& item) const
{
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    return node ? GetChildOptions(*node) : std::vector<ScriptTreeAddOption>();
}

std::vector<ScriptTreeAddOption> ScriptTreeDataViewModel::GetAddSiblingOptions(const wxDataViewItem& item) const
{
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    return (node && node->parent) ? GetChildOptions(*node->parent) : std::vector<ScriptTreeAddOption>();
}

bool ScriptTreeDataViewModel::CanAddChild(const wxDataViewItem& item) const
{
    return !GetAddChildOptions(item).empty();
}

bool ScriptTreeDataViewModel::CanAddSibling(const wxDataViewItem& item) const
{
    return !GetAddSiblingOptions(item).empty();
}

bool ScriptTreeDataViewModel::CanRemoveItem(const wxDataViewItem& item) const
{
    // Only nodes of a type the parent offers as an add option are user-managed - fixed
    // structure (slots like "On Pay:", a question's OnYes/OnNo, ...) can't be removed even
    // though its parent may offer add options for other types (e.g. "Function" at top level).
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    return node && node->parent && IsChildOptionType(*node->parent, node->type);
}

bool ScriptTreeDataViewModel::CanMoveItemUp(const wxDataViewItem& item) const
{
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    if (!node || !node->parent || !IsChildOptionType(*node->parent, node->type))
    {
        return false;
    }

    const auto& siblings = node->parent->children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it)
    {
        if (&(*it) == node)
        {
            // The neighbour must be user-managed too, so a movable row (e.g. a top-level
            // function) can't be shuffled up past the fixed slots above it.
            return it != siblings.begin() && IsChildOptionType(*node->parent, std::prev(it)->type);
        }
    }
    return false;
}

bool ScriptTreeDataViewModel::CanMoveItemDown(const wxDataViewItem& item) const
{
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(item.GetID());
    if (!node || !node->parent || !IsChildOptionType(*node->parent, node->type))
    {
        return false;
    }

    const auto& siblings = node->parent->children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it)
    {
        if (&(*it) == node)
        {
            return std::next(it) != siblings.end() && IsChildOptionType(*node->parent, std::next(it)->type);
        }
    }
    return false;
}

bool ScriptTreeDataViewModel::IsTerminatingStatement(ScriptTreeNodeType type)
{
    // Statement types whose Landstalker::Statements counterpart reports IsEndOfFunction() -
    // nothing placed after one of these in a function body can ever run.
    switch (type)
    {
    case ScriptTreeNodeType::RETURN:
    case ScriptTreeNodeType::PROG_DEP_TABLE:
    case ScriptTreeNodeType::DISPLAY_PRICE:
    case ScriptTreeNodeType::SHOP:
    case ScriptTreeNodeType::TABLE:
    case ScriptTreeNodeType::BRANCH:
        return true;
    default:
        return false;
    }
}

bool ScriptTreeDataViewModel::IsFunctionBody(const ScriptTreeNode& node)
{
    return node.type == ScriptTreeNodeType::FUNCTION || !node.embedded_function_name.empty();
}

void ScriptTreeDataViewModel::RemoveRedundantReturn(ScriptTreeNode& parent, const ScriptTreeNode& added)
{
    // A terminating statement inserted directly above a trailing "Return" replaces it - both
    // end the function, so the Return is dead weight, and a function whose statements continue
    // past a terminator can't even be written to ASM (the reader stops at the terminator,
    // leaving the trailing instructions orphaned outside any label).
    if (!IsFunctionBody(parent) || added.type == ScriptTreeNodeType::RETURN || !IsTerminatingStatement(added.type)
        || parent.children.size() < 2)
    {
        return;
    }
    auto last = std::prev(parent.children.end());
    if (last->type == ScriptTreeNodeType::RETURN && &(*std::prev(last)) == &added)
    {
        const wxDataViewItem parent_item = (&parent == &root) ? wxDataViewItem() : parent.ToItem();
        const wxDataViewItem return_item = last->ToItem();
        parent.children.erase(last);
        ItemDeleted(parent_item, return_item);
    }
}

wxDataViewItem ScriptTreeDataViewModel::AddChild(const wxDataViewItem& item, const ScriptTreeAddOption& option)
{
    ScriptTreeNode* parent = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!parent || !CanAddChild(item))
    {
        return wxDataViewItem();
    }

    auto& siblings = parent->children;
    // A new statement goes above a trailing terminator (Return, Branch, Shop, ...), not after
    // it, where it would be unreachable - and unwritable as ASM (the reader stops a function
    // at its terminator). A terminating statement is only hoisted above a trailing Return
    // (which RemoveRedundantReturn() then removes); stacking it above another terminator
    // wouldn't make the body any more valid.
    auto insert_pos = siblings.end();
    if (IsFunctionBody(*parent) && option.type != ScriptTreeNodeType::RETURN
        && !siblings.empty() && IsTerminatingStatement(siblings.back().type)
        && (!IsTerminatingStatement(option.type) || siblings.back().type == ScriptTreeNodeType::RETURN))
    {
        insert_pos = std::prev(siblings.end());
    }
    // Bare functions stay grouped at the bottom of the entry - anything else added at the top
    // level goes above the first function header.
    if (parent == &root && option.type != ScriptTreeNodeType::FUNCTION)
    {
        insert_pos = std::find_if(siblings.begin(), siblings.end(),
            [](const ScriptTreeNode& n) { return n.type == ScriptTreeNodeType::FUNCTION; });
    }
    auto inserted = siblings.insert(insert_pos, ScriptTreeNode{ option.type, option.default_name });
    ScriptTreeNode* child = &*inserted;
    PopulateFixedChildren(*child);
    child->SetParent(parent);

    RemoveRedundantReturn(*parent, *child);

    wxDataViewItem child_item = child->ToItem();
    ItemAdded(parent->ToItem(), child_item);
    SyncToScriptTable();
    return child_item;
}

wxDataViewItem ScriptTreeDataViewModel::AddSibling(const wxDataViewItem& item, const ScriptTreeAddOption& option)
{
    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    if (!node || !node->parent || !CanAddSibling(item))
    {
        return wxDataViewItem();
    }

    ScriptTreeNode* parent = node->parent;
    auto& siblings = parent->children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it)
    {
        if (&(*it) == node)
        {
            ScriptTreeNode sibling{ option.type, option.default_name };
            PopulateFixedChildren(sibling);
            // "Add Sibling" on a function's trailing terminator inserts above it rather than
            // after, where the new statement would be unreachable (see AddChild()).
            auto insert_pos = std::next(it);
            if (IsFunctionBody(*parent) && option.type != ScriptTreeNodeType::RETURN
                && std::next(it) == siblings.end() && IsTerminatingStatement(node->type)
                && (!IsTerminatingStatement(option.type) || node->type == ScriptTreeNodeType::RETURN))
            {
                insert_pos = it;
            }
            // Bare functions stay grouped at the bottom of the entry, below the fixed slots.
            if (parent == &root)
            {
                if (option.type == ScriptTreeNodeType::FUNCTION)
                {
                    insert_pos = siblings.end();
                }
                else if (node->type == ScriptTreeNodeType::FUNCTION)
                {
                    insert_pos = std::find_if(siblings.begin(), siblings.end(),
                        [](const ScriptTreeNode& n) { return n.type == ScriptTreeNodeType::FUNCTION; });
                }
            }
            auto inserted = siblings.insert(insert_pos, sibling);
            inserted->SetParent(parent);

            RemoveRedundantReturn(*parent, *inserted);

            wxDataViewItem parent_item = (parent == &root) ? wxDataViewItem() : parent->ToItem();
            wxDataViewItem child_item = inserted->ToItem();
            ItemAdded(parent_item, child_item);
            SyncToScriptTable();
            return child_item;
        }
    }
    return wxDataViewItem();
}

bool ScriptTreeDataViewModel::RemoveItem(const wxDataViewItem& item)
{
    if (!CanRemoveItem(item))
    {
        return false;
    }

    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    ScriptTreeNode* parent = node->parent;
    wxDataViewItem parent_item = (parent == &root) ? wxDataViewItem() : parent->ToItem();
    wxDataViewItem deleted_item = item;

    auto& siblings = parent->children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it)
    {
        if (&(*it) == node)
        {
            siblings.erase(it);
            ItemDeleted(parent_item, deleted_item);
            SyncToScriptTable();
            return true;
        }
    }
    return false;
}

wxDataViewItem ScriptTreeDataViewModel::MoveItemUp(const wxDataViewItem& item)
{
    if (!CanMoveItemUp(item))
    {
        return wxDataViewItem();
    }

    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    auto& siblings = node->parent->children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it)
    {
        if (&(*it) == node)
        {
            siblings.splice(std::prev(it), siblings, it);
            Cleared();
            SyncToScriptTable();
            return node->ToItem();
        }
    }
    return wxDataViewItem();
}

wxDataViewItem ScriptTreeDataViewModel::MoveItemDown(const wxDataViewItem& item)
{
    if (!CanMoveItemDown(item))
    {
        return wxDataViewItem();
    }

    ScriptTreeNode* node = reinterpret_cast<ScriptTreeNode*>(item.GetID());
    auto& siblings = node->parent->children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it)
    {
        if (&(*it) == node)
        {
            auto next = std::next(it);
            siblings.splice(std::next(next), siblings, it);
            Cleared();
            SyncToScriptTable();
            return node->ToItem();
        }
    }
    return wxDataViewItem();
}

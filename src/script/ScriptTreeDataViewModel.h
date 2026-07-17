#ifndef _SCRIPT_TREE_DATA_VIEW_MODEL_H_
#define _SCRIPT_TREE_DATA_VIEW_MODEL_H_

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <wx/dataview.h>

#include <landstalker/main/GameData.h>
#include <landstalker/script/ScriptFunction.h>
#include <landstalker/script/ScriptFunctionTable.h>
#include <landstalker/script/ScriptStatements.h>

#include <script/ScriptTreeNode.h>

// What a hyperlinked script preview row points at (see GetScriptEntryLinkTarget()).
enum class ScriptTreeLinkType
{
    CUTSCENE,
    CHARACTER
};

// A true tree-shaped wxDataViewModel over one script category (Shops / Custom Items /
// Characters / Cutscenes). Unlike the other editors' BaseDataViewModel (a virtual list),
// rows nest: table entries hold actions, actions hold function bodies, bodies hold
// statements. Edits sync straight back into the underlying ScriptFunctionTable(s).
class ScriptTreeDataViewModel : public wxDataViewModel
{
public:
    // `functions` is the category's own (primary) table: new functions are created here, its
    // unreferenced functions are listed under "Other Functions", and it is the table this
    // category's function order is applied to. `pool` lists every table that shares the function
    // namespace (always including `functions`) - all four script targets share one namespace, so
    // each category resolves/validates/syncs against the union.
    ScriptTreeDataViewModel(const ScriptTreeNode& tree_root,
                            std::shared_ptr<Landstalker::ScriptFunctionTable> functions,
                            std::shared_ptr<Landstalker::GameData> gd,
                            std::vector<std::shared_ptr<Landstalker::ScriptFunctionTable>> pool = {})
        : wxDataViewModel(),
          m_functions(functions),
          m_pool(std::move(pool)),
          m_gd(gd),
          root(tree_root)
    {
        if (m_pool.empty())
        {
            m_pool.push_back(m_functions);
        }
        root.SetParent();
        CollectFunctionNames(root, m_known_function_names);
    }

    virtual ~ScriptTreeDataViewModel() {}

    virtual void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
    virtual bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
    virtual bool IsEnabled(const wxDataViewItem& item, unsigned int col) const override;
    virtual wxDataViewItem GetParent(const wxDataViewItem& item) const override;
    virtual bool IsContainer(const wxDataViewItem& item) const override;
    virtual unsigned int GetChildren(const wxDataViewItem& parent, wxDataViewItemArray& array) const override;
    // Per-row colour/font styling, derived from the node's type (the renderer itself only ever
    // sees the display text, so classification has to happen here, where the node is available).
    virtual bool GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const override;
    wxDataViewItem AddChild(const wxDataViewItem& item, const ScriptTreeAddOption& option);
    wxDataViewItem AddSibling(const wxDataViewItem& item, const ScriptTreeAddOption& option);
    std::vector<ScriptTreeAddOption> GetAddChildOptions(const wxDataViewItem& item) const;
    std::vector<ScriptTreeAddOption> GetAddSiblingOptions(const wxDataViewItem& item) const;
    bool RemoveItem(const wxDataViewItem& item);
    wxDataViewItem MoveItemUp(const wxDataViewItem& item);
    wxDataViewItem MoveItemDown(const wxDataViewItem& item);
    bool CanAddChild(const wxDataViewItem& item) const;
    bool CanAddSibling(const wxDataViewItem& item) const;
    bool CanRemoveItem(const wxDataViewItem& item) const;
    bool CanMoveItemUp(const wxDataViewItem& item) const;
    bool CanMoveItemDown(const wxDataViewItem& item) const;
    bool HasFunction(const std::string& name) const;
    bool IsValidScriptId(uint16_t id) const;
    wxString GetScriptIdPreview(uint16_t id) const;
    wxString GetFunctionPreview(const std::string& name) const;
    std::vector<wxString> GetFunctionNameChoices() const;
    bool ApplyScriptAction(const wxDataViewItem& item, bool is_function, const std::string& function_name, uint16_t script_id);
    // Applies a new value to a numeric-parameter node (Play Sound, Set/Check Flag, Sleep,
    // Custom Item Script), rebuilding its display text to match.
    bool ApplyNumericValue(const wxDataViewItem& item, uint16_t value);
    // Applies an edited label to a Branch node. The text may carry the display convention's
    // trailing " W" to mark the branch as wide.
    bool ApplyBranchLabel(const wxDataViewItem& item, const std::string& text);
    // Applies edited quest/progress values to a Progress Entry node.
    bool ApplyQuestProgress(const wxDataViewItem& item, uint16_t quest, uint16_t progress);
    // Custom ASM blocks are edited as a whole (multi-line dialog) rather than in place.
    // Resolves the ASM block an activation refers to: the item itself for a "Custom ASM" node,
    // its parent block for one of the individual instruction lines, otherwise an invalid item.
    wxDataViewItem GetCustomAsmBlock(const wxDataViewItem& item) const;
    wxString GetCustomAsmText(const wxDataViewItem& item) const;
    // Replaces the block's instructions from multi-line text; fails (without changing anything)
    // if any non-blank line doesn't parse as a single instruction, or no instructions remain.
    bool ApplyCustomAsm(const wxDataViewItem& item, const wxString& text);
    // True if the given subtree contains anything that feeds the function-reference analysis
    // (a function display/definition, branch, custom ASM, or named action reference). Used by
    // the editor control to decide whether a removal needs the post-edit category rebuild.
    bool SubtreeAffectsReferences(const wxDataViewItem& item) const;
    // True when the item sits inside (or is) a top-level standalone "Function:" display - the
    // one kind of subtree whose content is never shown anywhere else in the category, so
    // value-only edits within it don't need the post-edit category rebuild (which would also
    // relocate a still-unreferenced, freshly created function to "Other Functions").
    bool IsInsideStandaloneFunction(const wxDataViewItem& item) const;
    // Syntax-only check for a free-typed ASM label (function name or branch target): 1-50
    // chars, "_"/A-Z/a-z/0-9 only, can't start with a digit. Anything looser would flow
    // verbatim into the generated ASM output.
    static bool IsValidLabelSyntax(const std::string& name);
    // IsValidLabelSyntax(), plus must not already name an existing function in this table.
    bool IsValidNewFunctionName(const std::string& name) const;
    // A SCRIPT_ENTRY preview row whose script line references a cutscene or a non-global
    // character with an in-range script-table entry links to that entry's own script tree:
    // returns the target (category + entry index), or nullopt for every other row. Drawn as a
    // hyperlink via GetAttr(); double-click opens the target's tree popup.
    std::optional<std::pair<ScriptTreeLinkType, int>> GetScriptEntryLinkTarget(const ScriptTreeNode& node) const;
    bool IsScriptEntryLink(const ScriptTreeNode& node) const;
private:
    // True for statement types that end a function (their Statements counterpart reports
    // IsEndOfFunction()) - nothing placed after one of these can ever run.
    static bool IsTerminatingStatement(ScriptTreeNodeType type);
    // True if the node's children form a function body (a standalone FUNCTION node or any node
    // displaying an embedded function).
    static bool IsFunctionBody(const ScriptTreeNode& node);
    // Removes the placeholder "Return" from a function body that consists of exactly the
    // just-added terminating statement plus that Return, notifying the view.
    void RemoveRedundantReturn(ScriptTreeNode& parent, const ScriptTreeNode& added);
    std::vector<ScriptTreeAddOption> GetChildOptions(const ScriptTreeNode& node) const;
    void PopulateFixedChildren(ScriptTreeNode& node) const;
    void AddDefaultScriptAction(ScriptTreeNode& node) const;
    void PopulateScriptAction(ScriptTreeNode& node, bool is_function, const std::string& function_name, uint16_t script_id) const;
    void SyncToScriptTable();
    // Called when a table-slot edit makes `name` this entry's location anchor: moves its
    // display node (and, via the sync that follows, its table position) to the head of the
    // contiguous group of standalone functions it sits in. The rebuild's location walk only
    // runs FORWARD from an anchor, so any of the entry's other functions positioned before the
    // anchor in the table would otherwise be re-attributed to the preceding entry's run.
    void MoveAnchoredFunctionToGroupHead(const std::string& name);
    // Adds a new, empty (single "Return" statement) function to m_functions - the caller must
    // have already verified IsValidNewFunctionName().
    void CreateFunction(const std::string& name);
    // Renames the bare function displayed by the given item, updating m_functions (including
    // jump/branch references) and the owning category's table slots. The display refresh is left
    // to the category rebuild that follows every edit.
    bool RenameDisplayedFunction(const wxDataViewItem& item, const std::string& new_name);
    void RenameTableReferences(const std::string& old_name, const std::string& new_name);
    // True if the parent offers an add option of the given type - i.e. children of that type
    // are user-managed (addable, removable, movable) rather than fixed structure.
    bool IsChildOptionType(const ScriptTreeNode& parent, ScriptTreeNodeType type) const;
    // True for one of a Custom Item Shop Action container's fixed "On Pick Up:"/"On Pay:"/
    // "On Steal:" leading entries (by position, not label) - these can't be removed or reordered,
    // unlike the numbered "Custom Action N" entries after them.
    bool IsFixedShopActionSlot(const ScriptTreeNode& node) const;
    // The ScriptTable::Action a Custom Item Shop Action entry's merged value represents - the
    // ScriptTable::Action counterpart of BuildAction() (which builds a Statements::Action for
    // function bodies instead; these entries live in a ScriptTable::Item, not a function).
    Landstalker::ScriptTable::Action BuildShopTableAction(const ScriptTreeNode& node) const;
    // Rebuilds the whole ScriptTable::Item::actions vector from a Custom Item Shop Action
    // container's current children (in order) and writes it back via write_back_actions - needed
    // after any structural change (add/remove/move) to that container's children, since each
    // entry's own write_back only ever overwrites its own already-existing table position.
    void SyncCustomItemShopActions(ScriptTreeNode& container) const;
    // Re-labels a Custom Item Shop Action container's numbered entries (position 4+) to match
    // their current position, preserving each entry's own resolved action ("body") - called after
    // any add/remove/move so labels never drift out of sequence. The fixed first three entries
    // ("On Pick Up:"/"On Pay:"/"On Steal:") are left untouched.
    void RenumberCustomShopActions(ScriptTreeNode& container);
    void CollectFunctionNodes(ScriptTreeNode& node, std::vector<ScriptTreeNode*>& functions) const;
    void CollectFunctionNames(const ScriptTreeNode& node, std::set<std::string>& names) const;
    // Direct node -> Statements construction, the exact inverse of AddFuncBody()/AddAction().
    // The tree syncs to the underlying ScriptFunctionTable through these; YAML is a separate
    // output format (export/import), never part of the edit path.
    Landstalker::Statements::Action BuildAction(const ScriptTreeNode& node) const;
    std::optional<Landstalker::Statements::ScriptStatement> BuildStatement(const ScriptTreeNode& node) const;
    Landstalker::ScriptFunction BuildFunction(const ScriptTreeNode& node) const;
    // True when the row's value resolves (known function target / in-range script ID) - drawn
    // red via GetAttr() otherwise.
    bool IsNodeValueValid(const ScriptTreeNode& node) const;
    // The colour/font classification part of GetAttr() (by node type/position only).
    void ApplyClassAttr(const ScriptTreeNode& node, wxDataViewItemAttr& attr) const;
    std::string GetFunctionName(const ScriptTreeNode& node) const;
    // The pool table that currently defines `name`, or nullptr if none does. Used to route a
    // function's edits/rename back to its owning table (= its output ASM file).
    Landstalker::ScriptFunctionTable* FindOwningTable(const std::string& name) const;
    // The pool table whose functions this category owns for creation/removal/ordering (== m_functions).
    std::shared_ptr<Landstalker::ScriptFunctionTable> m_functions;
    // Every table sharing the function namespace, always including m_functions (see ctor).
    std::vector<std::shared_ptr<Landstalker::ScriptFunctionTable>> m_pool;
    std::shared_ptr<Landstalker::GameData> m_gd;
    std::set<std::string> m_known_function_names;
    ScriptTreeNode root;
};

#endif // _SCRIPT_TREE_DATA_VIEW_MODEL_H_

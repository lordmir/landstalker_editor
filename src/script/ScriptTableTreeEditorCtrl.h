#ifndef _SCRIPT_TABLE_TREE_EDITOR_CTRL_H_
#define _SCRIPT_TABLE_TREE_EDITOR_CTRL_H_

#include <memory>
#include <vector>

#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/listbox.h>
#include <wx/splitter.h>

#include <landstalker/main/GameData.h>
#include <main/ImageList.h>
#include <script/ScriptTreeNode.h>
#include <script/ScriptTreeDataViewModel.h>

// One of the four script targets. Values match ScriptTreeNode::BuildCategoryTree()'s
// category indices.
enum class ScriptTableTreeCategory
{
    SHOP = 0,
    ITEM = 1,
    CHARACTER = 2,
    CUTSCENE = 3
};

// Tree-based editor for one script target: an entry list (the target's script table rows)
// alongside a fully expanded statement tree for the selected entry. Edits sync straight
// back into the shared function pool / script tables; "Other Functions" (the target's
// unreferenced functions) is always synthesised as the last entry.
class ScriptTableTreeEditorCtrl : public wxPanel
{
public:
    // `show_entry_list` false drops the left (entry list) pane entirely, leaving just the tree
    // and its edit buttons - the popup (ScriptTableTreeEditorDialog) hosts one fixed entry, so
    // the list is dead weight there. With the list hidden the entry buttons are never built, so
    // `imglst` may be nullptr in that mode.
    ScriptTableTreeEditorCtrl(wxWindow* parent, ImageList* imglst, bool show_entry_list = true);
    virtual ~ScriptTableTreeEditorCtrl();

    virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
    virtual void ClearGameData();

    // `entry` >= 0 opens with that specific entry selected instead of the default (first, or the
    // previously selected entry when re-opening the same category).
    void Open(ScriptTableTreeCategory category, int entry = -1);
    ScriptTableTreeCategory GetCategory() const { return m_category; }
    int GetSelectedEntry() const;

    // Tree-item operations (used by both the panel buttons and the frame's menu/toolbar).
    void AddChildItem();
    void AddSiblingItem();
    void RemoveSelectedItem();
    void MoveSelectedItemUp();
    void MoveSelectedItemDown();
    bool CanAddChild() const;
    bool CanAddSibling() const;
    bool CanRemoveItem() const;
    bool CanMoveItemUp() const;
    bool CanMoveItemDown() const;

    // Entry (table row) operations.
    void AddEntry();
    void RemoveEntry();
    void MoveEntryUp();
    void MoveEntryDown();
    bool CanRemoveEntry() const;
    bool CanMoveEntryUp() const;
    bool CanMoveEntryDown() const;

    // Function-pool import/export. `yaml` selects YAML, otherwise raw ASM.
    void ExportScript(bool yaml);
    void ImportScript(bool yaml);

    // Character/cutscene table import/export. Their ASM form preserves Action variants as
    // ScriptID/ScriptJump directives; shop and custom-item tables are not exposed here.
    void ExportTableYaml();
    void ImportTableYaml();
    void ExportTableAsm();
    void ImportTableAsm();

    // Commits any in-place edit currently open on the tree column, pushing its value through
    // the model (and so into the underlying script tables). The floating editor only commits
    // itself when the tree decides editing has finished; menu and toolbar clicks don't move
    // focus, so anything that snapshots the tables (project save, export) must flush first.
    void CommitTreeEditing();

    // Refreshes the current category's tree/entry list after its underlying table was mutated
    // (also called internally after every reference-affecting edit).
    void RebuildCategory(int select_entry);

    // Invoked whenever the selection or edit-capability state changes - the owning frame uses
    // it to refresh its toolbar enable states and the properties pane.
    void SetStateChangeCallback(std::function<void()> callback) { m_on_state_change = std::move(callback); }

private:
    void BuildUi();
    // Opens the popup a double-clicked script preview row (SCRIPT_ENTRY) leads to: the linked
    // cutscene/character's own script tree for cutscene/speaker entries, otherwise the segment
    // script editor for the entry's script line. Rebuilds the category afterwards - both popups
    // edit shared data this tree displays.
    void OpenScriptEntryPopup(const ScriptTreeNode& node);
    void ShowAddMenu(bool child);
    void AddSelectedItem(bool child, const ScriptTreeAddOption& option);
    void MoveCurrentEntry(int direction);
    void SelectScriptEntry(int entry_index);
    void RebuildCurrentCategory();
    bool SetScriptTable(Landstalker::ScriptFunctionTable functions);
    void OnTreeSelectionChanged(wxDataViewEvent& event);
    void OnTreeItemActivated(wxDataViewEvent& event);
    void OnTreeItemEditingDone(wxDataViewEvent& event);
    // Cancels (without committing) any in-place edit currently active on the tree column.
    // Only for paths where the edit's target data is going away entirely (project close,
    // whole-table import) - navigation and save paths use CommitTreeEditing() instead.
    void CancelTreeEditing();
    void ExpandTree(const wxDataViewItem& parent = wxDataViewItem());
    void UpdateEditButtons();
    ScriptTreeDataViewModel* GetTreeModel() const;
    // The category's own function table (new functions are created here; it is written back to
    // this target's ASM file) and the shared four-table resolution pool.
    std::shared_ptr<Landstalker::ScriptFunctionTable> GetCategoryFunctions() const;
    std::vector<std::shared_ptr<Landstalker::ScriptFunctionTable>> GetFunctionPool() const;
    wxString GetCategoryName() const;
    wxString GetFunctionYamlFilename() const;
    wxString GetTableYamlFilename() const;
    wxString GetTableAsmFilename() const;

    std::shared_ptr<Landstalker::GameData> m_gd;
    ScriptTableTreeCategory m_category = ScriptTableTreeCategory::SHOP;
    bool m_open = false;

    // Built per Open()/rebuild and kept alive while displayed: m_entries holds raw pointers
    // into this tree, so it must outlive them.
    ScriptTreeNode m_category_root;
    std::vector<ScriptTreeNode*> m_entries;
    // Lazily populated, one slot per entry; created on first selection and then kept alive
    // until the next rebuild so revisiting an entry doesn't lose in-progress structural edits.
    std::vector<wxObjectDataPtr<ScriptTreeDataViewModel>> m_models;

    wxListBox* m_entry_list = nullptr;
    wxDataViewCtrl* m_dvc_ctrl = nullptr;
    std::function<void()> m_on_state_change;
    ImageList* m_imglst = nullptr;
    bool m_show_entry_list = true;
    // The current entry when there's no entry list to hold the selection (popup mode).
    int m_selected_entry = 0;
};

#endif // _SCRIPT_TABLE_TREE_EDITOR_CTRL_H_

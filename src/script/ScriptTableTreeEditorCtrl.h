#ifndef _SCRIPT_TABLE_TREE_EDITOR_CTRL_H_
#define _SCRIPT_TABLE_TREE_EDITOR_CTRL_H_

#include <memory>
#include <vector>

#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/listbox.h>
#include <wx/splitter.h>

#include <landstalker/main/GameData.h>
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
    ScriptTableTreeEditorCtrl(wxWindow* parent);
    virtual ~ScriptTableTreeEditorCtrl();

    virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
    virtual void ClearGameData();

    void Open(ScriptTableTreeCategory category);
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

    // Whole-target import/export. `yaml` selects YAML, otherwise raw ASM.
    void ExportScript(bool yaml);
    void ImportScript(bool yaml);

    // Refreshes the current category's tree/entry list after its underlying table was mutated
    // (also called internally after every reference-affecting edit).
    void RebuildCategory(int select_entry);

    // Invoked whenever the selection or edit-capability state changes - the owning frame uses
    // it to refresh its toolbar enable states and the properties pane.
    void SetStateChangeCallback(std::function<void()> callback) { m_on_state_change = std::move(callback); }

private:
    void BuildUi();
    void ShowAddMenu(bool child);
    void AddSelectedItem(bool child, const ScriptTreeAddOption& option);
    void MoveCurrentEntry(int direction);
    void SelectScriptEntry(int entry_index);
    void RebuildCurrentCategory();
    bool SetScriptTable(Landstalker::ScriptFunctionTable functions);
    void OnTreeSelectionChanged(wxDataViewEvent& event);
    void OnTreeItemActivated(wxDataViewEvent& event);
    void OnTreeItemEditingDone(wxDataViewEvent& event);
    // Cancels (without committing) any in-place edit currently active on the tree column, so
    // that switching entries or categories while editing doesn't leave the floating editor
    // control stuck open over what's now a different row/entry.
    void CancelTreeEditing();
    void ExpandTree(const wxDataViewItem& parent = wxDataViewItem());
    void UpdateEditButtons();
    ScriptTreeDataViewModel* GetTreeModel() const;
    // The category's own function table (new functions are created here; it is written back to
    // this target's ASM file) and the shared four-table resolution pool.
    std::shared_ptr<Landstalker::ScriptFunctionTable> GetCategoryFunctions() const;
    std::vector<std::shared_ptr<Landstalker::ScriptFunctionTable>> GetFunctionPool() const;
    wxString GetCategoryName() const;

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
};

#endif // _SCRIPT_TABLE_TREE_EDITOR_CTRL_H_

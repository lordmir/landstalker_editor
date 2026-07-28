#include <script/ScriptTableTreeEditorCtrl.h>

#include <misc/MovableModalDialog.h>
#include <misc/DataViewModelAssociate.h>

#include <script/ScriptTableTreeEditorDialog.h>
#include <script/ScriptTreeActionEditors.h>

#include <landstalker/main/AsmFile.h>

#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace Landstalker;

namespace
{
    constexpr int ID_ADD_SCRIPT_CHILD = wxID_HIGHEST + 5;
    constexpr int ID_ADD_SCRIPT_ITEM = wxID_HIGHEST + 6;
    constexpr int ID_REMOVE_SCRIPT_ITEM = wxID_HIGHEST + 7;
    constexpr int ID_MOVE_SCRIPT_ITEM_UP = wxID_HIGHEST + 8;
    constexpr int ID_MOVE_SCRIPT_ITEM_DOWN = wxID_HIGHEST + 9;
    constexpr int ID_ADD_ENTRY = wxID_HIGHEST + 10;
    constexpr int ID_REMOVE_ENTRY = wxID_HIGHEST + 11;
    constexpr int ID_MOVE_ENTRY_UP = wxID_HIGHEST + 12;
    constexpr int ID_MOVE_ENTRY_DOWN = wxID_HIGHEST + 13;
    constexpr int ID_ADD_MENU_BASE = wxID_HIGHEST + 100;

    template <typename T>
    std::size_t AddEntryGeneric(std::shared_ptr<std::vector<T>> table, T default_value)
    {
        std::size_t index = table->size();
        table->push_back(std::move(default_value));
        return index;
    }

    template <typename T>
    bool RemoveEntryGeneric(std::shared_ptr<std::vector<T>> table, std::size_t index)
    {
        if (index >= table->size())
        {
            return false;
        }
        table->erase(table->begin() + index);
        return true;
    }

    template <typename T>
    bool MoveEntryGeneric(std::shared_ptr<std::vector<T>> table, std::size_t index, int direction)
    {
        const int other = static_cast<int>(index) + direction;
        if (other < 0 || static_cast<std::size_t>(other) >= table->size())
        {
            return false;
        }
        std::iter_swap(table->begin() + index, table->begin() + other);
        return true;
    }

    int AddTableEntry(std::shared_ptr<GameData> gd, ScriptTableTreeCategory category)
    {
        switch (category)
        {
        case ScriptTableTreeCategory::SHOP:
            return static_cast<int>(AddEntryGeneric(gd->GetScriptData()->GetShopTable(), ScriptTable::Shop{}));
        case ScriptTableTreeCategory::ITEM:
        {
            ScriptTable::Item item;
            item.actions.assign(3, ScriptTable::Action(uint16_t(0)));
            return static_cast<int>(AddEntryGeneric(gd->GetScriptData()->GetItemTable(), item));
        }
        case ScriptTableTreeCategory::CHARACTER:
            return static_cast<int>(AddEntryGeneric(gd->GetScriptData()->GetCharTable(), ScriptTable::Action(uint16_t(0))));
        case ScriptTableTreeCategory::CUTSCENE:
            return static_cast<int>(AddEntryGeneric(gd->GetScriptData()->GetCutsceneTable(), ScriptTable::Action(uint16_t(0))));
        default:
            return -1;
        }
    }

    bool RemoveTableEntry(std::shared_ptr<GameData> gd, ScriptTableTreeCategory category, std::size_t index)
    {
        switch (category)
        {
        case ScriptTableTreeCategory::SHOP: return RemoveEntryGeneric(gd->GetScriptData()->GetShopTable(), index);
        case ScriptTableTreeCategory::ITEM: return RemoveEntryGeneric(gd->GetScriptData()->GetItemTable(), index);
        case ScriptTableTreeCategory::CHARACTER: return RemoveEntryGeneric(gd->GetScriptData()->GetCharTable(), index);
        case ScriptTableTreeCategory::CUTSCENE: return RemoveEntryGeneric(gd->GetScriptData()->GetCutsceneTable(), index);
        default: return false;
        }
    }

    bool MoveTableEntry(std::shared_ptr<GameData> gd, ScriptTableTreeCategory category, std::size_t index, int direction)
    {
        switch (category)
        {
        case ScriptTableTreeCategory::SHOP: return MoveEntryGeneric(gd->GetScriptData()->GetShopTable(), index, direction);
        case ScriptTableTreeCategory::ITEM: return MoveEntryGeneric(gd->GetScriptData()->GetItemTable(), index, direction);
        case ScriptTableTreeCategory::CHARACTER: return MoveEntryGeneric(gd->GetScriptData()->GetCharTable(), index, direction);
        case ScriptTableTreeCategory::CUTSCENE: return MoveEntryGeneric(gd->GetScriptData()->GetCutsceneTable(), index, direction);
        default: return false;
        }
    }
}

ScriptTableTreeEditorCtrl::ScriptTableTreeEditorCtrl(wxWindow* parent, ImageList* imglst, bool show_entry_list)
    : wxPanel(parent),
      m_imglst(imglst),
      m_show_entry_list(show_entry_list)
{
    BuildUi();
}

ScriptTableTreeEditorCtrl::~ScriptTableTreeEditorCtrl()
{
}

void ScriptTableTreeEditorCtrl::BuildUi()
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

    wxSplitterWindow* splitter = nullptr;
    wxPanel* left_panel = nullptr;
    if (m_show_entry_list)
    {
        splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH);
        splitter->SetMinimumPaneSize(150);
        // Keep the entry list's width fixed and give any extra space from resizing to the tree
        // pane instead - the more natural behaviour for a sidebar-style list.
        splitter->SetSashGravity(0.0);

        left_panel = new wxPanel(splitter, wxID_ANY);
        wxBoxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);

        // Image-only (not text) buttons here specifically - the entry list pane is narrow, and these
        // mirror the icons the old frame-level toolbar used for the same actions.
        wxBoxSizer* entry_button_sizer = new wxBoxSizer(wxHORIZONTAL);
        wxBitmapButton* add_entry_button = new wxBitmapButton(left_panel, ID_ADD_ENTRY, m_imglst->GetImage("append_tile"));
        wxBitmapButton* remove_entry_button = new wxBitmapButton(left_panel, ID_REMOVE_ENTRY, m_imglst->GetImage("delete"));
        wxBitmapButton* move_entry_up_button = new wxBitmapButton(left_panel, ID_MOVE_ENTRY_UP, m_imglst->GetImage("up"));
        wxBitmapButton* move_entry_down_button = new wxBitmapButton(left_panel, ID_MOVE_ENTRY_DOWN, m_imglst->GetImage("down"));
        add_entry_button->SetToolTip("Add Entry");
        remove_entry_button->SetToolTip("Remove Entry");
        move_entry_up_button->SetToolTip("Move Entry Up");
        move_entry_down_button->SetToolTip("Move Entry Down");
        add_entry_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { AddEntry(); });
        remove_entry_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { RemoveEntry(); });
        move_entry_up_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveEntryUp(); });
        move_entry_down_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveEntryDown(); });
        entry_button_sizer->Add(add_entry_button, 0, wxRIGHT, 4);
        entry_button_sizer->Add(remove_entry_button, 0, wxRIGHT, 4);
        entry_button_sizer->Add(move_entry_up_button, 0, wxRIGHT, 4);
        entry_button_sizer->Add(move_entry_down_button, 0);

        m_entry_list = new wxListBox(left_panel, wxID_ANY);
        m_entry_list->Bind(wxEVT_LISTBOX, [this](wxCommandEvent& event)
        {
            SelectScriptEntry(event.GetSelection());
        });

        left_sizer->Add(entry_button_sizer, 0, wxALL, 4);
        left_sizer->Add(m_entry_list, 1, wxGROW);
        left_panel->SetSizer(left_sizer);
    }

    wxPanel* right_panel = new wxPanel(splitter ? static_cast<wxWindow*>(splitter) : this, wxID_ANY);
    wxBoxSizer* right_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* edit_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* add_child_button = new wxButton(right_panel, ID_ADD_SCRIPT_CHILD, "Add Child");
    wxButton* add_button = new wxButton(right_panel, ID_ADD_SCRIPT_ITEM, "Add Sibling");
    wxButton* remove_button = new wxButton(right_panel, ID_REMOVE_SCRIPT_ITEM, "Remove");
    wxButton* move_up_button = new wxButton(right_panel, ID_MOVE_SCRIPT_ITEM_UP, "Move Up");
    wxButton* move_down_button = new wxButton(right_panel, ID_MOVE_SCRIPT_ITEM_DOWN, "Move Down");
    add_child_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { AddChildItem(); });
    add_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { AddSiblingItem(); });
    remove_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { RemoveSelectedItem(); });
    move_up_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveSelectedItemUp(); });
    move_down_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveSelectedItemDown(); });
    add_child_button->Disable();
    add_button->Disable();
    remove_button->Disable();
    move_up_button->Disable();
    move_down_button->Disable();
    edit_sizer->Add(add_child_button, 0, wxRIGHT, 4);
    edit_sizer->Add(add_button, 0, wxRIGHT, 4);
    edit_sizer->Add(remove_button, 0, wxRIGHT, 4);
    edit_sizer->Add(move_up_button, 0, wxRIGHT, 4);
    edit_sizer->Add(move_down_button, 0);

    m_dvc_ctrl = new wxDataViewCtrl(right_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE | wxDV_NO_HEADER);
    m_dvc_ctrl->AppendColumn(new wxDataViewColumn("Script", new ScriptActionRenderer(), 0, 500, wxALIGN_LEFT));
    m_dvc_ctrl->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ScriptTableTreeEditorCtrl::OnTreeSelectionChanged, this);
    m_dvc_ctrl->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ScriptTableTreeEditorCtrl::OnTreeItemActivated, this);
    m_dvc_ctrl->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &ScriptTableTreeEditorCtrl::OnTreeItemEditingDone, this);

    right_sizer->Add(edit_sizer, 0, wxALL, 4);
    right_sizer->Add(m_dvc_ctrl, 1, wxGROW);
    right_panel->SetSizer(right_sizer);

    if (splitter)
    {
        splitter->SplitVertically(left_panel, right_panel, 340);
        sizer->Add(splitter, 1, wxGROW);
    }
    else
    {
        sizer->Add(right_panel, 1, wxGROW);
    }
    SetSizer(sizer);
}

void ScriptTableTreeEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
    m_gd = gd;
    m_open = false;
}

void ScriptTableTreeEditorCtrl::ClearGameData()
{
    CancelTreeEditing();
    if (m_dvc_ctrl)
    {
        m_dvc_ctrl->UnselectAll();
        AssociateDataViewModel(m_dvc_ctrl, nullptr);
    }
    m_models.clear();
    m_entries.clear();
    m_category_root = ScriptTreeNode();
    if (m_entry_list)
    {
        m_entry_list->Clear();
    }
    m_gd.reset();
    m_open = false;
}

void ScriptTableTreeEditorCtrl::Open(ScriptTableTreeCategory category, int entry)
{
    if (!m_gd)
    {
        return;
    }
    // Always rebuild, even when re-opening the same category: script data can be edited
    // elsewhere (e.g. the entity properties dialog's character script tree), so cached trees
    // may be stale. Keep the current entry selection when the category hasn't changed.
    const int select_entry = entry >= 0 ? entry
        : ((m_open && category == m_category) ? GetSelectedEntry() : 0);
    m_category = category;
    m_open = true;
    RebuildCategory(select_entry);
}

int ScriptTableTreeEditorCtrl::GetSelectedEntry() const
{
    return m_entry_list ? m_entry_list->GetSelection() : m_selected_entry;
}

std::shared_ptr<ScriptFunctionTable> ScriptTableTreeEditorCtrl::GetCategoryFunctions() const
{
    if (!m_gd || !m_gd->GetScriptData())
    {
        return nullptr;
    }
    switch (m_category)
    {
    case ScriptTableTreeCategory::SHOP: return m_gd->GetScriptData()->GetShopFuncs();
    case ScriptTableTreeCategory::ITEM: return m_gd->GetScriptData()->GetItemFuncs();
    case ScriptTableTreeCategory::CHARACTER: return m_gd->GetScriptData()->GetCharFuncs();
    case ScriptTableTreeCategory::CUTSCENE: return m_gd->GetScriptData()->GetCutsceneFuncs();
    default: return nullptr;
    }
}

std::vector<std::shared_ptr<ScriptFunctionTable>> ScriptTableTreeEditorCtrl::GetFunctionPool() const
{
    if (!m_gd || !m_gd->GetScriptData())
    {
        return {};
    }
    // Every function file can reference any other's functions, so all categories share one
    // resolution pool (each table still writes back to its own ASM file on save).
    auto sd = m_gd->GetScriptData();
    return { sd->GetCharFuncs(), sd->GetCutsceneFuncs(), sd->GetShopFuncs(), sd->GetItemFuncs() };
}

wxString ScriptTableTreeEditorCtrl::GetCategoryName() const
{
    switch (m_category)
    {
    case ScriptTableTreeCategory::SHOP: return "Shops";
    case ScriptTableTreeCategory::ITEM: return "Shops : Custom Items";
    case ScriptTableTreeCategory::CHARACTER: return "Characters";
    case ScriptTableTreeCategory::CUTSCENE: return "Cutscenes";
    default: return "Script";
    }
}

wxString ScriptTableTreeEditorCtrl::GetFunctionYamlFilename() const
{
    switch (m_category)
    {
    case ScriptTableTreeCategory::SHOP: return "shop_funcs.yaml";
    case ScriptTableTreeCategory::ITEM: return "shop_item_funcs.yaml";
    case ScriptTableTreeCategory::CHARACTER: return "character_funcs.yaml";
    case ScriptTableTreeCategory::CUTSCENE: return "cutscene_funcs.yaml";
    default: return "script_funcs.yaml";
    }
}

wxString ScriptTableTreeEditorCtrl::GetTableYamlFilename() const
{
    switch (m_category)
    {
    case ScriptTableTreeCategory::CHARACTER: return "character_tables.yaml";
    case ScriptTableTreeCategory::CUTSCENE: return "cutscene_tables.yaml";
    default: return wxEmptyString;
    }
}

wxString ScriptTableTreeEditorCtrl::GetTableAsmFilename() const
{
    switch (m_category)
    {
    case ScriptTableTreeCategory::CHARACTER: return "character_tables.asm";
    case ScriptTableTreeCategory::CUTSCENE: return "cutscene_tables.asm";
    default: return wxEmptyString;
    }
}

ScriptTreeDataViewModel* ScriptTableTreeEditorCtrl::GetTreeModel() const
{
    return m_dvc_ctrl ? static_cast<ScriptTreeDataViewModel*>(m_dvc_ctrl->GetModel()) : nullptr;
}

void ScriptTableTreeEditorCtrl::ExpandTree(const wxDataViewItem& parent)
{
    // ExpandChildren() recursively expands the whole subtree natively in one call, tracking
    // row positions incrementally. Calling Expand() individually per node re-resolves the
    // item's row from scratch on every single call, which becomes quadratic as the tree grows.
    wxDataViewModel* model = m_dvc_ctrl->GetModel();
    if (!model)
    {
        return;
    }
    wxDataViewItemArray children;
    model->GetChildren(parent, children);
    for (const auto& child : children)
    {
        m_dvc_ctrl->ExpandChildren(child);
    }
}

void ScriptTableTreeEditorCtrl::CancelTreeEditing()
{
    if (!m_dvc_ctrl)
    {
        return;
    }
    wxDataViewColumn* column = m_dvc_ctrl->GetColumn(0);
    if (wxDataViewRenderer* renderer = column ? column->GetRenderer() : nullptr)
    {
        renderer->CancelEditing();
    }
}

void ScriptTableTreeEditorCtrl::CommitTreeEditing()
{
    if (!m_dvc_ctrl)
    {
        return;
    }
    wxDataViewColumn* column = m_dvc_ctrl->GetColumn(0);
    wxDataViewRenderer* renderer = column ? column->GetRenderer() : nullptr;
    if (renderer && renderer->GetEditorCtrl())
    {
        // Pulls the editor's value through the model (SetValue -> SyncToScriptTable) and
        // closes the editor; an invalid value is simply dropped, as if editing were escaped.
        renderer->FinishEditing();
    }
}

void ScriptTableTreeEditorCtrl::SelectScriptEntry(int entry_index)
{
    if (entry_index < 0 || static_cast<std::size_t>(entry_index) >= m_entries.size())
    {
        return;
    }

    // The tree's in-place editor is a floating child window positioned over a specific row; if
    // it's left open while we swap in a different entry's model below, it stays visible, now
    // floating over whatever row happens to occupy that same screen position in the new tree.
    // Commit (rather than discard) whatever the user had typed - switching entries is a
    // navigation action, not a request to abandon the edit in progress.
    CommitTreeEditing();

    m_selected_entry = entry_index;

    if (!m_models[entry_index])
    {
        m_models[entry_index] = wxObjectDataPtr<ScriptTreeDataViewModel>(
            new ScriptTreeDataViewModel(*m_entries[entry_index], GetCategoryFunctions(), m_gd, GetFunctionPool()));
    }

    // wxDataViewCtrl::AssociateModel() rebuilds the row/tree bookkeeping but never clears the
    // control's selected-row state. If a row near the bottom of the current (larger) tree is
    // selected, the stale row index can be left pointing past the end of the newly-associated
    // (smaller) tree; wxDataViewCtrlBase::GetSelection() then indexes into an empty item array
    // and throws "out of range". Clear the selection first, while it still refers to the tree
    // that's about to be replaced, so the swap starts from a clean (empty) selection.
    m_dvc_ctrl->UnselectAll();
    // Freeze while the model is swapped and the whole tree expanded - repainting row by row
    // during the expansion is by far the slowest (and most flickery) part of a rebuild.
    m_dvc_ctrl->Freeze();
    AssociateDataViewModel(m_dvc_ctrl, m_models[entry_index].get());
    ExpandTree();
    m_dvc_ctrl->Thaw();
    UpdateEditButtons();
}

void ScriptTableTreeEditorCtrl::RebuildCategory(int select_entry)
{
    if (!m_gd || !m_open)
    {
        return;
    }

    // Commit any in-flight edit before the model it belongs to is destroyed below, or the
    // typed value would be silently lost.
    CommitTreeEditing();
    m_dvc_ctrl->UnselectAll();
    AssociateDataViewModel(m_dvc_ctrl, nullptr);
    m_models.clear();
    m_entries.clear();

    m_category_root = ScriptTreeNode::BuildCategoryTree(m_gd, static_cast<int>(m_category));
    m_category_root.SetParent();
    for (auto& entry : m_category_root.children)
    {
        m_entries.push_back(&entry);
    }
    m_models.resize(m_entries.size());

    // Update the entry list in place rather than Clear()+Append() - most rebuilds change no
    // entry names at all, and repopulating a long list is slow, flickers, and loses the scroll
    // position.
    if (m_entry_list)
    {
        m_entry_list->Freeze();
        unsigned int index_in_list = 0;
        for (const ScriptTreeNode* entry : m_entries)
        {
            if (index_in_list < m_entry_list->GetCount())
            {
                if (m_entry_list->GetString(index_in_list) != entry->name)
                {
                    m_entry_list->SetString(index_in_list, entry->name);
                }
            }
            else
            {
                m_entry_list->Append(entry->name);
            }
            ++index_in_list;
        }
        while (m_entry_list->GetCount() > m_entries.size())
        {
            m_entry_list->Delete(m_entry_list->GetCount() - 1);
        }
        m_entry_list->Thaw();
    }

    if (!m_entries.empty())
    {
        int index = std::clamp(select_entry, 0, static_cast<int>(m_entries.size()) - 1);
        if (m_entry_list)
        {
            m_entry_list->SetSelection(index);
        }
        SelectScriptEntry(index);
    }

    UpdateEditButtons();
}

void ScriptTableTreeEditorCtrl::RebuildCurrentCategory()
{
    RebuildCategory(GetSelectedEntry());
}

bool ScriptTableTreeEditorCtrl::CanAddChild() const
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    return model && !model->GetAddChildOptions(m_dvc_ctrl->GetSelection()).empty();
}

bool ScriptTableTreeEditorCtrl::CanAddSibling() const
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    return model && !model->GetAddSiblingOptions(m_dvc_ctrl->GetSelection()).empty();
}

bool ScriptTableTreeEditorCtrl::CanRemoveItem() const
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    return model && model->CanRemoveItem(m_dvc_ctrl->GetSelection());
}

bool ScriptTableTreeEditorCtrl::CanMoveItemUp() const
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    return model && model->CanMoveItemUp(m_dvc_ctrl->GetSelection());
}

bool ScriptTableTreeEditorCtrl::CanMoveItemDown() const
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    return model && model->CanMoveItemDown(m_dvc_ctrl->GetSelection());
}

void ScriptTableTreeEditorCtrl::AddChildItem()
{
    ShowAddMenu(true);
}

void ScriptTableTreeEditorCtrl::AddSiblingItem()
{
    ShowAddMenu(false);
}

void ScriptTableTreeEditorCtrl::ShowAddMenu(bool child)
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    if (!model || !m_dvc_ctrl)
    {
        return;
    }

    wxDataViewItem selected = m_dvc_ctrl->GetSelection();
    std::vector<ScriptTreeAddOption> options = child ? model->GetAddChildOptions(selected) : model->GetAddSiblingOptions(selected);
    if (options.empty())
    {
        wxMessageBox(child ? "This script item cannot have child items added." : "This script item cannot have more siblings.",
                     child ? "Add Child" : "Add Sibling",
                     wxOK | wxICON_INFORMATION,
                     this);
        UpdateEditButtons();
        return;
    }

    wxMenu menu;
    for (std::size_t i = 0; i < options.size(); ++i)
    {
        menu.Append(ID_ADD_MENU_BASE + static_cast<int>(i), options.at(i).label);
    }

    int selection = GetPopupMenuSelectionFromUser(menu);
    if (selection < ID_ADD_MENU_BASE)
    {
        return;
    }

    std::size_t option_index = static_cast<std::size_t>(selection - ID_ADD_MENU_BASE);
    if (option_index >= options.size())
    {
        return;
    }

    AddSelectedItem(child, options.at(option_index));
}

void ScriptTableTreeEditorCtrl::AddSelectedItem(bool child, const ScriptTreeAddOption& option)
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    if (!m_dvc_ctrl || !model)
    {
        return;
    }

    wxDataViewItem selected = m_dvc_ctrl->GetSelection();
    wxDataViewItem added = child ? model->AddChild(selected, option) : model->AddSibling(selected, option);
    if (added.IsOk())
    {
        if (child)
        {
            m_dvc_ctrl->Expand(selected);
        }
        m_dvc_ctrl->Expand(added);
        m_dvc_ctrl->Select(added);
        m_dvc_ctrl->EnsureVisible(added);
    }
    UpdateEditButtons();
}

void ScriptTableTreeEditorCtrl::RemoveSelectedItem()
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    if (!m_dvc_ctrl || !model)
    {
        return;
    }

    wxDataViewItem selected = m_dvc_ctrl->GetSelection();
    if (!model->CanRemoveItem(selected))
    {
        wxMessageBox("This script item cannot be removed.", "Remove", wxOK | wxICON_INFORMATION, this);
        UpdateEditButtons();
        return;
    }

    if (wxMessageBox("Remove the selected script item from this view?", "Remove", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES)
    {
        return;
    }

    // Removing a subtree can remove the only reference to an embedded function, so rebuild the
    // whole category to correctly move it back to "Other Functions" (or drop its reference
    // count/[Shared] marker) rather than leaving it stale until some other rebuild happens to
    // run. But skip the rebuild when it can't matter - a reference-free removal (a Return, a
    // Sleep, ...) inside a standalone function - since the rebuild would also relocate a
    // still-unreferenced (freshly created) function to "Other Functions" mid-edit. Both checks
    // walk the subtree, so they must run before the node is destroyed.
    const bool need_rebuild = model->SubtreeAffectsReferences(selected)
        || !model->IsInsideStandaloneFunction(selected);
    model->RemoveItem(selected);
    if (need_rebuild)
    {
        RebuildCurrentCategory();
    }
    else
    {
        UpdateEditButtons();
    }
}

void ScriptTableTreeEditorCtrl::MoveSelectedItemUp()
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    if (!m_dvc_ctrl || !model)
    {
        return;
    }

    wxDataViewItem moved = model->MoveItemUp(m_dvc_ctrl->GetSelection());
    if (moved.IsOk())
    {
        ExpandTree();
        m_dvc_ctrl->Select(moved);
        m_dvc_ctrl->EnsureVisible(moved);
    }
    UpdateEditButtons();
}

void ScriptTableTreeEditorCtrl::MoveSelectedItemDown()
{
    ScriptTreeDataViewModel* model = GetTreeModel();
    if (!m_dvc_ctrl || !model)
    {
        return;
    }

    wxDataViewItem moved = model->MoveItemDown(m_dvc_ctrl->GetSelection());
    if (moved.IsOk())
    {
        ExpandTree();
        m_dvc_ctrl->Select(moved);
        m_dvc_ctrl->EnsureVisible(moved);
    }
    UpdateEditButtons();
}

bool ScriptTableTreeEditorCtrl::CanRemoveEntry() const
{
    // "Other Functions" is always the last entry and isn't backed by a table row.
    const int index = GetSelectedEntry();
    const int real_count = static_cast<int>(m_entries.size()) - 1;
    return index >= 0 && index < real_count;
}

bool ScriptTableTreeEditorCtrl::CanMoveEntryUp() const
{
    const int index = GetSelectedEntry();
    return CanRemoveEntry() && index - 1 >= 0;
}

bool ScriptTableTreeEditorCtrl::CanMoveEntryDown() const
{
    const int index = GetSelectedEntry();
    const int real_count = static_cast<int>(m_entries.size()) - 1;
    return CanRemoveEntry() && index + 1 < real_count;
}

void ScriptTableTreeEditorCtrl::AddEntry()
{
    if (!m_gd || !m_open)
    {
        return;
    }

    int new_index = AddTableEntry(m_gd, m_category);
    if (new_index < 0)
    {
        return;
    }

    RebuildCategory(new_index);
}

void ScriptTableTreeEditorCtrl::RemoveEntry()
{
    if (!m_gd || !m_open || !m_entry_list)
    {
        return;
    }

    int index = GetSelectedEntry();
    if (!CanRemoveEntry())
    {
        wxMessageBox("This entry cannot be removed.", "Remove Entry", wxOK | wxICON_INFORMATION, this);
        return;
    }

    if (wxMessageBox("Remove the selected entry? This cannot be undone.", "Remove Entry", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES)
    {
        return;
    }

    if (!RemoveTableEntry(m_gd, m_category, static_cast<std::size_t>(index)))
    {
        return;
    }

    RebuildCategory(index);
}

void ScriptTableTreeEditorCtrl::MoveEntryUp()
{
    MoveCurrentEntry(-1);
}

void ScriptTableTreeEditorCtrl::MoveEntryDown()
{
    MoveCurrentEntry(1);
}

void ScriptTableTreeEditorCtrl::MoveCurrentEntry(int direction)
{
    if (!m_gd || !m_open || !m_entry_list)
    {
        return;
    }

    int index = GetSelectedEntry();
    if (direction < 0 ? !CanMoveEntryUp() : !CanMoveEntryDown())
    {
        return;
    }

    if (!MoveTableEntry(m_gd, m_category, static_cast<std::size_t>(index), direction))
    {
        return;
    }

    RebuildCategory(index + direction);
}

void ScriptTableTreeEditorCtrl::OnTreeSelectionChanged(wxDataViewEvent& event)
{
    event.Skip();
    UpdateEditButtons();
}

void ScriptTableTreeEditorCtrl::OnTreeItemActivated(wxDataViewEvent& event)
{
    ScriptTreeDataViewModel* model = GetTreeModel();

    // A Custom ASM block is edited as a whole in a multi-line dialog rather than in place -
    // activating either the block itself or any of its instruction lines opens it.
    const wxDataViewItem asm_block = (model && event.GetItem().IsOk())
        ? model->GetCustomAsmBlock(event.GetItem()) : wxDataViewItem();
    if (m_dvc_ctrl && model && asm_block.IsOk())
    {
        wxTextEntryDialog dialog(this,
                                 "Edit the custom assembler block, one instruction per line:",
                                 "Custom ASM",
                                 model->GetCustomAsmText(asm_block),
                                 wxTextEntryDialogStyle | wxTE_MULTILINE);
        MakeModalDialogMovable(&dialog);
        for (wxWindow* child : dialog.GetChildren())
        {
            if (wxTextCtrl* text = wxDynamicCast(child, wxTextCtrl))
            {
                text->SetFont(wxFont(wxFontInfo(dialog.GetFont().GetPointSize()).Family(wxFONTFAMILY_TELETYPE)));
                text->SetMinSize(FromDIP(wxSize(480, 240)));
            }
        }
        if (wxSizer* sizer = dialog.GetSizer())
        {
            sizer->SetSizeHints(&dialog);
        }
        dialog.Centre();
        if (dialog.ShowModal() == wxID_OK)
        {
            if (model->ApplyCustomAsm(asm_block, dialog.GetValue()))
            {
                // ASM lines can mention function labels, which feeds the reference analysis -
                // refresh the category like any other committed edit.
                RebuildCurrentCategory();
            }
            else
            {
                wxMessageBox("The assembler block was not changed - check that every line is a single, valid instruction, "
                             "that at least one instruction remains, and that no line is a script-structure instruction "
                             "(rts, bra, trap #0-#2) - those have their own statement types.",
                             "Custom ASM", wxOK | wxICON_WARNING, this);
            }
        }
        event.Skip();
        return;
    }

    // Script preview rows aren't editable in place - double-clicking one opens a popup instead:
    // the linked cutscene/character's own script tree for hyperlinked rows, otherwise the
    // segment script editor for that row's script line.
    const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(event.GetItem().GetID());
    if (m_gd && node && node->type == ScriptTreeNodeType::SCRIPT_ENTRY)
    {
        OpenScriptEntryPopup(*node);
        event.Skip();
        return;
    }

    if (m_dvc_ctrl && event.GetItem().IsOk())
    {
        m_dvc_ctrl->EditItem(event.GetItem(), m_dvc_ctrl->GetColumn(0));
    }
    event.Skip();
}

void ScriptTableTreeEditorCtrl::OpenScriptEntryPopup(const ScriptTreeNode& node)
{
    // Both popups edit the same shared data this tree is displaying - flush any in-flight cell
    // edit first, then rebuild afterwards so the preview rows reflect whatever was changed.
    CommitTreeEditing();
    if (ScriptEntryPopup::Open(this, m_gd, GetTreeModel(), node))
    {
        RebuildCurrentCategory();
    }
}

void ScriptTableTreeEditorCtrl::OnTreeItemEditingDone(wxDataViewEvent& event)
{
    // The model isn't updated with the new value until after this event is processed, so defer
    // until that has happened. A Script Action edit can add or remove a function reference, so
    // do a full category rebuild (rather than just expanding the edited item) to keep reference
    // counts, the "[Shared xN]" markers, and "Other Functions" membership accurate everywhere.
    if (!event.IsEditCancelled() && event.GetItem().IsOk())
    {
        // Value-only edits (numeric parameters, quest/progress) can't change any function
        // reference; when made inside a standalone function - whose body is displayed nowhere
        // else - the rebuild would achieve nothing except relocating a still-unreferenced
        // (freshly created) function to "Other Functions" mid-edit, so skip it. Edits inside
        // merged/embedded bodies still rebuild: the same function can be displayed by other
        // entries, whose cached models would otherwise go stale.
        bool value_only = false;
        const ScriptTreeNode* node = reinterpret_cast<const ScriptTreeNode*>(event.GetItem().GetID());
        ScriptTreeDataViewModel* model = GetTreeModel();
        if (node && model)
        {
            switch (node->type)
            {
            case ScriptTreeNodeType::PLAY_SOUND:
            case ScriptTreeNodeType::SET_FLAG:
            case ScriptTreeNodeType::CHECK_FLAG:
            case ScriptTreeNodeType::SLEEP:
            case ScriptTreeNodeType::CUSTOM_SHOP_ACTION:
            case ScriptTreeNodeType::PROG_DEP_ACTION:
                value_only = model->IsInsideStandaloneFunction(event.GetItem());
                break;
            default:
                break;
            }
        }
        if (!value_only)
        {
            CallAfter([this]()
            {
                RebuildCurrentCategory();
            });
        }
    }
    event.Skip();
}

void ScriptTableTreeEditorCtrl::UpdateEditButtons()
{
    if (wxWindow* add_child_button = FindWindow(ID_ADD_SCRIPT_CHILD))
    {
        add_child_button->Enable(CanAddChild());
    }
    if (wxWindow* add_button = FindWindow(ID_ADD_SCRIPT_ITEM))
    {
        add_button->Enable(CanAddSibling());
    }
    if (wxWindow* remove_button = FindWindow(ID_REMOVE_SCRIPT_ITEM))
    {
        remove_button->Enable(CanRemoveItem());
    }
    if (wxWindow* move_up_button = FindWindow(ID_MOVE_SCRIPT_ITEM_UP))
    {
        move_up_button->Enable(CanMoveItemUp());
    }
    if (wxWindow* move_down_button = FindWindow(ID_MOVE_SCRIPT_ITEM_DOWN))
    {
        move_down_button->Enable(CanMoveItemDown());
    }
    if (wxWindow* remove_entry_button = FindWindow(ID_REMOVE_ENTRY))
    {
        remove_entry_button->Enable(CanRemoveEntry());
    }
    if (wxWindow* move_entry_up_button = FindWindow(ID_MOVE_ENTRY_UP))
    {
        move_entry_up_button->Enable(CanMoveEntryUp());
    }
    if (wxWindow* move_entry_down_button = FindWindow(ID_MOVE_ENTRY_DOWN))
    {
        move_entry_down_button->Enable(CanMoveEntryDown());
    }
    if (m_on_state_change)
    {
        m_on_state_change();
    }
}

bool ScriptTableTreeEditorCtrl::SetScriptTable(ScriptFunctionTable functions)
{
    if (!m_gd || !m_gd->GetScriptData())
    {
        return false;
    }

    switch (m_category)
    {
    case ScriptTableTreeCategory::SHOP:
        m_gd->GetScriptData()->SetShopFuncs(functions);
        return true;
    case ScriptTableTreeCategory::ITEM:
        m_gd->GetScriptData()->SetItemFuncs(functions);
        return true;
    case ScriptTableTreeCategory::CHARACTER:
        m_gd->GetScriptData()->SetCharFuncs(functions);
        return true;
    case ScriptTableTreeCategory::CUTSCENE:
        m_gd->GetScriptData()->SetCutsceneFuncs(functions);
        return true;
    default:
        return false;
    }
}

void ScriptTableTreeEditorCtrl::ImportScript(bool yaml)
{
    if (!m_gd || !m_open)
    {
        wxMessageBox("No script table is open.", yaml ? "Import YAML" : "Import ASM", wxOK | wxICON_WARNING, this);
        return;
    }

    wxString wildcard = yaml ? "YAML files (*.yaml;*.yml)|*.yaml;*.yml|All files (*.*)|*.*" : "Assembly files (*.asm)|*.asm|All files (*.*)|*.*";
    wxFileDialog dialog(this,
                        yaml ? "Import script from YAML" : "Import script from ASM",
                        wxEmptyString,
                        yaml ? GetFunctionYamlFilename() : wxString(),
                        wildcard,
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() != wxID_OK)
    {
        return;
    }

    // Discard (don't commit) any edit in progress: it belongs to the table that's about to be
    // replaced wholesale, and committing it during the post-import rebuild would sync stale
    // pre-import content over the freshly imported functions.
    CancelTreeEditing();

    try
    {
        std::filesystem::path path(dialog.GetPath().ToStdWstring());
        ScriptFunctionTable imported;
        if (yaml)
        {
            std::ifstream in(path, std::ios::binary);
            if (!in)
            {
                throw std::runtime_error("Unable to open YAML file.");
            }
            std::ostringstream buffer;
            buffer << in.rdbuf();
            imported = ScriptFunctionTable(buffer.str());
        }
        else
        {
            AsmFile file(path);
            imported = ScriptFunctionTable(file);
        }

        if (imported.GetFunctionNames().empty())
        {
            throw std::runtime_error("The imported file did not contain any script functions.");
        }

        if (!SetScriptTable(imported))
        {
            throw std::runtime_error("Unable to update the selected script table.");
        }

        RebuildCategory(0);
        wxMessageBox("Import complete.", yaml ? "Import YAML" : "Import ASM", wxOK | wxICON_INFORMATION, this);
    }
    catch (const std::exception& e)
    {
        wxMessageBox(e.what(), yaml ? "Import YAML" : "Import ASM", wxOK | wxICON_ERROR, this);
    }
}

void ScriptTableTreeEditorCtrl::ExportScript(bool yaml)
{
    auto functions = GetCategoryFunctions();
    if (!functions || !m_open)
    {
        wxMessageBox("No script table is open.", yaml ? "Export" : "Save", wxOK | wxICON_WARNING, this);
        return;
    }

    // The export snapshots the table, so any in-flight in-place edit must reach it first (the
    // menu click that got us here doesn't move focus, so the editor won't have committed itself).
    CommitTreeEditing();

    const wxString name = GetCategoryName();
    const wxString filename = yaml ? GetFunctionYamlFilename() : name + ".asm";
    wxString wildcard = yaml ? "YAML files (*.yaml)|*.yaml|All files (*.*)|*.*" : "Assembly files (*.asm)|*.asm|All files (*.*)|*.*";
    wxFileDialog dialog(this,
                        yaml ? "Export script as YAML" : "Export script as ASM",
                        wxEmptyString,
                        filename,
                        wildcard,
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() != wxID_OK)
    {
        return;
    }

    std::filesystem::path path(dialog.GetPath().ToStdWstring());
    bool ok = false;
    if (yaml)
    {
        try
        {
            std::ofstream out(path, std::ios::binary);
            if (out)
            {
                out << functions->ToYaml(name.ToStdString());
                ok = out.good();
            }
        }
        catch (const std::exception& e)
        {
            // ToYaml() refuses to serialize invalid (empty) script actions.
            wxMessageBox(wxString("Unable to export: ") + e.what(), "Export", wxOK | wxICON_ERROR, this);
            return;
        }
    }
    else
    {
        AsmFile file;
        file.WriteFileHeader(path.filename(), name.ToStdString() + " Script Functions");
        ok = functions->WriteAsm(file) && file.WriteFile(path);
    }

    if (!ok)
    {
        wxMessageBox("Unable to write the selected file.", yaml ? "Export" : "Save", wxOK | wxICON_ERROR, this);
    }
}

void ScriptTableTreeEditorCtrl::ImportTableYaml()
{
    const wxString filename = GetTableYamlFilename();
    if (!m_gd || !m_open || filename.empty())
    {
        wxMessageBox("No character or cutscene table is open.", "Import YAML", wxOK | wxICON_WARNING, this);
        return;
    }

    wxFileDialog dialog(this,
                        "Import script table from YAML",
                        wxEmptyString,
                        filename,
                        "YAML files (*.yaml;*.yml)|*.yaml;*.yml|All files (*.*)|*.*",
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_OK)
    {
        return;
    }

    // The current edit belongs to the table that is about to be replaced.
    CancelTreeEditing();

    try
    {
        std::ifstream in(std::filesystem::path(dialog.GetPath().ToStdWstring()), std::ios::binary);
        if (!in)
        {
            throw std::runtime_error("Unable to open YAML file.");
        }
        std::ostringstream buffer;
        buffer << in.rdbuf();
        auto imported = ScriptTable::TableFromYaml(buffer.str());

        auto script_data = m_gd->GetScriptData();
        if (m_category == ScriptTableTreeCategory::CHARACTER)
        {
            *script_data->GetCharTable() = std::move(imported);
        }
        else
        {
            *script_data->GetCutsceneTable() = std::move(imported);
        }

        RebuildCategory(0);
        wxMessageBox("Import complete.", "Import YAML", wxOK | wxICON_INFORMATION, this);
    }
    catch (const std::exception& e)
    {
        wxMessageBox(e.what(), "Import YAML", wxOK | wxICON_ERROR, this);
    }
}

void ScriptTableTreeEditorCtrl::ExportTableYaml()
{
    const wxString filename = GetTableYamlFilename();
    if (!m_gd || !m_open || filename.empty())
    {
        wxMessageBox("No character or cutscene table is open.", "Export YAML", wxOK | wxICON_WARNING, this);
        return;
    }

    CommitTreeEditing();

    wxFileDialog dialog(this,
                        "Export script table as YAML",
                        wxEmptyString,
                        filename,
                        "YAML files (*.yaml)|*.yaml|All files (*.*)|*.*",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK)
    {
        return;
    }

    try
    {
        auto script_data = m_gd->GetScriptData();
        const std::string yaml = m_category == ScriptTableTreeCategory::CHARACTER
            ? ScriptTable::TableToYaml(script_data->GetCharTable())
            : ScriptTable::TableToYaml(script_data->GetCutsceneTable());
        std::ofstream out(std::filesystem::path(dialog.GetPath().ToStdWstring()), std::ios::binary);
        if (!out)
        {
            throw std::runtime_error("Unable to open the selected file for writing.");
        }
        out << yaml;
        if (!out.good())
        {
            throw std::runtime_error("Unable to write the selected file.");
        }
    }
    catch (const std::exception& e)
    {
        wxMessageBox(e.what(), "Export YAML", wxOK | wxICON_ERROR, this);
    }
}

void ScriptTableTreeEditorCtrl::ImportTableAsm()
{
    const wxString filename = GetTableAsmFilename();
    if (!m_gd || !m_open || filename.empty())
    {
        wxMessageBox("No character or cutscene table is open.", "Import ASM", wxOK | wxICON_WARNING, this);
        return;
    }

    wxFileDialog dialog(this,
                        "Import script table from ASM",
                        wxEmptyString,
                        filename,
                        "Assembly files (*.asm)|*.asm|All files (*.*)|*.*",
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_OK)
    {
        return;
    }

    CancelTreeEditing();

    try
    {
        auto imported = ScriptTable::ReadTable(dialog.GetPath().ToStdString());
        if (!imported || imported->empty())
        {
            throw std::runtime_error("The imported file did not contain any script table entries.");
        }

        auto script_data = m_gd->GetScriptData();
        if (m_category == ScriptTableTreeCategory::CHARACTER)
        {
            *script_data->GetCharTable() = std::move(*imported);
        }
        else
        {
            *script_data->GetCutsceneTable() = std::move(*imported);
        }

        RebuildCategory(0);
        wxMessageBox("Import complete.", "Import ASM", wxOK | wxICON_INFORMATION, this);
    }
    catch (const std::exception& e)
    {
        wxMessageBox(e.what(), "Import ASM", wxOK | wxICON_ERROR, this);
    }
}

void ScriptTableTreeEditorCtrl::ExportTableAsm()
{
    const wxString filename = GetTableAsmFilename();
    if (!m_gd || !m_open || filename.empty())
    {
        wxMessageBox("No character or cutscene table is open.", "Export ASM", wxOK | wxICON_WARNING, this);
        return;
    }

    CommitTreeEditing();

    wxFileDialog dialog(this,
                        "Export script table as ASM",
                        wxEmptyString,
                        filename,
                        "Assembly files (*.asm)|*.asm|All files (*.*)|*.*",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK)
    {
        return;
    }

    const std::filesystem::path path(dialog.GetPath().ToStdWstring());
    auto script_data = m_gd->GetScriptData();
    const bool ok = m_category == ScriptTableTreeCategory::CHARACTER
        ? ScriptTable::WriteTable(path.parent_path(), path.filename(), "Character Script Table", script_data->GetCharTable())
        : ScriptTable::WriteTable(path.parent_path(), path.filename(), "Cutscene Script Table", script_data->GetCutsceneTable());
    if (!ok)
    {
        wxMessageBox("Unable to write the selected file.", "Export ASM", wxOK | wxICON_ERROR, this);
    }
}

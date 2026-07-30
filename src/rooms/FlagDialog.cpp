#include <rooms/FlagDialog.h>
#include <rooms/FlagDataViewModel.h>
#include <rooms/TileSwapDataViewModel.h>
#include <wx/listctrl.h>

FlagDialog::FlagDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<Landstalker::GameData> gd)
	: wxDialog(parent, wxID_ANY, "Flags", wxDefaultPosition, {800, 480},
	           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  m_gd(gd),
      m_imglst(imglst),
	  m_roomnum(room),
      m_tabs(nullptr),
      m_button_sizer(nullptr),
      m_ok(nullptr),
      m_cancel(nullptr),
      m_ctrl_add(nullptr),
      m_ctrl_delete(nullptr),
      m_ctrl_move_up(nullptr),
	  m_ctrl_move_down(nullptr)
{
	const int plus_img = m_imglst->GetIdx("plus");
    const int minus_img = m_imglst->GetIdx("minus");
    const int up_img = m_imglst->GetIdx("up");
    const int down_img = m_imglst->GetIdx("down");

    wxBoxSizer* szr1 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(szr1);

    m_tabs = new wxListbook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_LEFT);
    m_tabs->SetMinSize(wxSize(650, 360));

    szr1->Add(m_tabs, 1, wxALL | wxEXPAND, 5);

    // PageProperties third arg = rearrange_enabled (the up/down reorder buttons). Only enable it
    // where the on-disk order is semantically significant. Room transitions are resolved by first
    // match while the game scans the table in order, so their order matters (and the editor now
    // preserves it - see WarpList::GetTransitionBytes). Every other flag list here is applied
    // all-match (the game scans the whole table and acts on every entry matching the current room -
    // CheckSpriteVisibleFlags / CheckForGraphicSwapFlags) or is a keyed per-room lookup, so
    // reordering has no effect and the buttons would only mislead.
    AddPage(Landstalker::FlagType::ROOM_TRANSITION, "Room Transitions", new RoomTransitionFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, true));
    AddPage(Landstalker::FlagType::ENTITY_VISIBILITY, "Entity Visibility", new EntityVisibilityFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(Landstalker::FlagType::ONE_TIME_ENTITY_VISIBILITY, "One Time Entity Visibility", new OneTimeEventFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(Landstalker::FlagType::HIDE_MULTIPLE_ENTITIES, "Multiple Entity Visibility", new RoomClearFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(Landstalker::FlagType::LOCKED_DOOR, "Locked Door (Entity)", new LockedDoorFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(Landstalker::FlagType::PERMANENT_SWITCH, "Permanent Switches", new PermanentSwitchFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(Landstalker::FlagType::SACRED_TREE, "Sacred Trees", new SacredTreeFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(Landstalker::FlagType::TILESWAP, "Tile Swap Flags", new TileSwapFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(Landstalker::FlagType::LOCKED_DOOR_TILESWAP, "Locked Door (Tile Swap)", new LockedDoorTileSwapFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(Landstalker::FlagType::TREE_WARP, "Tree Warp Flag", new TreeWarpFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    m_tabs->GetListView()->SetColumnWidth(0, 150);

    wxBoxSizer* szr2b = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2b, 0, wxEXPAND, 5);
    wxBoxSizer* szr3 = new wxBoxSizer(wxHORIZONTAL);
    szr2b->Add(szr3, 1, wxALL | wxEXPAND, 5);

    m_ctrl_add = new wxBitmapButton(this, wxID_ADD, imglst->GetBitmap(plus_img), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW);
    m_ctrl_delete = new wxBitmapButton(this, wxID_DELETE, imglst->GetBitmap(minus_img), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW);
    m_ctrl_move_up = new wxBitmapButton(this, wxID_UP, imglst->GetBitmap(up_img), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW);
    m_ctrl_move_down = new wxBitmapButton(this, wxID_DOWN, imglst->GetBitmap(down_img), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW);
    szr3->Add(m_ctrl_add, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
    szr3->Add(m_ctrl_delete, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
    szr3->Add(m_ctrl_move_up, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
    szr3->Add(m_ctrl_move_down, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);

    m_button_sizer = new wxStdDialogButtonSizer();

    szr2b->Add(m_button_sizer, 0, wxALL, 5);

    m_ok = new wxButton(this, wxID_OK, wxT(""), wxDefaultPosition, wxDefaultSize, 0);
    m_cancel = new wxButton(this, wxID_CANCEL, wxT(""), wxDefaultPosition, wxDefaultSize, 0);
    m_ok->SetDefault();
    m_button_sizer->AddButton(m_ok);
    m_button_sizer->AddButton(m_cancel);
    m_button_sizer->Realize();

    SetMinClientSize(wxSize(800, 480));
    SetSize(wxSize(800, 480));
    GetSizer()->Fit(this);
    CentreOnParent(wxBOTH);

    std::size_t initial_tab = 0;
    for (std::size_t i = 0; i < m_tabs->GetPageCount(); ++i)
    {
        const auto* model = m_models[m_pages[i]];
        if (model->GetRowCount() > 0)
        {
            initial_tab = i;
            break;
        }
    }

    m_tabs->SetSelection(initial_tab);

    UpdateUI();

    for (auto& ctrl : m_dvc_ctrls)
    {
        ctrl.second->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(FlagDialog::OnKeyPress), nullptr, this);
    }
    m_tabs->Connect(wxEVT_LISTBOOK_PAGE_CHANGED, wxBookCtrlEventHandler(FlagDialog::OnTabChange), nullptr, this);
    m_ok->Connect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnOK), nullptr, this);
    m_cancel->Connect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnCancel), nullptr, this);
    m_ctrl_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnAdd), nullptr, this);
    m_ctrl_delete->Connect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnDelete), nullptr, this);
    m_ctrl_move_up->Connect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnMoveUp), nullptr, this);
    m_ctrl_move_down->Connect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnMoveDown), nullptr, this);
}

FlagDialog::~FlagDialog()
{
    for (auto& ctrl : m_dvc_ctrls)
    {
        ctrl.second->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(FlagDialog::OnKeyPress), nullptr, this);
    }

    m_tabs->Disconnect(wxEVT_LISTBOOK_PAGE_CHANGED, wxBookCtrlEventHandler(FlagDialog::OnTabChange), nullptr, this);
    m_ok->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnOK), nullptr, this);
    m_cancel->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnCancel), nullptr, this);
    m_ctrl_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnAdd), nullptr, this);
    m_ctrl_delete->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnDelete), nullptr, this);
    m_ctrl_move_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnMoveUp), nullptr, this);
    m_ctrl_move_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnMoveDown), nullptr, this);
}

void FlagDialog::CommitAll()
{
    FinishDataViewEditors();

    for (auto& model : m_models)
    {
        model.second->CommitData();
    }
}

void FlagDialog::AddToCurrentList()
{
    FinishDataViewEditors();
    if (m_page_properties[GetSelectedTab()].add_enabled == false)
    {
        return;
    }
    auto* model = m_models[GetSelectedTab()];
    const unsigned int old_row_count = model->GetRowCount();
    const bool added = model->AddRow(old_row_count);
    if (!added)
    {
        return;
    }
    SelectRowAfterMutation(GetSelectedTab(), model->GetRowCount() - 1);
}

void FlagDialog::DeleteFromCurrentList()
{
    FinishDataViewEditors();
    if (!m_dvc_ctrls[GetSelectedTab()]->HasSelection())
    {
        return;
    }

    if (m_page_properties[GetSelectedTab()].delete_enabled == false)
    {
        return;
    }

    auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
    auto* model = m_models[GetSelectedTab()];

    const auto selected_item = ctrl->GetSelection();
    const auto selected_id = reinterpret_cast<intptr_t>(selected_item.GetID());
    if (selected_id <= 0)
    {
        return;
    }

    const unsigned int sel = static_cast<unsigned int>(selected_id - 1);
    const bool deleted = model->DeleteRow(sel);
    if (!deleted)
    {
        return;
    }

    const unsigned int row_count = model->GetRowCount();
    if (row_count == 0)
    {
        CallAfter([this, type = GetSelectedTab()]()
        {
            m_dvc_ctrls[type]->UnselectAll();
            RefreshList(type);
        });
        return;
    }

    const unsigned int next_sel = (sel < row_count) ? sel : (row_count - 1);
    SelectRowAfterMutation(GetSelectedTab(), next_sel);
}

void FlagDialog::MoveSelectedUpCurrentList()
{
    FinishDataViewEditors();
    auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
    auto* model = m_models[GetSelectedTab()];
    if (m_dvc_ctrls[GetSelectedTab()]->HasSelection() && m_models[GetSelectedTab()]->GetRowCount() >= 2)
    {
        if (m_page_properties[GetSelectedTab()].rearrange_enabled == false)
        {
            return;
        }
        int sel = reinterpret_cast<intptr_t>(ctrl->GetSelection().GetID()) - 1;
        if (sel > 0)
        {
            model->SwapRows(sel - 1, sel);
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(sel))));
            RefreshCurrentList();
        }
    }
}

void FlagDialog::MoveSelectedDownCurrentList()
{
    FinishDataViewEditors();
    auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
    auto* model = m_models[GetSelectedTab()];
    if (m_dvc_ctrls[GetSelectedTab()]->HasSelection() && m_models[GetSelectedTab()]->GetRowCount() >= 2)
    {
        if (m_page_properties[GetSelectedTab()].rearrange_enabled == false)
        {
            return;
        }
        unsigned int sel = reinterpret_cast<intptr_t>(ctrl->GetSelection().GetID()) - 1;
        if (sel < model->GetRowCount() - 1)
        {
            model->SwapRows(sel, sel + 1);
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(sel + 2))));
            RefreshCurrentList();
        }
    }
}

void FlagDialog::AddPage(Landstalker::FlagType type, const std::string& name, BaseDataViewModel* model, const FlagDialog::PageProperties& props)
{
    auto* panel = new wxPanel(m_tabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_tabs->AddPage(panel, wxString(name), false);
    wxBoxSizer* szr = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(szr);
    m_dvc_ctrls[type] = new wxDataViewCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    szr->Add(m_dvc_ctrls[type], 1, wxALL | wxEXPAND, 5);
    m_pages.push_back(type);
    m_page_panels[type] = panel;
    m_page_properties.insert({ type, props });

    auto* ctrl = m_dvc_ctrls[type];
    ctrl->ClearColumns();
    m_models[type] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();
    model->InitControl(ctrl);
}

void FlagDialog::FinishDataViewEditors()
{
    for (auto& ctrl : m_dvc_ctrls)
    {
        for (unsigned int col = 0; col < ctrl.second->GetColumnCount(); ++col)
        {
            auto* column = ctrl.second->GetColumn(col);
            auto* renderer = column ? column->GetRenderer() : nullptr;
            if (renderer && renderer->GetEditorCtrl())
            {
                renderer->FinishEditing();
            }
        }
    }
}

void FlagDialog::SelectRowAfterMutation(Landstalker::FlagType type, unsigned int row)
{
    CallAfter([this, type, row]()
    {
        auto* ctrl = m_dvc_ctrls[type];
        auto* model = m_models[type];
        if (row >= model->GetRowCount())
        {
            return;
        }

        const auto item = wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(row + 1)));
        ctrl->Select(item);
        ctrl->EnsureVisible(item);
        RefreshList(type);
    });
}

void FlagDialog::UpdateUI()
{
    UpdateUI(GetSelectedTab());
}

void FlagDialog::UpdateUI(Landstalker::FlagType type)
{
    const auto& props = m_page_properties[type];
    m_ctrl_add->Enable(props.add_enabled);
    m_ctrl_delete->Enable(props.delete_enabled);
    m_ctrl_move_up->Enable(props.rearrange_enabled);
    m_ctrl_move_down->Enable(props.rearrange_enabled);
}

void FlagDialog::RefreshList(Landstalker::FlagType type)
{
    auto* ctrl = m_dvc_ctrls[type];
    ctrl->Layout();
    ctrl->Refresh();
    ctrl->Update();
}

void FlagDialog::RefreshCurrentList()
{
    RefreshList(GetSelectedTab());
}

Landstalker::FlagType FlagDialog::GetSelectedTab()
{
    return GetTabByIndex(m_tabs->GetSelection());
}

Landstalker::FlagType FlagDialog::GetTabByIndex(int selection) const
{
    if (selection < 0 || static_cast<std::size_t>(selection) >= m_pages.size())
    {
        return m_pages.front();
    }
    return m_pages[static_cast<std::size_t>(selection)];
}

void FlagDialog::OnTabChange(wxBookCtrlEvent& evt)
{
    FinishDataViewEditors();
    const int selection = evt.GetSelection();
    const Landstalker::FlagType selected_type = GetTabByIndex(selection);
    evt.Skip();
    CallAfter([this, selected_type]()
    {
        UpdateUI(selected_type);
        RefreshList(selected_type);
    });
}

void FlagDialog::OnOK(wxCommandEvent& /*evt*/)
{
    CommitAll();
    EndModal(wxID_OK);
}

void FlagDialog::OnCancel(wxCommandEvent& /*evt*/)
{
    EndModal(wxID_CANCEL);
}

void FlagDialog::OnAdd(wxCommandEvent& evt)
{
    AddToCurrentList();
    evt.Skip();
}

void FlagDialog::OnDelete(wxCommandEvent& evt)
{
    DeleteFromCurrentList();
    evt.Skip();
}

void FlagDialog::OnMoveUp(wxCommandEvent& evt)
{
    MoveSelectedUpCurrentList();
    evt.Skip();
}

void FlagDialog::OnMoveDown(wxCommandEvent& evt)
{
    MoveSelectedDownCurrentList();
    evt.Skip();
}

void FlagDialog::OnKeyPress(wxKeyEvent& evt)
{
    switch (evt.GetKeyCode())
    {
    case WXK_DELETE:
        DeleteFromCurrentList();
        break;
    case WXK_INSERT:
        AddToCurrentList();
        break;
    case WXK_UP:
        if (evt.ControlDown())
        {
            MoveSelectedUpCurrentList();
        }
        break;
    case WXK_DOWN:
        if (evt.ControlDown())
        {
            MoveSelectedDownCurrentList();
        }
    }
    evt.Skip();
}

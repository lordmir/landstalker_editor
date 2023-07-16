#include <FlagDialog.h>
#include <FlagDataViewModel.h>

FlagDialog::FlagDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<GameData> gd)
	: wxDialog(parent, wxID_ANY, "Flags", wxDefaultPosition, {640, 480}),
      m_imglst(imglst),
	  m_gd(gd),
	  m_roomnum(room)
{
    const int plus_img = m_imglst->GetIdx("plus");
    const int minus_img = m_imglst->GetIdx("minus");
    const int up_img = m_imglst->GetIdx("up");
    const int down_img = m_imglst->GetIdx("down");

    wxBoxSizer* szr1 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(szr1);

    m_tabs = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBK_DEFAULT | wxNB_MULTILINE);

    szr1->Add(m_tabs, 1, wxALL | wxEXPAND, 5);

    AddPage(FlagType::ROOM_TRANSITION, "Room Transitions", PageProperties(true, true, false));
    AddPage(FlagType::ENTITY_VISIBILITY, "Entity Visibility", PageProperties(true, true, true));
    AddPage(FlagType::ONE_TIME_ENTITY_VISIBILITY, "One Time Entity Visibility", PageProperties(true, true, true));
    AddPage(FlagType::HIDE_MULTIPLE_ENTITIES, "Multiple Entity Visibility", PageProperties(true, true, true));
    AddPage(FlagType::LOCKED_DOOR, "Locked Door (Entity)", PageProperties(true, true, true));
    AddPage(FlagType::PERMANENT_SWITCH, "Permanent Switches", PageProperties(true, true, true));
    AddPage(FlagType::SACRED_TREE, "Sacred Trees", PageProperties(true, true, false));
    AddPage(FlagType::TILE_SWAP, "Tile Swaps", PageProperties(true, true, true));
    AddPage(FlagType::LOCKED_DOOR_TILESWAP, "Locked Door (Tile Swap)", PageProperties(true, true, true));

    InitRoomTransitionFlags();
    InitEntityVisibleFlags();
    InitOneTimeFlags();
    InitRoomClearFlags();
    InitLockedDoorFlags();
    InitPermanentSwitchFlags();
    InitSacredTreeFlags();
    InitTileSwapFlags();
    InitLockedDoorTileSwapFlags();

    wxBoxSizer* szr2b = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2b, 0, wxEXPAND, 5);
    wxBoxSizer* szr3 = new wxBoxSizer(wxHORIZONTAL);
    szr2b->Add(szr3, 1, wxALL | wxEXPAND, 5);

    m_ctrl_add = new wxBitmapButton(this, wxID_ADD, imglst->GetBitmap(plus_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    m_ctrl_delete = new wxBitmapButton(this, wxID_DELETE, imglst->GetBitmap(minus_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    m_ctrl_move_up = new wxBitmapButton(this, wxID_UP, imglst->GetBitmap(up_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    m_ctrl_move_down = new wxBitmapButton(this, wxID_DOWN, imglst->GetBitmap(down_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    szr3->Add(m_ctrl_add, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
    szr3->Add(m_ctrl_delete, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
    szr3->Add(m_ctrl_move_up, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
    szr3->Add(m_ctrl_move_down, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);

    m_button_sizer = new wxStdDialogButtonSizer();

    szr2b->Add(m_button_sizer, 0, wxALL, 5);

    m_ok = new wxButton(this, wxID_OK, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_cancel = new wxButton(this, wxID_CANCEL, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ok->SetDefault();
    m_button_sizer->AddButton(m_ok);
    m_button_sizer->AddButton(m_cancel);
    m_button_sizer->Realize();

    SetMinClientSize(wxSize(640, 480));
    SetSize(wxSize(640, 480));
    GetSizer()->Fit(this);
    CentreOnParent(wxBOTH);

    UpdateUI();

    for (auto& ctrl : m_dvc_ctrls)
    {
        ctrl.second->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(FlagDialog::OnKeyPress), nullptr, this);
    }
    m_tabs->Connect(wxEVT_BOOKCTRL_PAGE_CHANGED, wxBookCtrlEventHandler(FlagDialog::OnTabChange), nullptr, this);
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
    m_tabs->Disconnect(wxEVT_BOOKCTRL_PAGE_CHANGED, wxBookCtrlEventHandler(FlagDialog::OnTabChange), nullptr, this);
    m_ok->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnOK), nullptr, this);
    m_cancel->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnCancel), nullptr, this);
    m_ctrl_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnAdd), nullptr, this);
    m_ctrl_delete->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnDelete), nullptr, this);
    m_ctrl_move_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnMoveUp), nullptr, this);
    m_ctrl_move_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(FlagDialog::OnMoveDown), nullptr, this);
}

void FlagDialog::CommitAll()
{
    for (auto& model : m_models)
    {
        model.second->CommitData();
    }
}

void FlagDialog::AddToCurrentList()
{
    if (m_page_properties[GetSelectedTab()].add_enabled == false)
    {
        return;
    }
    auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
    auto* model = m_models[GetSelectedTab()];
    model->AddRow(model->GetRowCount());
    ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(model->GetRowCount())));
}

void FlagDialog::DeleteFromCurrentList()
{
    if (m_dvc_ctrls[GetSelectedTab()]->HasSelection())
    {
        if (m_page_properties[GetSelectedTab()].delete_enabled == false)
        {
            return;
        }
        auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
        auto* model = m_models[GetSelectedTab()];
        unsigned int sel = reinterpret_cast<intptr_t>(ctrl->GetSelection().GetID()) - 1;
        model->DeleteRow(sel);
        if (model->GetRowCount() > sel)
        {
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel - 1)));
        }
        else if (model->GetRowCount() != 0)
        {
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(model->GetRowCount())));
        }
    }
}

void FlagDialog::MoveSelectedUpCurrentList()
{
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
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel)));
        }
    }
}

void FlagDialog::MoveSelectedDownCurrentList()
{
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
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel + 2)));
        }
    }
}

void FlagDialog::AddPage(FlagType type, const std::string& name, const FlagDialog::PageProperties& props)
{
    auto* panel = new wxPanel(m_tabs, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(m_tabs, wxSize(-1, -1)), wxTAB_TRAVERSAL);
    m_tabs->AddPage(panel, _(name), false);
    wxBoxSizer* szr = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(szr);
    m_dvc_ctrls[type] = new wxDataViewCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(600, 400));
    szr->Add(m_dvc_ctrls[type], 1, wxALL | wxEXPAND, 5);
    m_pages.push_back(type);
    m_page_properties.insert({ type, props });
}

void FlagDialog::InitRoomTransitionFlags()
{
    auto* ctrl = m_dvc_ctrls[FlagType::ROOM_TRANSITION];
    ctrl->ClearColumns();
    auto* model = new RoomTransitionFlagViewModel(m_roomnum, m_gd);
    m_models[FlagType::ROOM_TRANSITION] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();

    ctrl->InsertColumn(0, new wxDataViewColumn(model->GetColumnHeader(0),
        new wxDataViewChoiceByIndexRenderer(model->GetColumnChoices(0)), 0, 420, wxALIGN_LEFT));
    ctrl->InsertColumn(1, new wxDataViewColumn(model->GetColumnHeader(1),
        new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 140, wxALIGN_LEFT));
}

void FlagDialog::InitEntityVisibleFlags()
{
    auto* ctrl = m_dvc_ctrls[FlagType::ENTITY_VISIBILITY];
    ctrl->ClearColumns();
    auto* model = new EntityVisibilityFlagViewModel(m_roomnum, m_gd);
    m_models[FlagType::ENTITY_VISIBILITY] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();

    ctrl->InsertColumn(0, new wxDataViewColumn(model->GetColumnHeader(0),
        new wxDataViewChoiceByIndexRenderer(model->GetColumnChoices(0)), 0, 340, wxALIGN_LEFT));
    ctrl->InsertColumn(1, new wxDataViewColumn(model->GetColumnHeader(1),
        new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 120, wxALIGN_LEFT));
    ctrl->InsertColumn(2, new wxDataViewColumn(model->GetColumnHeader(2),
        new wxDataViewChoiceByIndexRenderer(model->GetColumnChoices(2)), 2, 100, wxALIGN_LEFT));
}

void FlagDialog::InitOneTimeFlags()
{
    auto* ctrl = m_dvc_ctrls[FlagType::ONE_TIME_ENTITY_VISIBILITY];
    ctrl->ClearColumns();
    auto* model = new OneTimeEventFlagViewModel(m_roomnum, m_gd);
    m_models[FlagType::ONE_TIME_ENTITY_VISIBILITY] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();

    ctrl->InsertColumn(0, new wxDataViewColumn(model->GetColumnHeader(0),
        new wxDataViewChoiceByIndexRenderer(model->GetColumnChoices(0)), 0, 200, wxALIGN_LEFT));
    ctrl->InsertColumn(1, new wxDataViewColumn(model->GetColumnHeader(1),
        new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 100, wxALIGN_LEFT));
    ctrl->InsertColumn(2, new wxDataViewColumn(model->GetColumnHeader(2),
        new wxDataViewChoiceByIndexRenderer(model->GetColumnChoices(2)), 2, 80, wxALIGN_LEFT));
    ctrl->InsertColumn(3, new wxDataViewColumn(model->GetColumnHeader(3),
        new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 3, 100, wxALIGN_LEFT));
    ctrl->InsertColumn(4, new wxDataViewColumn(model->GetColumnHeader(4),
        new wxDataViewChoiceByIndexRenderer(model->GetColumnChoices(4)), 4, 80, wxALIGN_LEFT));
}

void FlagDialog::InitRoomClearFlags()
{
    auto* ctrl = m_dvc_ctrls[FlagType::HIDE_MULTIPLE_ENTITIES];
    ctrl->ClearColumns();
    auto* model = new RoomClearFlagViewModel(m_roomnum, m_gd);
    m_models[FlagType::HIDE_MULTIPLE_ENTITIES] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();

    ctrl->InsertColumn(0, new wxDataViewColumn(model->GetColumnHeader(0),
        new wxDataViewChoiceByIndexRenderer(model->GetColumnChoices(0)), 0, 420, wxALIGN_LEFT));
    ctrl->InsertColumn(1, new wxDataViewColumn(model->GetColumnHeader(1),
        new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 140, wxALIGN_LEFT));
}

void FlagDialog::InitLockedDoorFlags()
{
    auto* ctrl = m_dvc_ctrls[FlagType::LOCKED_DOOR];
    ctrl->ClearColumns();
    auto* model = new LockedDoorFlagViewModel(m_roomnum, m_gd);
    m_models[FlagType::LOCKED_DOOR] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();

    ctrl->InsertColumn(0, new wxDataViewColumn(model->GetColumnHeader(0),
        new wxDataViewChoiceByIndexRenderer(model->GetColumnChoices(0)), 0, 420, wxALIGN_LEFT));
    ctrl->InsertColumn(1, new wxDataViewColumn(model->GetColumnHeader(1),
        new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 140, wxALIGN_LEFT));
}

void FlagDialog::InitPermanentSwitchFlags()
{
    auto* ctrl = m_dvc_ctrls[FlagType::PERMANENT_SWITCH];
    ctrl->ClearColumns();
    auto* model = new PermanentSwitchFlagViewModel(m_roomnum, m_gd);
    m_models[FlagType::PERMANENT_SWITCH] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();

    ctrl->InsertColumn(0, new wxDataViewColumn(model->GetColumnHeader(0),
        new wxDataViewChoiceByIndexRenderer(model->GetColumnChoices(0)), 0, 420, wxALIGN_LEFT));
    ctrl->InsertColumn(1, new wxDataViewColumn(model->GetColumnHeader(1),
        new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 140, wxALIGN_LEFT));
}

void FlagDialog::InitSacredTreeFlags()
{
    auto* ctrl = m_dvc_ctrls[FlagType::SACRED_TREE];
    ctrl->ClearColumns();
    auto* model = new SacredTreeFlagViewModel(m_roomnum, m_gd);
    m_models[FlagType::SACRED_TREE] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();

    ctrl->InsertColumn(0, new wxDataViewColumn(model->GetColumnHeader(0),
        new wxDataViewTextRenderer(), 0, 420, wxALIGN_LEFT));
    ctrl->InsertColumn(1, new wxDataViewColumn(model->GetColumnHeader(1),
        new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 140, wxALIGN_LEFT));
}

void FlagDialog::InitTileSwapFlags()
{
    auto* ctrl = m_dvc_ctrls[FlagType::TILE_SWAP];
    ctrl->ClearColumns();
    auto* model = new TileSwapFlagViewModel(m_roomnum, m_gd);
    m_models[FlagType::TILE_SWAP] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();

    ctrl->InsertColumn(0, new wxDataViewColumn(model->GetColumnHeader(0),
        new wxDataViewSpinRenderer(0, 30, wxDATAVIEW_CELL_EDITABLE), 0, 200, wxALIGN_LEFT));
    ctrl->InsertColumn(1, new wxDataViewColumn(model->GetColumnHeader(1),
        new wxDataViewToggleRenderer("bool", wxDATAVIEW_CELL_ACTIVATABLE), 1, 100, wxALIGN_LEFT));
    ctrl->InsertColumn(2, new wxDataViewColumn(model->GetColumnHeader(2),
        new wxDataViewSpinRenderer(0, 2047, wxDATAVIEW_CELL_EDITABLE), 2, 240, wxALIGN_LEFT));
}

void FlagDialog::InitLockedDoorTileSwapFlags()
{
    auto* ctrl = m_dvc_ctrls[FlagType::LOCKED_DOOR_TILESWAP];
    ctrl->ClearColumns();
    auto* model = new LockedDoorTileSwapFlagViewModel(m_roomnum, m_gd);
    m_models[FlagType::LOCKED_DOOR_TILESWAP] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();

    ctrl->InsertColumn(0, new wxDataViewColumn(model->GetColumnHeader(0),
        new wxDataViewSpinRenderer(0, 30, wxDATAVIEW_CELL_EDITABLE), 0, 200, wxALIGN_LEFT));
    ctrl->InsertColumn(1, new wxDataViewColumn(model->GetColumnHeader(1),
        new wxDataViewToggleRenderer("bool", wxDATAVIEW_CELL_ACTIVATABLE), 1, 100, wxALIGN_LEFT));
    ctrl->InsertColumn(2, new wxDataViewColumn(model->GetColumnHeader(2),
        new wxDataViewSpinRenderer(0, 2047, wxDATAVIEW_CELL_EDITABLE), 2, 240, wxALIGN_LEFT));
}

void FlagDialog::UpdateUI()
{
    const auto& props = m_page_properties[GetSelectedTab()];
    m_ctrl_add->Enable(props.add_enabled);
    m_ctrl_delete->Enable(props.delete_enabled);
    m_ctrl_move_up->Enable(props.rearrange_enabled);
    m_ctrl_move_down->Enable(props.rearrange_enabled);
}

FlagType FlagDialog::GetSelectedTab()
{
    return m_pages[m_tabs->GetSelection()];
}

void FlagDialog::OnTabChange(wxBookCtrlEvent& e)
{
    UpdateUI();
}

void FlagDialog::OnOK(wxCommandEvent& evt)
{
    CommitAll();
    EndModal(wxID_OK);
}

void FlagDialog::OnCancel(wxCommandEvent& evt)
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

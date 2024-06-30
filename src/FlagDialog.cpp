#include <FlagDialog.h>
#include <FlagDataViewModel.h>
#include <TileSwapDataViewModel.h>

FlagDialog::FlagDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<GameData> gd)
	: wxDialog(parent, wxID_ANY, "Flags", wxDefaultPosition, {640, 480}),
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

    m_tabs = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBK_DEFAULT | wxNB_MULTILINE);

    szr1->Add(m_tabs, 1, wxALL | wxEXPAND, 5);

    AddPage(FlagType::ROOM_TRANSITION, "Room Transitions", new RoomTransitionFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(FlagType::ENTITY_VISIBILITY, "Entity Visibility", new EntityVisibilityFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, true));
    AddPage(FlagType::ONE_TIME_ENTITY_VISIBILITY, "One Time Entity Visibility", new OneTimeEventFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, true));
    AddPage(FlagType::HIDE_MULTIPLE_ENTITIES, "Multiple Entity Visibility", new RoomClearFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, true));
    AddPage(FlagType::LOCKED_DOOR, "Locked Door (Entity)", new LockedDoorFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, true));
    AddPage(FlagType::PERMANENT_SWITCH, "Permanent Switches", new PermanentSwitchFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, true));
    AddPage(FlagType::SACRED_TREE, "Sacred Trees", new SacredTreeFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));
    AddPage(FlagType::TILESWAP, "Tile Swap Flags", new TileSwapFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, true));
    AddPage(FlagType::LOCKED_DOOR_TILESWAP, "Locked Door (Tile Swap)", new LockedDoorTileSwapFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, true));
    AddPage(FlagType::TREE_WARP, "Tree Warp Flag", new TreeWarpFlagViewModel(m_roomnum, m_gd), PageProperties(true, true, false));

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

    for (std::size_t i = 0; i < m_tabs->GetPageCount(); ++i)
    {
        const auto* model = m_models[m_pages[i]];
        if (model->GetRowCount() > 0)
        {
            m_tabs->ChangeSelection(i);
            break;
        }
    }

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

void FlagDialog::AddPage(FlagType type, const std::string& name, BaseDataViewModel* model, const FlagDialog::PageProperties& props)
{
    auto* panel = new wxPanel(m_tabs, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(m_tabs, wxSize(-1, -1)), wxTAB_TRAVERSAL);
    m_tabs->AddPage(panel, _(name), false);
    wxBoxSizer* szr = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(szr);
    m_dvc_ctrls[type] = new wxDataViewCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(600, 400));
    szr->Add(m_dvc_ctrls[type], 1, wxALL | wxEXPAND, 5);
    m_pages.push_back(type);
    m_page_properties.insert({ type, props });

    auto* ctrl = m_dvc_ctrls[type];
    ctrl->ClearColumns();
    m_models[type] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();
    model->InitControl(ctrl);
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

void FlagDialog::OnTabChange(wxBookCtrlEvent& /*evt*/)
{
    UpdateUI();
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

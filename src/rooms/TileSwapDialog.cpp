#include <rooms/TileSwapDialog.h>

TileSwapDialog::TileSwapDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<Landstalker::GameData> gd)
	: wxDialog(parent, wxID_ANY, "Tile Swaps", wxDefaultPosition, { 640, 480 }),
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

    AddPage(PageType::SWAPS, "Tile Swaps", new TileSwapDataViewModel(m_roomnum, m_gd));
    AddPage(PageType::DOORS, "Doors", new DoorDataViewModel(m_roomnum, m_gd));

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
        ctrl.second->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(TileSwapDialog::OnKeyPress), nullptr, this);
    }
    m_tabs->Connect(wxEVT_BOOKCTRL_PAGE_CHANGED, wxBookCtrlEventHandler(TileSwapDialog::OnTabChange), nullptr, this);
    m_ok->Connect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnOK), nullptr, this);
    m_cancel->Connect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnCancel), nullptr, this);
    m_ctrl_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnAdd), nullptr, this);
    m_ctrl_delete->Connect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnDelete), nullptr, this);
    m_ctrl_move_up->Connect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnMoveUp), nullptr, this);
    m_ctrl_move_down->Connect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnMoveDown), nullptr, this);
}

TileSwapDialog::~TileSwapDialog()
{
    for (auto& ctrl : m_dvc_ctrls)
    {
        ctrl.second->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(TileSwapDialog::OnKeyPress), nullptr, this);
    }
    m_tabs->Disconnect(wxEVT_BOOKCTRL_PAGE_CHANGED, wxBookCtrlEventHandler(TileSwapDialog::OnTabChange), nullptr, this);
    m_ok->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnOK), nullptr, this);
    m_cancel->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnCancel), nullptr, this);
    m_ctrl_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnAdd), nullptr, this);
    m_ctrl_delete->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnDelete), nullptr, this);
    m_ctrl_move_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnMoveUp), nullptr, this);
    m_ctrl_move_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(TileSwapDialog::OnMoveDown), nullptr, this);
}

void TileSwapDialog::CommitAll()
{
    for (auto& model : m_models)
    {
        model.second->CommitData();
    }
}

void TileSwapDialog::AddToCurrentList()
{
    auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
    auto* model = m_models[GetSelectedTab()];
    model->AddRow(model->GetRowCount());
    ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(model->GetRowCount()))));
}

void TileSwapDialog::DeleteFromCurrentList()
{
    if (m_dvc_ctrls[GetSelectedTab()]->HasSelection())
    {
        auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
        auto* model = m_models[GetSelectedTab()];
        unsigned int sel = reinterpret_cast<intptr_t>(ctrl->GetSelection().GetID()) - 1;
        model->DeleteRow(sel);
        if (model->GetRowCount() > sel)
        {
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(sel - 1))));
        }
        else if (model->GetRowCount() != 0)
        {
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(model->GetRowCount()))));
        }
    }
}

void TileSwapDialog::MoveSelectedUpCurrentList()
{
    auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
    auto* model = m_models[GetSelectedTab()];
    if (m_dvc_ctrls[GetSelectedTab()]->HasSelection() && m_models[GetSelectedTab()]->GetRowCount() >= 2)
    {
        int sel = static_cast<int>(reinterpret_cast<intptr_t>(ctrl->GetSelection().GetID())) - 1;
        if (sel > 0)
        {
            model->SwapRows(sel - 1, sel);
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(sel))));
        }
    }
}

void TileSwapDialog::MoveSelectedDownCurrentList()
{
    auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
    auto* model = m_models[GetSelectedTab()];
    if (m_dvc_ctrls[GetSelectedTab()]->HasSelection() && m_models[GetSelectedTab()]->GetRowCount() >= 2)
    {
        unsigned int sel = reinterpret_cast<intptr_t>(ctrl->GetSelection().GetID()) - 1;
        if (sel < model->GetRowCount() - 1)
        {
            model->SwapRows(sel, sel + 1);
            ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(sel + 2))));
        }
    }
}

void TileSwapDialog::SetPage(PageType p)
{
    m_tabs->ChangeSelection(static_cast<int>(p));
}

void TileSwapDialog::SelectRow(int row)
{
    auto* ctrl = m_dvc_ctrls[GetSelectedTab()];
    auto* model = m_models[GetSelectedTab()];
    if (row > 0 && row <= static_cast<int>(model->GetRowCount()))
    {
        ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(row))));
    }
    else
    {
        ctrl->UnselectAll();
    }
}

TileSwapDialog::PageType TileSwapDialog::GetLastPage() const
{
    return GetSelectedTab();
}

int TileSwapDialog::GetLastSelected() const
{
    auto* ctrl = m_dvc_ctrls.at(GetSelectedTab());
    return reinterpret_cast<intptr_t>(ctrl->GetSelection().GetID());
}

void TileSwapDialog::UpdateUI()
{
}

TileSwapDialog::PageType TileSwapDialog::GetSelectedTab() const
{
    return m_pages[m_tabs->GetSelection()];
}

void TileSwapDialog::OnTabChange(wxBookCtrlEvent& e)
{
    e.Skip();
}

void TileSwapDialog::OnOK(wxCommandEvent& /*evt*/)
{
    CommitAll();
    EndModal(wxID_OK);
}

void TileSwapDialog::OnCancel(wxCommandEvent& /*evt*/)
{
    EndModal(wxID_OK);
}

void TileSwapDialog::OnAdd(wxCommandEvent& evt)
{
    AddToCurrentList();
    evt.Skip();
}

void TileSwapDialog::OnDelete(wxCommandEvent& evt)
{
    DeleteFromCurrentList();
    evt.Skip();
}

void TileSwapDialog::OnMoveUp(wxCommandEvent& evt)
{
    MoveSelectedUpCurrentList();
    evt.Skip();
}

void TileSwapDialog::OnMoveDown(wxCommandEvent& evt)
{
    MoveSelectedDownCurrentList();
    evt.Skip();
}

void TileSwapDialog::OnKeyPress(wxKeyEvent& evt)
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

void TileSwapDialog::AddPage(const PageType type, const std::string& name, BaseDataViewModel* model)
{
    auto* panel = new wxPanel(m_tabs, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(m_tabs, wxSize(-1, -1)), wxTAB_TRAVERSAL);
    m_tabs->AddPage(panel, name, false);
    wxBoxSizer* szr = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(szr);
    m_dvc_ctrls[type] = new wxDataViewCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(600, 400));
    szr->Add(m_dvc_ctrls[type], 1, wxALL | wxEXPAND, 5);
    m_pages.push_back(type);

    auto* ctrl = m_dvc_ctrls[type];
    ctrl->ClearColumns();
    m_models[type] = model;
    model->Initialise();
    ctrl->AssociateModel(model);
    model->DecRef();
    model->InitControl(ctrl);
}

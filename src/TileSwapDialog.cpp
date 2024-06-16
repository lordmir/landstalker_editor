#include <TileSwapDialog.h>

TileSwapDialog::TileSwapDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<GameData> gd)
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

    UpdateUI();
}

TileSwapDialog::~TileSwapDialog()
{
}

void TileSwapDialog::UpdateUI()
{
}

void TileSwapDialog::AddPage(const PageType type, const std::string& name, BaseDataViewModel* model)
{
    auto* panel = new wxPanel(m_tabs, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(m_tabs, wxSize(-1, -1)), wxTAB_TRAVERSAL);
    m_tabs->AddPage(panel, _(name), false);
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

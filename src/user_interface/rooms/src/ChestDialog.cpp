#include <user_interface/rooms/include/ChestDialog.h>

ChestDialog::ChestDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<GameData> gd)
	: wxDialog(parent, wxID_ANY, "Chests", wxDefaultPosition, { 640, 480 }),
	  m_gd(gd),
	  m_imglst(imglst),
	  m_roomnum(room)
{
	wxBoxSizer* szr1 = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(szr1);

    m_ctrl_chests_enabled = new wxCheckBox(this, wxID_ANY, "Chests Enabled");
    m_ctrl_chest_list = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(620, 420));
    szr1->Add(m_ctrl_chests_enabled, 0, wxALL, 5);
    szr1->Add(m_ctrl_chest_list, 1, wxALL | wxEXPAND, 5);

    m_ctrl_chest_list->ClearColumns();
    m_model = new ChestFlagViewModel(m_roomnum, m_gd);
    m_model->Initialise();
    m_ctrl_chest_list->AssociateModel(m_model);
    m_model->DecRef();
    m_model->InitControl(m_ctrl_chest_list);

    m_button_sizer = new wxStdDialogButtonSizer();
    szr1->Add(m_button_sizer, 0, wxALL, 5);

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

    m_ok->Connect(wxEVT_BUTTON, wxCommandEventHandler(ChestDialog::OnOK), nullptr, this);
    m_cancel->Connect(wxEVT_BUTTON, wxCommandEventHandler(ChestDialog::OnCancel), nullptr, this);
    m_ctrl_chests_enabled->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(ChestDialog::OnCheck), nullptr, this);
}

ChestDialog::~ChestDialog()
{
    m_ok->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(ChestDialog::OnOK), nullptr, this);
    m_cancel->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(ChestDialog::OnCancel), nullptr, this);
}

void ChestDialog::CommitAll()
{
    m_model->CommitData();
}

void ChestDialog::UpdateUI()
{
    bool chests_on = m_model->GetChestSetFlag();
    m_ctrl_chests_enabled->SetValue(chests_on);
    m_ctrl_chest_list->Enable(chests_on);
}

void ChestDialog::OnCheck(wxCommandEvent& evt)
{
    m_model->SetChestSetFlag(m_ctrl_chests_enabled->GetValue());
    UpdateUI();
    evt.Skip();
}

void ChestDialog::OnOK(wxCommandEvent&)
{
    CommitAll();
    EndModal(wxID_OK);
}

void ChestDialog::OnCancel(wxCommandEvent&)
{
    EndModal(wxID_CANCEL);
}

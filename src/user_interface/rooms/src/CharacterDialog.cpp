#include <user_interface/rooms/include/CharacterDialog.h>

CharacterDialog::CharacterDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<GameData> gd)
	: wxDialog(parent, wxID_ANY, "Characters", wxDefaultPosition, { 640, 480 }),
	  m_gd(gd),
	  m_imglst(imglst),
	  m_roomnum(room)
{
	const int plus_img = m_imglst->GetIdx("plus");
	const int minus_img = m_imglst->GetIdx("minus");
    wxBoxSizer* szr1 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(szr1);

    m_ctrl_list = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(620, 420));
    szr1->Add(m_ctrl_list, 1, wxALL | wxEXPAND, 5);

    m_ctrl_list->ClearColumns();
    m_model = new RoomDialogueFlagViewModel(m_roomnum, m_gd);
    m_model->Initialise();
    m_ctrl_list->AssociateModel(m_model);
    m_model->DecRef();
    m_model->InitControl(m_ctrl_list);

    wxBoxSizer* szr2 = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2, 0, wxEXPAND, 5);
    wxBoxSizer* szr3 = new wxBoxSizer(wxHORIZONTAL);
    szr2->Add(szr3, 1, wxALL | wxEXPAND, 5);

    m_add = new wxBitmapButton(this, wxID_ADD, imglst->GetBitmap(plus_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    m_delete = new wxBitmapButton(this, wxID_DELETE, imglst->GetBitmap(minus_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    szr3->Add(m_add, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
    szr3->Add(m_delete, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
    
    m_button_sizer = new wxStdDialogButtonSizer();
    szr2->Add(m_button_sizer, 0, wxALL, 5);

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

    m_ok->Connect(wxEVT_BUTTON, wxCommandEventHandler(CharacterDialog::OnOK), nullptr, this);
    m_cancel->Connect(wxEVT_BUTTON, wxCommandEventHandler(CharacterDialog::OnCancel), nullptr, this);
    m_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(CharacterDialog::OnAdd), nullptr, this);
    m_delete->Connect(wxEVT_BUTTON, wxCommandEventHandler(CharacterDialog::OnDelete), nullptr, this);
    m_ctrl_list->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(CharacterDialog::OnKeyPress), nullptr, this);
}

CharacterDialog::~CharacterDialog()
{
    m_ok->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(CharacterDialog::OnOK), nullptr, this);
    m_cancel->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(CharacterDialog::OnCancel), nullptr, this);
    m_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(CharacterDialog::OnAdd), nullptr, this);
    m_delete->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(CharacterDialog::OnDelete), nullptr, this);
    m_ctrl_list->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(CharacterDialog::OnKeyPress), nullptr, this);
}

void CharacterDialog::CommitAll()
{
    m_model->CommitData();
}

void CharacterDialog::UpdateUI()
{
}

void CharacterDialog::AddRow()
{
    m_model->AddRow(m_model->GetRowCount());
    m_ctrl_list->Select(wxDataViewItem(reinterpret_cast<void*>(m_model->GetRowCount())));
}

void CharacterDialog::DeleteRow()
{
    if (m_ctrl_list->HasSelection())
    {
        unsigned int sel = reinterpret_cast<intptr_t>(m_ctrl_list->GetSelection().GetID()) - 1;
        m_model->DeleteRow(sel);
        if (m_model->GetRowCount() > sel)
        {
            m_ctrl_list->Select(wxDataViewItem(reinterpret_cast<void*>(sel - 1)));
        }
        else if (m_model->GetRowCount() != 0)
        {
            m_ctrl_list->Select(wxDataViewItem(reinterpret_cast<void*>(m_model->GetRowCount())));
        }
    }
}

void CharacterDialog::OnOK(wxCommandEvent&)
{
    CommitAll();
    EndModal(wxID_OK);
}

void CharacterDialog::OnCancel(wxCommandEvent&)
{
    EndModal(wxID_OK);
}

void CharacterDialog::OnAdd(wxCommandEvent& evt)
{
    AddRow();
    evt.Skip();
}

void CharacterDialog::OnDelete(wxCommandEvent& evt)
{
    DeleteRow();
    evt.Skip();
}

void CharacterDialog::OnKeyPress(wxKeyEvent& evt)
{
    switch (evt.GetKeyCode())
    {
    case WXK_DELETE:
        DeleteRow();
        break;
    case WXK_INSERT:
        AddRow();
        break;
    }
    evt.Skip();
}

#include <rooms/RoomErrorDialog.h>

RoomErrorDialog::RoomErrorDialog(wxWindow* parent, const std::vector<std::string>& errors)
    : wxDialog(parent, wxID_ANY, "Room Errors", wxDefaultPosition, { 300, 300 })
{
    wxBoxSizer* szr1 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(szr1);

    auto* list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(620, 420));
    if (errors.empty())
    {
        list->Insert("No Errors", 0);
    }
    else
    {
        for (const auto& e : errors)
        {
            list->Insert(wxString(e), list->GetCount());
        }
    }
    szr1->Add(list, 1, wxALL | wxEXPAND, 5);

    auto* btnszr = new wxStdDialogButtonSizer();
    szr1->Add(btnszr, 0, wxALL, 5);

    m_ok = new wxButton(this, wxID_OK, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ok->SetDefault();
    btnszr->AddButton(m_ok);
    btnszr->Realize();

    SetMinClientSize(wxSize(300, 300));
    SetSize(wxSize(300, 300));
    GetSizer()->Fit(this);
    CentreOnParent(wxBOTH);

    m_ok->Connect(wxEVT_BUTTON, wxCommandEventHandler(RoomErrorDialog::OnOK), nullptr, this);
}

RoomErrorDialog::~RoomErrorDialog()
{
    m_ok->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(RoomErrorDialog::OnOK), nullptr, this);
}

void RoomErrorDialog::OnOK(wxCommandEvent& /*evt*/)
{
	EndModal(wxID_OK);
}

#include <FlagDialog.h>
#include <FlagDataViewModel.h>

FlagDialog::FlagDialog(wxWindow* parent, uint16_t room, std::shared_ptr<GameData> gd)
	: wxDialog(parent, wxID_ANY, "Flags", wxDefaultPosition, {640, 480}),
	  m_gd(gd),
	  m_roomnum(room),
      m_model(nullptr)
{
    wxBoxSizer* szr1 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(szr1);

    m_tabs = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBK_DEFAULT);

    szr1->Add(m_tabs, 1, wxALL | wxEXPAND, 5);

    auto* panel = new wxPanel(m_tabs, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(m_tabs, wxSize(-1, -1)), wxTAB_TRAVERSAL);
    m_tabs->AddPage(panel, _("Sprite Visibility"), false);

    wxBoxSizer* szr2a = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(szr2a);

    m_dvc_sprite_visible = new wxDataViewCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(600, 400));

    szr2a->Add(m_dvc_sprite_visible, 1, wxALL | wxEXPAND, 5);

    InitSpriteVisibleFlags();

    m_button_sizer = new wxStdDialogButtonSizer();

    szr1->Add(m_button_sizer, 0, wxALL, 5);

    m_ok = new wxButton(this, wxID_OK, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_button_sizer->AddButton(m_ok);

    m_cancel = new wxButton(this, wxID_CANCEL, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_button_sizer->AddButton(m_cancel);
    m_button_sizer->Realize();

    SetMinClientSize(wxSize(640, 480));
    SetSize(wxSize(640, 480));
    GetSizer()->Fit(this);
    CentreOnParent(wxBOTH);
}

FlagDialog::~FlagDialog()
{
}

void FlagDialog::InitSpriteVisibleFlags()
{
    m_dvc_sprite_visible->ClearColumns();
    m_model = new EntityVisiblityFlagDataViewModel(m_roomnum, m_gd);
    m_dvc_sprite_visible->AssociateModel(m_model);
    m_model->DecRef();

    m_dvc_sprite_visible->InsertColumn(0, new wxDataViewColumn(m_model->GetColumnHeader(0),
        new wxDataViewChoiceByIndexRenderer(m_model->GetColumnChoices(0)), 0, 380, wxALIGN_LEFT));
    m_dvc_sprite_visible->InsertColumn(1, new wxDataViewColumn(m_model->GetColumnHeader(1),
        new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 100, wxALIGN_CENTER));
    m_dvc_sprite_visible->InsertColumn(2, new wxDataViewColumn(m_model->GetColumnHeader(2),
        new wxDataViewChoiceByIndexRenderer(m_model->GetColumnChoices(2)), 2, 80, wxALIGN_LEFT));
}

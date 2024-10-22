#include <user_interface/script/include/ScriptEditorCtrl.h>

wxBEGIN_EVENT_TABLE(ScriptEditorCtrl, wxPanel)
wxEND_EVENT_TABLE()

ScriptEditorCtrl::ScriptEditorCtrl(wxWindow* parent)
  : wxPanel(parent),
	m_model(nullptr)
{
	wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(hsizer);

	m_dvc_ctrl = new wxDataViewCtrl(this, wxID_ANY);
	hsizer->Add(m_dvc_ctrl, 1, wxALL | wxEXPAND, 5);

	GetSizer()->Fit(this);
}

ScriptEditorCtrl::~ScriptEditorCtrl()
{
}

void ScriptEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_dvc_ctrl->ClearColumns();
	delete m_model;
	m_model = new ScriptDataViewModel(gd);

	m_model->Initialise();
	m_dvc_ctrl->AssociateModel(m_model);
	m_model->DecRef();
	m_model->InitControl(m_dvc_ctrl);
	auto font = m_dvc_ctrl->GetFont();
	font.SetFamily(wxFONTFAMILY_TELETYPE);
	font.SetPointSize(10);
	m_dvc_ctrl->SetFont(font);
}

void ScriptEditorCtrl::ClearGameData()
{
	m_gd.reset();
	m_dvc_ctrl->ClearColumns();
}

void ScriptEditorCtrl::Open()
{
	/*if (m_gd)
	{
		m_text_ctrl->Clear();
		auto font = m_text_ctrl->GetFont();
		font.SetFamily(wxFONTFAMILY_TELETYPE);
		font.SetPointSize(10);
		m_text_ctrl->SetBackgroundColour(*wxWHITE);
		m_text_ctrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
		m_text_ctrl->AppendText(_(m_gd->GetScriptData()->GetScript()->GetAllScriptStrings(m_gd)));
		m_text_ctrl->SetInsertionPoint(0);
		m_text_ctrl->ShowPosition(0);
	}*/
}

void ScriptEditorCtrl::UpdateUI()
{
	/*if (m_gd)
	{
		m_text_ctrl->Clear();
		auto font = m_text_ctrl->GetFont();
		font.SetFamily(wxFONTFAMILY_TELETYPE);
		font.SetPointSize(10);
		m_text_ctrl->SetBackgroundColour(*wxWHITE);
		m_text_ctrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
		m_text_ctrl->AppendText(_(m_gd->GetScriptData()->GetScript()->GetAllScriptStrings(m_gd)));
		m_text_ctrl->SetInsertionPoint(0);
		m_text_ctrl->ShowPosition(0);
	}*/
}

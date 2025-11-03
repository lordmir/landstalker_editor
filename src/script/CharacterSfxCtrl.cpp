#include <script/CharacterSfxCtrl.h>

CharacterSfxEditorCtrl::CharacterSfxEditorCtrl(wxWindow* parent)
	: wxPanel(parent),
	  m_model(nullptr)
{
	wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(hsizer);

	m_dvc_ctrl = new wxDataViewCtrl(this, wxID_ANY);
	hsizer->Add(m_dvc_ctrl, 1, wxALL | wxEXPAND, 5);

	GetSizer()->Fit(this);
}

CharacterSfxEditorCtrl::~CharacterSfxEditorCtrl()
{
}

void CharacterSfxEditorCtrl::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_dvc_ctrl->ClearColumns();
	m_dvc_ctrl->AssociateModel(nullptr);
	m_model = new CharacterSfxDataViewModel(gd);

	m_model->Initialise();
	m_dvc_ctrl->AssociateModel(m_model);
	m_model->DecRef();
	m_model->InitControl(m_dvc_ctrl);
	m_dvc_ctrl->SetSelections({});
	UpdateUI();
}

void CharacterSfxEditorCtrl::ClearGameData()
{
	m_gd.reset();
	m_dvc_ctrl->ClearColumns();
}

void CharacterSfxEditorCtrl::Open(int chr)
{
	if (m_model)
	{
		Freeze();
		for (unsigned int i = 0; i < m_dvc_ctrl->GetColumnCount(); ++i)
		{
			m_dvc_ctrl->GetColumn(i)->SetTitle(m_model->GetColumnHeader(i));
		}
		Thaw();
		UpdateUI();
		if (chr >= 0)
		{
			m_dvc_ctrl->EnsureVisible(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(chr))));
		}
	}
}

void CharacterSfxEditorCtrl::RefreshData()
{
	m_model->Reset(m_model->GetRowCount());
	UpdateUI();
}

void CharacterSfxEditorCtrl::UpdateUI()
{
}

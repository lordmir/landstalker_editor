#include <user_interface/script/include/ScriptEditorCtrl.h>
#include <user_interface/script/include/ScriptEditorFrame.h>

wxBEGIN_EVENT_TABLE(ScriptEditorCtrl, wxPanel)
EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, OnSelectionChange)
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
	m_dvc_ctrl->AssociateModel(nullptr);
	m_model = new ScriptDataViewModel(gd);

	m_model->Initialise();
	m_dvc_ctrl->AssociateModel(m_model);
	m_model->DecRef();
	m_model->InitControl(m_dvc_ctrl);
	m_dvc_ctrl->SetSelections({});
	UpdateUI();
}

void ScriptEditorCtrl::ClearGameData()
{
	m_gd.reset();
	m_dvc_ctrl->ClearColumns();
}

void ScriptEditorCtrl::Open()
{
}

void ScriptEditorCtrl::RefreshData()
{
	m_model->Reset(m_model->GetRowCount());
	UpdateUI();
}

void ScriptEditorCtrl::InsertRow()
{
	if (IsRowSelected())
	{
		m_model->AddRow(PtrToUint(m_dvc_ctrl->GetSelection().GetID()) - 1);
	}
	else
	{
		m_model->AddRow(0);
	}
	UpdateUI();
}

void ScriptEditorCtrl::DeleteRow()
{
	if (IsRowSelected())
	{
		std::size_t sel = PtrToUint(m_dvc_ctrl->GetSelection().GetID()) - 1;
		m_model->DeleteRow(sel);
		if (m_model->GetRowCount() > sel)
		{
			m_dvc_ctrl->Select(wxDataViewItem(UintToPtr(sel + 1)));
		}
		else if (m_model->GetRowCount() != 0)
		{
			m_dvc_ctrl->Select(wxDataViewItem(UintToPtr(m_model->GetRowCount())));
		}
	}
	UpdateUI();
}

void ScriptEditorCtrl::MoveRowUp()
{
	if (IsRowSelected() && m_model->GetRowCount() >= 2)
	{
		std::size_t sel = PtrToUint(m_dvc_ctrl->GetSelection().GetID()) - 1;
		if (sel > 0)
		{
			m_model->SwapRows(sel - 1, sel);
			m_dvc_ctrl->Select(wxDataViewItem(UintToPtr(sel)));
		}
	}
	UpdateUI();
}

void ScriptEditorCtrl::MoveRowDown()
{
	if (IsRowSelected() && m_model->GetRowCount() >= 2)
	{
		std::size_t sel = PtrToUint(m_dvc_ctrl->GetSelection().GetID()) - 1;
		if (sel < m_model->GetRowCount() - 1)
		{
			m_model->SwapRows(sel, sel + 1);
			m_dvc_ctrl->Select(wxDataViewItem(UintToPtr(sel + 2)));
		}
	}
	UpdateUI();
}

bool ScriptEditorCtrl::IsRowSelected() const
{
	return m_dvc_ctrl->HasSelection();
}

bool ScriptEditorCtrl::IsSelTop() const
{
	return PtrToUint(m_dvc_ctrl->GetSelection().GetID()) < 2;
}

bool ScriptEditorCtrl::IsSelBottom() const
{
	return PtrToUint(m_dvc_ctrl->GetSelection().GetID()) >= m_model->GetRowCount();
}

void ScriptEditorCtrl::UpdateUI()
{
	static_cast<ScriptEditorFrame*>(GetParent())->UpdateUI();
}

void ScriptEditorCtrl::OnSelectionChange(wxDataViewEvent& evt)
{
	UpdateUI();
	evt.Skip();
}

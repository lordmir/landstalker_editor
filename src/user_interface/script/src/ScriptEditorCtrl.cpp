#include <user_interface/script/include/ScriptEditorCtrl.h>
#include <user_interface/script/include/ScriptEditorFrame.h>

wxBEGIN_EVENT_TABLE(ScriptEditorCtrl, wxPanel)
EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, ScriptEditorCtrl::OnSelectionChange)
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

void ScriptEditorCtrl::AppendRow()
{
	m_model->AddRow(m_model->GetRowCount());
	UpdateUI();
	m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(m_model->GetRowCount())));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
}

void ScriptEditorCtrl::InsertRow()
{
	if (IsRowSelected())
	{
		m_model->AddRow(reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1);
	}
	else
	{
		m_model->AddRow(0);
		m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(1)));
		m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	}
	UpdateUI();
}

void ScriptEditorCtrl::DeleteRow()
{
	if (IsRowSelected())
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		m_model->DeleteRow(sel);
		if (m_model->GetRowCount() > sel)
		{
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel + 1)));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
		}
		else if (m_model->GetRowCount() != 0)
		{
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(m_model->GetRowCount())));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
		}
	}
	UpdateUI();
}

void ScriptEditorCtrl::MoveRowUp()
{
	if (IsRowSelected() && m_model->GetRowCount() >= 2)
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		if (sel > 0)
		{
			m_model->SwapRows(sel - 1, sel);
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel)));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
		}
	}
	UpdateUI();
}

void ScriptEditorCtrl::MoveRowDown()
{
	if (IsRowSelected() && m_model->GetRowCount() >= 2)
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		if (sel < m_model->GetRowCount() - 1)
		{
			m_model->SwapRows(sel, sel + 1);
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel + 2)));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
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
	return reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) < 2;
}

bool ScriptEditorCtrl::IsSelBottom() const
{
	return reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) >= static_cast<intptr_t>(m_model->GetRowCount());
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

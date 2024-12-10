#include <user_interface/script/include/ScriptTableEditorCtrl.h>
#include <user_interface/script/include/ScriptTableEditorFrame.h>

wxBEGIN_EVENT_TABLE(ScriptTableEditorCtrl, wxPanel)
EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, ScriptTableEditorCtrl::OnSelectionChange)
wxEND_EVENT_TABLE()

ScriptTableEditorCtrl::ScriptTableEditorCtrl(wxWindow* parent)
	: wxPanel(parent),
	  m_model(nullptr)
{
	wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(hsizer);

	m_dvc_ctrl = new wxDataViewCtrl(this, wxID_ANY);
	hsizer->Add(m_dvc_ctrl, 1, wxALL | wxEXPAND, 5);

	GetSizer()->Fit(this);
}

ScriptTableEditorCtrl::~ScriptTableEditorCtrl()
{
}

void ScriptTableEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_dvc_ctrl->ClearColumns();
	m_dvc_ctrl->AssociateModel(nullptr);
	m_model = new ScriptTableDataViewModel(gd);

	m_model->Initialise();
	m_dvc_ctrl->AssociateModel(m_model);
	m_model->DecRef();
	m_model->InitControl(m_dvc_ctrl);
	m_dvc_ctrl->SetSelections({});
	UpdateUI();
}

void ScriptTableEditorCtrl::ClearGameData()
{
	m_gd.reset();
	m_dvc_ctrl->ClearColumns();
}

void ScriptTableEditorCtrl::Open(const ScriptTableDataViewModel::Mode& mode, unsigned int index)
{
	if (m_model)
	{
		Freeze();
		m_model->SetMode(mode, index);
		for (unsigned int i = 0; i < m_dvc_ctrl->GetColumnCount(); ++i)
		{
			m_dvc_ctrl->GetColumn(i)->SetTitle(m_model->GetColumnHeader(i));
		}
		Thaw();
		UpdateUI();
	}
}

void ScriptTableEditorCtrl::RefreshData()
{
	m_model->Reset(m_model->GetRowCount());
	UpdateUI();
}

void ScriptTableEditorCtrl::InsertRow()
{
	if (IsRowSelected())
	{
		m_model->AddRow(reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1);
	}
	else
	{
		m_model->AddRow(0);
	}
	UpdateUI();
}

void ScriptTableEditorCtrl::DeleteRow()
{
	if (IsRowSelected())
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		m_model->DeleteRow(sel);
		if (m_model->GetRowCount() > sel)
		{
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel + 1)));
		}
		else if (m_model->GetRowCount() != 0)
		{
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(m_model->GetRowCount())));
		}
	}
	UpdateUI();
}

void ScriptTableEditorCtrl::MoveRowUp()
{
	if (IsRowSelected() && m_model->GetRowCount() >= 2)
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		if (sel > 0)
		{
			m_model->SwapRows(sel - 1, sel);
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel)));
		}
	}
	UpdateUI();
}

void ScriptTableEditorCtrl::MoveRowDown()
{
	if (IsRowSelected() && m_model->GetRowCount() >= 2)
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		if (sel < m_model->GetRowCount() - 1)
		{
			m_model->SwapRows(sel, sel + 1);
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(sel + 2)));
		}
	}
	UpdateUI();
}

bool ScriptTableEditorCtrl::IsRowSelected() const
{
	return m_dvc_ctrl->HasSelection();
}

bool ScriptTableEditorCtrl::IsSelTop() const
{
	return reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) < 2;
}

bool ScriptTableEditorCtrl::IsSelBottom() const
{
	return reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) >= static_cast<intptr_t>(m_model->GetRowCount());
}

void ScriptTableEditorCtrl::UpdateUI()
{
	static_cast<ScriptTableEditorFrame*>(GetParent())->UpdateUI();
}

void ScriptTableEditorCtrl::OnSelectionChange(wxDataViewEvent& evt)
{
	UpdateUI();
	evt.Skip();
}

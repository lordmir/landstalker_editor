#include <user_interface/script/include/ProgressFlagsCtrl.h>
#include <user_interface/script/include/ProgressFlagsFrame.h>

wxBEGIN_EVENT_TABLE(ProgressFlagsEditorCtrl, wxPanel)
EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, ProgressFlagsEditorCtrl::OnSelectionChange)
wxEND_EVENT_TABLE()

ProgressFlagsEditorCtrl::ProgressFlagsEditorCtrl(wxWindow* parent)
	: wxPanel(parent),
	  m_model(nullptr)
{
	wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(hsizer);

	m_dvc_ctrl = new wxDataViewCtrl(this, wxID_ANY);
	hsizer->Add(m_dvc_ctrl, 1, wxALL | wxEXPAND, 5);

	GetSizer()->Fit(this);
}

ProgressFlagsEditorCtrl::~ProgressFlagsEditorCtrl()
{
}

void ProgressFlagsEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_dvc_ctrl->ClearColumns();
	m_dvc_ctrl->AssociateModel(nullptr);
	m_model = new ProgressFlagsDataViewModel(gd);

	m_model->Initialise();
	m_dvc_ctrl->AssociateModel(m_model);
	m_model->DecRef();
	m_model->InitControl(m_dvc_ctrl);
	m_dvc_ctrl->SetSelections({});
	UpdateUI();
}

void ProgressFlagsEditorCtrl::ClearGameData()
{
	m_gd.reset();
	m_dvc_ctrl->ClearColumns();
}

void ProgressFlagsEditorCtrl::Open(int /*quest*/, int /*prog*/)
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
	}
}

void ProgressFlagsEditorCtrl::RefreshData()
{
	m_model->Initialise();
	UpdateUI();
}

void ProgressFlagsEditorCtrl::AddQuest()
{
	int quest = m_model->GetNextFreeQuest();
	if (quest == -1)
	{
		return;
	}
	m_model->AddQuest();
	int row = m_model->GetRowFromQuestProgress(quest, 0);
	UpdateUI();
	m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(row + 1)));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
}

void ProgressFlagsEditorCtrl::DeleteQuest()
{
	std::intptr_t row;
	if (IsRowSelected() && m_model->GetTotalQuests() > 1)
	{
		row = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		auto [q, p] = m_model->GetQuestProgressFromRow(row);
		m_model->DeleteQuest(row);
		int new_row = m_model->GetRowFromQuestProgress(std::max(q - 1, 0), 0);
		if (new_row < 0)
		{
			new_row = 1;
		}
		m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(new_row + 1)));
		m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	}
}

void ProgressFlagsEditorCtrl::AppendRow()
{
	std::intptr_t row;
	if (IsRowSelected())
	{
		row = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
	}
	else
	{
		return;
	}
	m_model->AppendRow(row);
	UpdateUI();
	auto [q, p] = m_model->GetQuestProgressFromRow(static_cast<int>(row));
	int new_row = m_model->GetRowFromQuestProgress(q, m_model->GetTotalProgressInQuest(q) - 1) + 1;
	m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(new_row)));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
}

void ProgressFlagsEditorCtrl::InsertRow()
{
	if (IsRowSelected())
	{
		std::intptr_t row = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		m_model->AddRow(row);
		std::intptr_t new_row = std::clamp<int>(row, 0, m_model->GetRowCount());
		m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(new_row + 1)));
		m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	}
	UpdateUI();
}

void ProgressFlagsEditorCtrl::DeleteRow()
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
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(m_model->GetRowCount()))));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
		}
	}
	UpdateUI();
}

void ProgressFlagsEditorCtrl::MoveRowUp()
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

void ProgressFlagsEditorCtrl::MoveRowDown()
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

bool ProgressFlagsEditorCtrl::IsRowSelected() const
{
	return m_dvc_ctrl->HasSelection();
}

bool ProgressFlagsEditorCtrl::IsSelTop() const
{
	int row = static_cast<int>(reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID())) - 1;
	auto [q, p] = m_model->GetQuestProgressFromRow(row);
	return p + 1 >= m_model->GetTotalProgressInQuest(q);
}

bool ProgressFlagsEditorCtrl::IsSelBottom() const
{
	int row = static_cast<int>(reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID())) - 1;
	auto [q, p] = m_model->GetQuestProgressFromRow(row);
	return p == 0;
}

std::pair<int, int> ProgressFlagsEditorCtrl::GetSelectedQuestProgress() const
{
	if (m_model)
	{
		int row = static_cast<int>(reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID())) - 1;
		auto pq = m_model->GetQuestProgressFromRow(row);
		return pq;
	}
	return { -1, -1 };
}

const ProgressFlagsDataViewModel* ProgressFlagsEditorCtrl::GetModel() const
{
	return m_model;
}

void ProgressFlagsEditorCtrl::OnSelectionChange(wxDataViewEvent& evt)
{
	UpdateUI();
	evt.Skip();
}

void ProgressFlagsEditorCtrl::UpdateUI()
{
	m_model->CommitData();
	static_cast<ProgressFlagsEditorFrame*>(GetParent())->UpdateUI();
}

void ProgressFlagsEditorCtrl::FireEvent(const wxEventType& e, const wxString& data, long numeric_data, long extra_numeric_data, long extra_extra_numeric_data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetInt(numeric_data);
	evt.SetExtraLong(extra_numeric_data);
	evt.SetId(extra_extra_numeric_data);
	evt.SetClientData(this);
	wxPostEvent(this->GetParent(), evt);
}

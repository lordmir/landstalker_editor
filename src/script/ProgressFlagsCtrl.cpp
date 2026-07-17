#include <script/ProgressFlagsCtrl.h>
#include <misc/DataViewModelAssociate.h>
#include <script/ProgressFlagsFrame.h>

wxBEGIN_EVENT_TABLE(ProgressFlagsEditorCtrl, wxPanel)
EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, ProgressFlagsEditorCtrl::OnSelectionChange)
wxEND_EVENT_TABLE()

ProgressFlagsEditorCtrl::ProgressFlagsEditorCtrl(wxWindow* parent)
	: wxPanel(parent),
	  m_model(nullptr)
{
	wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(vsizer);

	wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_append_button = new wxButton(this, wxID_ANY, "Append");
	m_insert_button = new wxButton(this, wxID_ANY, "Insert");
	m_delete_button = new wxButton(this, wxID_ANY, "Delete");
	m_move_up_button = new wxButton(this, wxID_ANY, "Move Up");
	m_move_down_button = new wxButton(this, wxID_ANY, "Move Down");
	m_new_quest_button = new wxButton(this, wxID_ANY, "New Quest");
	m_delete_quest_button = new wxButton(this, wxID_ANY, "Delete Quest");
	m_append_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { AppendRow(); });
	m_insert_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { InsertRow(); });
	m_delete_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DeleteRow(); });
	m_move_up_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveRowUp(); });
	m_move_down_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveRowDown(); });
	m_new_quest_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { AddQuest(); });
	m_delete_quest_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
	{
		if (wxMessageBox("Really delete quest?", "Confirm", wxYES_NO, this) == wxYES)
		{
			DeleteQuest();
		}
	});
	button_sizer->Add(m_append_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_insert_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_delete_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_move_up_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_move_down_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_new_quest_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_delete_quest_button, 0);
	vsizer->Add(button_sizer, 0, wxALL, 4);

	m_dvc_ctrl = new wxDataViewCtrl(this, wxID_ANY);
	vsizer->Add(m_dvc_ctrl, 1, wxALL | wxEXPAND, 5);

	GetSizer()->Fit(this);
}

ProgressFlagsEditorCtrl::~ProgressFlagsEditorCtrl()
{
}

void ProgressFlagsEditorCtrl::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_dvc_ctrl->ClearColumns();
	AssociateDataViewModel(m_dvc_ctrl, nullptr);
	m_model = new ProgressFlagsDataViewModel(gd);

	m_model->Initialise();
	AssociateDataViewModel(m_dvc_ctrl, m_model);
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
	m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(row + 1))));
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
		m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(new_row + 1))));
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
	m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(static_cast<intptr_t>(new_row))));
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
	UpdateButtonStates();
	static_cast<ProgressFlagsEditorFrame*>(GetParent())->UpdateUI();
}

void ProgressFlagsEditorCtrl::UpdateButtonStates()
{
	// Mirrors ProgressFlagsEditorFrame's old EnableToolbarItem() conditions exactly (including
	// Move Up/Move Down checking IsSelBottom()/IsSelTop() respectively, not the other way around
	// - that's how the original toolbar logic already had it).
	const bool loaded = m_gd != nullptr && m_model != nullptr && m_gd->GetScriptData()->HasTables();
	const bool selected = loaded && IsRowSelected();
	const int q = selected ? GetSelectedQuestProgress().second : 0;
	m_append_button->Enable(selected && m_model->GetTotalProgressInQuest(q) < 255);
	m_insert_button->Enable(selected && m_model->GetTotalProgressInQuest(q) < 255);
	m_delete_button->Enable(selected && m_model->GetRowCount() > 1);
	m_move_up_button->Enable(selected && !IsSelBottom());
	m_move_down_button->Enable(selected && !IsSelTop());
	m_new_quest_button->Enable(loaded && m_model->GetTotalQuests() < 255);
	m_delete_quest_button->Enable(selected && m_model->GetTotalQuests() > 1);
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

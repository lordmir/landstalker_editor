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
	m_dvc_ctrl->GetMainWindow()->Bind(wxEVT_MOTION, &ScriptTableEditorCtrl::OnMotion, this);
	m_dvc_ctrl->GetMainWindow()->Bind(wxEVT_LEFT_DOWN, &ScriptTableEditorCtrl::OnLClick, this);
}

ScriptTableEditorCtrl::~ScriptTableEditorCtrl()
{
	m_dvc_ctrl->GetMainWindow()->Unbind(wxEVT_LEFT_DOWN, &ScriptTableEditorCtrl::OnLClick, this);
	m_dvc_ctrl->GetMainWindow()->Unbind(wxEVT_MOTION, &ScriptTableEditorCtrl::OnMotion, this);
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

void ScriptTableEditorCtrl::Open(const ScriptTableDataViewModel::Mode& mode, unsigned int index, int row)
{
	if (m_model)
	{
		Freeze();
		m_model->SetMode(mode, index);
		for (unsigned int i = 0; i < m_dvc_ctrl->GetColumnCount(); ++i)
		{
			m_dvc_ctrl->GetColumn(i)->SetTitle(m_model->GetColumnHeader(i));
		}
		if (row != -1)
		{
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(row + 1)));
			m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
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

void ScriptTableEditorCtrl::AppendRow()
{
	m_model->AddRow(m_model->GetRowCount());
	UpdateUI();
	m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(m_model->GetRowCount())));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
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
		m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(1)));
		m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
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

void ScriptTableEditorCtrl::MoveRowUp()
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

void ScriptTableEditorCtrl::MoveRowDown()
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

void ScriptTableEditorCtrl::OnMotion(wxMouseEvent& evt)
{
	HandleMouseMove(evt.GetPosition());
	evt.Skip();
}

void ScriptTableEditorCtrl::OnLClick(wxMouseEvent& evt)
{
	HandleMouseMove(evt.GetPosition());
	if (m_last_tooltip_item != -1)
	{
		ScriptTable::Action action = m_model->GetAction(m_last_tooltip_item);
		if (std::holds_alternative<uint16_t>(action))
		{
			FireEvent(EVT_GO_TO_NAV_ITEM, "Script/Main Script", std::get<uint16_t>(action));
		}
	}
	evt.Skip();
}

void ScriptTableEditorCtrl::HandleMouseMove(const wxPoint& mouse_pos)
{
	wxDataViewItem item(nullptr);
	wxDataViewColumn* col = nullptr;
	wxPoint pos = m_dvc_ctrl->ScreenToClient(m_dvc_ctrl->GetMainWindow()->ClientToScreen(mouse_pos));
	m_dvc_ctrl->HitTest(pos, item, col);
	int hovered = -1;
	if (col && item.IsOk() && col->GetModelColumn() == 2)
	{
		auto rect = m_dvc_ctrl->GetItemRect(item, col);
		rect.x += 152;
		rect.width -= 152;
		if (rect.Contains(pos))
		{
			hovered = static_cast<int>(reinterpret_cast<std::intptr_t>(item.GetID())) - 1;
		}
	}
	if (hovered != m_last_tooltip_item)
	{
		std::wstring text;
		if (hovered != -1 && m_gd)
		{
			text = ScriptTable::GetActionDescription(m_model->GetAction(hovered), m_gd);
			if (text.find(L"\n") == std::string::npos)
			{
				text.clear();
			}
		}
		m_dvc_ctrl->GetMainWindow()->SetToolTip(text);
		m_last_tooltip_item = hovered;
		m_dvc_ctrl->GetMainWindow()->SetCursor((hovered == -1 || !std::holds_alternative<uint16_t>(m_model->GetAction(hovered))) ? wxCURSOR_ARROW : wxCURSOR_HAND);
	}
}

void ScriptTableEditorCtrl::FireEvent(const wxEventType& e, const wxString& data, long numeric_data, long extra_numeric_data, long extra_extra_numeric_data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetInt(numeric_data);
	evt.SetExtraLong(extra_numeric_data);
	evt.SetId(extra_extra_numeric_data);
	evt.SetClientData(this);
	wxPostEvent(this->GetParent(), evt);
}

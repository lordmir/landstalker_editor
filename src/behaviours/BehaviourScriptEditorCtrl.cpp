#include <behaviours/BehaviourScriptEditorCtrl.h>

#include <algorithm>
#include <optional>
#include <variant>

#include <misc/DataViewModelAssociate.h>
#include <misc/InputScriptDialog.h>
#include <script/CutsceneEditorDialog.h>
#include <script/TriggerActionEditorDialog.h>

using namespace Landstalker;

namespace
{
enum
{
	ID_CTX_ADD_ABOVE = wxID_HIGHEST + 700,
	ID_CTX_ADD_BELOW,
	ID_CTX_DELETE,
	ID_CTX_MOVE_UP,
	ID_CTX_MOVE_DOWN,
	ID_CTX_EDIT,
	ID_CTX_OPEN_CUTSCENE,
	ID_CTX_OPEN_PLAYBACK,
	ID_CTX_OPEN_TRIGGER,

	ID_KB_INSERT,
	ID_KB_DELETE,
	ID_KB_MOVE_UP,
	ID_KB_MOVE_DOWN
};

// If the command starts a cutscene, the cutscene index it plays (the dialogueactions handler slot);
// otherwise nullopt. HIGH_CUTSCENE values are already decoded to the full index (+256).
std::optional<int> CutsceneIndexOf(const Behaviours::Command& cmd)
{
	if (cmd.command != Behaviours::CommandType::START_LO_CUTSCENE
		&& cmd.command != Behaviours::CommandType::START_HI_CUTSCENE)
	{
		return std::nullopt;
	}
	for (const auto& p : cmd.params)
	{
		const Behaviours::ParamType type = std::get<2>(p);
		if (type == Behaviours::ParamType::LOW_CUTSCENE || type == Behaviours::ParamType::HIGH_CUTSCENE)
		{
			const auto& val = std::get<1>(p);
			if (std::holds_alternative<int>(val))
			{
				return std::get<int>(val);
			}
		}
	}
	return std::nullopt;
}

// If the command waits on a condition (WaitForCondition), the trigger-action index it waits on
// (the TA_xx handler slot); otherwise nullopt.
std::optional<int> TriggerIndexOf(const Behaviours::Command& cmd)
{
	if (cmd.command != Behaviours::CommandType::WAIT_FOR_CONDITION)
	{
		return std::nullopt;
	}
	for (const auto& p : cmd.params)
	{
		const auto& val = std::get<1>(p);
		if (std::holds_alternative<int>(val))
		{
			return std::get<int>(val);
		}
	}
	return std::nullopt;
}

// If the command plays an input script (PlaybackInput), the input-sequence index it plays.
std::optional<int> PlaybackIndexOf(const Behaviours::Command& cmd)
{
	if (cmd.command != Behaviours::CommandType::PLAYBACK_INPUT)
	{
		return std::nullopt;
	}
	for (const auto& p : cmd.params)
	{
		const auto& val = std::get<1>(p);
		if (std::holds_alternative<int>(val))
		{
			return std::get<int>(val);
		}
	}
	return std::nullopt;
}
}

wxBEGIN_EVENT_TABLE(BehaviourScriptEditorCtrl, wxPanel)
EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, BehaviourScriptEditorCtrl::OnSelectionChange)
EVT_DATAVIEW_ITEM_CONTEXT_MENU(wxID_ANY, BehaviourScriptEditorCtrl::OnContextMenu)
wxEND_EVENT_TABLE()

BehaviourScriptEditorCtrl::BehaviourScriptEditorCtrl(wxWindow* parent)
	: wxPanel(parent),
	  m_model(nullptr),
	  m_behaviour_script(-1)
{
	wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(vsizer);

	wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
	vsizer->Add(hsizer, 0, wxALL, 0);

	m_append_button = new wxButton(this, wxID_ANY, "Append");
	m_insert_button = new wxButton(this, wxID_ANY, "Insert");
	m_delete_button = new wxButton(this, wxID_ANY, "Delete");
	m_move_up_button = new wxButton(this, wxID_ANY, "Move Up");
	m_move_down_button = new wxButton(this, wxID_ANY, "Move Down");
	m_append_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { AppendRow(); });
	m_insert_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { InsertRow(); });
	m_delete_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DeleteRow(); });
	m_move_up_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveRowUp(); });
	m_move_down_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveRowDown(); });
	hsizer->Add(m_append_button, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
	hsizer->Add(m_insert_button, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
	hsizer->Add(m_delete_button, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
	hsizer->Add(m_move_up_button, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
	hsizer->Add(m_move_down_button, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);

	m_dvc_ctrl = new wxDataViewCtrl(this, wxID_ANY);
	vsizer->Add(m_dvc_ctrl, 1, wxALL | wxEXPAND, 5);

	// wxDataViewColumn has no "fill remaining width" mode - a width of -1 is just the default
	// (~80px), and a large SetMinWidth() only looks right when the control happens to be that
	// wide (it forces a horizontal scrollbar when narrower, e.g. in the Entity Properties tab).
	// Instead, keep the last (Command) column sized to whatever space is left after the Index
	// column, re-applied whenever the control is resized.
	m_dvc_ctrl->Bind(wxEVT_SIZE, [this](wxSizeEvent& e)
	{
		FitCommandColumn();
		e.Skip();
	});

	wxAcceleratorEntry accel_entries[4];
	accel_entries[0].Set(wxACCEL_NORMAL, WXK_INSERT, ID_KB_INSERT);
	accel_entries[1].Set(wxACCEL_NORMAL, WXK_DELETE, ID_KB_DELETE);
	accel_entries[2].Set(wxACCEL_CTRL, WXK_UP, ID_KB_MOVE_UP);
	accel_entries[3].Set(wxACCEL_CTRL, WXK_DOWN, ID_KB_MOVE_DOWN);
	SetAcceleratorTable(wxAcceleratorTable(4, accel_entries));
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { InsertRow(); }, ID_KB_INSERT);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { DeleteRow(); }, ID_KB_DELETE);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { MoveRowUp(); }, ID_KB_MOVE_UP);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { MoveRowDown(); }, ID_KB_MOVE_DOWN);

	GetSizer()->Fit(this);
	RecreateModel();
}

BehaviourScriptEditorCtrl::~BehaviourScriptEditorCtrl()
{
}

void BehaviourScriptEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	RecreateModel();
}

void BehaviourScriptEditorCtrl::ClearGameData()
{
	m_gd = nullptr;
	m_behaviour_script = -1;
	RecreateModel();
}

void BehaviourScriptEditorCtrl::Open(int id)
{
	m_behaviour_script = id;
	RecreateModel();
}

int BehaviourScriptEditorCtrl::GetOpenScriptId() const
{
	return m_behaviour_script;
}

void BehaviourScriptEditorCtrl::FitCommandColumn()
{
	if (m_dvc_ctrl->GetColumnCount() < 2)
	{
		return;
	}
	wxDataViewColumn* index_col = m_dvc_ctrl->GetColumn(0);
	wxDataViewColumn* command_col = m_dvc_ctrl->GetColumn(1);
	// Client width minus the Index column and a small margin for the (possible) vertical
	// scrollbar, floored so the command content never collapses when the control is tiny.
	const int avail = m_dvc_ctrl->GetClientSize().GetWidth() - index_col->GetWidth() - 4;
	const int width = std::max(avail, 200);
	if (command_col->GetWidth() != width)
	{
		command_col->SetWidth(width);
	}
}

void BehaviourScriptEditorCtrl::RecreateModel()
{
	// Freeze both this panel and the dataview explicitly - see ScriptEditorCtrl::SetGameData()'s
	// comment for why freezing just the parent isn't enough on every platform.
	Freeze();
	m_dvc_ctrl->Freeze();
	m_dvc_ctrl->ClearColumns();
	AssociateDataViewModel(m_dvc_ctrl, nullptr);
	m_model = nullptr;
	if (m_gd && m_behaviour_script >= 0)
	{
		m_model = new BehaviourScriptDataViewModel(m_gd, m_behaviour_script);
		m_model->Initialise();
		AssociateDataViewModel(m_dvc_ctrl, m_model);
		m_model->DecRef();
		m_model->InitControl(m_dvc_ctrl);
		m_dvc_ctrl->SetSelections({});
		// InitControl re-created the columns; size the Command column to the current width.
		FitCommandColumn();
	}
	UpdateButtonStates();
	m_dvc_ctrl->Thaw();
	Thaw();
}

void BehaviourScriptEditorCtrl::AppendRow()
{
	if (!m_model)
	{
		return;
	}
	m_model->AddRow(m_model->GetRowCount());
	m_dvc_ctrl->SetCurrentItem(ItemFromRow(m_model->GetRowCount() - 1));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	UpdateButtonStates();
}

void BehaviourScriptEditorCtrl::InsertRow()
{
	if (!m_model)
	{
		return;
	}
	if (IsRowSelected())
	{
		m_model->AddRow(RowFromItem(m_dvc_ctrl->GetSelection()));
	}
	else
	{
		m_model->AddRow(0);
		m_dvc_ctrl->Select(ItemFromRow(0));
		m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	}
	UpdateButtonStates();
}

void BehaviourScriptEditorCtrl::DeleteRow()
{
	if (m_model && IsRowSelected())
	{
		DeleteRowAt(RowFromItem(m_dvc_ctrl->GetSelection()));
	}
}

void BehaviourScriptEditorCtrl::MoveRowUp()
{
	if (m_model && IsRowSelected())
	{
		MoveRowAt(RowFromItem(m_dvc_ctrl->GetSelection()), -1);
	}
}

void BehaviourScriptEditorCtrl::MoveRowDown()
{
	if (m_model && IsRowSelected())
	{
		MoveRowAt(RowFromItem(m_dvc_ctrl->GetSelection()), 1);
	}
}

bool BehaviourScriptEditorCtrl::IsRowSelected() const
{
	return m_dvc_ctrl->HasSelection();
}

bool BehaviourScriptEditorCtrl::IsSelTop() const
{
	return RowFromItem(m_dvc_ctrl->GetSelection()) <= 0;
}

bool BehaviourScriptEditorCtrl::IsSelBottom() const
{
	return !m_model || RowFromItem(m_dvc_ctrl->GetSelection()) >= static_cast<int>(m_model->GetRowCount()) - 1;
}

void BehaviourScriptEditorCtrl::UpdateButtonStates()
{
	const bool loaded = m_model != nullptr;
	const bool selected = loaded && IsRowSelected();
	m_append_button->Enable(loaded);
	m_insert_button->Enable(loaded);
	m_delete_button->Enable(selected);
	m_move_up_button->Enable(selected && !IsSelTop());
	m_move_down_button->Enable(selected && !IsSelBottom());
}

void BehaviourScriptEditorCtrl::OnSelectionChange(wxDataViewEvent& evt)
{
	UpdateButtonStates();
	evt.Skip();
}

int BehaviourScriptEditorCtrl::RowFromItem(const wxDataViewItem& item) const
{
	if (!item.IsOk())
	{
		return -1;
	}
	return static_cast<int>(reinterpret_cast<std::intptr_t>(item.GetID()) - 1);
}

wxDataViewItem BehaviourScriptEditorCtrl::ItemFromRow(int row) const
{
	return wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(row + 1)));
}

void BehaviourScriptEditorCtrl::OnContextMenu(wxDataViewEvent& evt)
{
	if (!m_model)
	{
		return;
	}
	const int row = RowFromItem(evt.GetItem());
	if (row < 0)
	{
		return;
	}
	m_dvc_ctrl->Select(evt.GetItem());
	UpdateButtonStates();

	// Unlike ScriptEditorCtrl there are no per-type submenus here: with ~105 command types,
	// Add just inserts a default Pause and the command is picked in the row's own editor.
	wxMenu menu;
	menu.Append(ID_CTX_ADD_ABOVE, "Add Above");
	menu.Append(ID_CTX_ADD_BELOW, "Add Below");
	menu.Append(ID_CTX_DELETE, "Delete");
	menu.AppendSeparator();
	menu.Append(ID_CTX_MOVE_UP, "Move Up")->Enable(row > 0);
	menu.Append(ID_CTX_MOVE_DOWN, "Move Down")->Enable(row < static_cast<int>(m_model->GetRowCount()) - 1);
	menu.AppendSeparator();
	menu.Append(ID_CTX_EDIT, "Edit");

	// If this row starts a cutscene, offer to jump to that cutscene's handler.
	const auto* cmd = m_model->GetCommand(static_cast<unsigned int>(row));
	const std::optional<int> cutscene = cmd ? CutsceneIndexOf(*cmd) : std::nullopt;
	const std::optional<int> playback = cmd ? PlaybackIndexOf(*cmd) : std::nullopt;
	const std::optional<int> trigger = cmd ? TriggerIndexOf(*cmd) : std::nullopt;
	if (cutscene)
	{
		menu.AppendSeparator();
		menu.Append(ID_CTX_OPEN_CUTSCENE,
			wxString::Format("Open Cutscene $%03X", static_cast<unsigned>(*cutscene)));
	}
	if (playback)
	{
		menu.AppendSeparator();
		menu.Append(ID_CTX_OPEN_PLAYBACK,
			wxString::Format("Open Playback Script $%03X", static_cast<unsigned>(*playback)));
	}
	if (trigger)
	{
		menu.AppendSeparator();
		menu.Append(ID_CTX_OPEN_TRIGGER,
			wxString::Format("Open Trigger Action $%02X", static_cast<unsigned>(*trigger)));
	}

	switch (GetPopupMenuSelectionFromUser(menu))
	{
	case ID_CTX_ADD_ABOVE:
		AddRowAt(row, false);
		break;
	case ID_CTX_ADD_BELOW:
		AddRowAt(row, true);
		break;
	case ID_CTX_DELETE:
		DeleteRowAt(row);
		break;
	case ID_CTX_MOVE_UP:
		MoveRowAt(row, -1);
		break;
	case ID_CTX_MOVE_DOWN:
		MoveRowAt(row, 1);
		break;
	case ID_CTX_EDIT:
		EditRowAt(row);
		break;
	case ID_CTX_OPEN_CUTSCENE:
		if (cutscene)
		{
			CutsceneEditorDialog dlg(this, m_gd, *cutscene);
			dlg.ShowModal();
		}
		break;
	case ID_CTX_OPEN_PLAYBACK:
		if (playback)
		{
			InputScriptDialog dlg(this, m_gd, *playback);
			dlg.ShowModal();
		}
		break;
	case ID_CTX_OPEN_TRIGGER:
		if (trigger)
		{
			TriggerActionEditorDialog dlg(this, m_gd, *trigger);
			dlg.ShowModal();
		}
		break;
	default:
		break;
	}
}

void BehaviourScriptEditorCtrl::AddRowAt(int row, bool below)
{
	const int insert_pos = below ? row + 1 : row;
	m_model->AddRow(insert_pos);
	m_dvc_ctrl->Select(ItemFromRow(insert_pos));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	UpdateButtonStates();
}

void BehaviourScriptEditorCtrl::DeleteRowAt(int row)
{
	m_model->DeleteRow(row);
	if (m_model->GetRowCount() > 0)
	{
		const int new_sel = std::min(row, static_cast<int>(m_model->GetRowCount()) - 1);
		m_dvc_ctrl->Select(ItemFromRow(new_sel));
		m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	}
	UpdateButtonStates();
}

void BehaviourScriptEditorCtrl::MoveRowAt(int row, int direction)
{
	const int other = row + direction;
	if (other < 0 || other >= static_cast<int>(m_model->GetRowCount()))
	{
		return;
	}
	m_model->SwapRows(std::min(row, other), std::max(row, other));
	m_dvc_ctrl->Select(ItemFromRow(other));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	UpdateButtonStates();
}

void BehaviourScriptEditorCtrl::EditRowAt(int row)
{
	m_dvc_ctrl->EditItem(ItemFromRow(row), m_dvc_ctrl->GetColumn(1));
}

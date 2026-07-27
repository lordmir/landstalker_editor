#include <script/ScriptEditorCtrl.h>
#include <misc/DataViewModelAssociate.h>
#include <script/ScriptEditorFrame.h>
#include <script/ScriptDataViewRenderer.h>
#include <script/ScriptTableTreeEditorDialog.h>
#include <script/CutsceneEditorDialog.h>

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

#include <algorithm>
#include <array>

namespace
{
// Mirrors the old inline type-change dropdown's order/labels exactly, so the context menu's
// "Change Type" submenu reads the same way it always has.
struct TypeOption
{
	const char* label;
	Landstalker::ScriptTableEntryType type;
};
const std::array<TypeOption, 12> kTypeOptions = { {
	{ "String", Landstalker::ScriptTableEntryType::STRING },
	{ "Set Item", Landstalker::ScriptTableEntryType::ITEM_LOAD },
	{ "Load Global Char", Landstalker::ScriptTableEntryType::GLOBAL_CHAR_LOAD },
	{ "Set Number", Landstalker::ScriptTableEntryType::NUMBER_LOAD },
	{ "Set Flag", Landstalker::ScriptTableEntryType::SET_FLAG },
	{ "Give Item To Player", Landstalker::ScriptTableEntryType::GIVE_ITEM },
	{ "Give Money To Player", Landstalker::ScriptTableEntryType::GIVE_MONEY },
	{ "Play BGM", Landstalker::ScriptTableEntryType::PLAY_BGM },
	{ "Set Character", Landstalker::ScriptTableEntryType::SET_SPEAKER },
	{ "Set Global Char", Landstalker::ScriptTableEntryType::SET_GLOBAL_SPEAKER },
	{ "Play Cutscene", Landstalker::ScriptTableEntryType::PLAY_CUTSCENE },
	{ "Custom", Landstalker::ScriptTableEntryType::INVALID },
} };

enum
{
	ID_CTX_DELETE = wxID_HIGHEST + 600,
	ID_CTX_MOVE_UP,
	ID_CTX_MOVE_DOWN,
	ID_CTX_EDIT,
	ID_CTX_CHANGE_TYPE_BASE,
	ID_CTX_ADD_ABOVE_BASE = ID_CTX_CHANGE_TYPE_BASE + static_cast<int>(kTypeOptions.size()),
	ID_CTX_ADD_BELOW_BASE = ID_CTX_ADD_ABOVE_BASE + static_cast<int>(kTypeOptions.size()),

	ID_KB_INSERT = ID_CTX_ADD_BELOW_BASE + static_cast<int>(kTypeOptions.size()) + 1,
	ID_KB_DELETE,
	ID_KB_MOVE_UP,
	ID_KB_MOVE_DOWN
};
}

wxBEGIN_EVENT_TABLE(ScriptEditorCtrl, wxPanel)
EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, ScriptEditorCtrl::OnSelectionChange)
EVT_DATAVIEW_ITEM_CONTEXT_MENU(wxID_ANY, ScriptEditorCtrl::OnContextMenu)
wxEND_EVENT_TABLE()

ScriptEditorCtrl::ScriptEditorCtrl(wxWindow* parent)
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
	m_append_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { AppendRow(); });
	m_insert_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { InsertRow(); });
	m_delete_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DeleteRow(); });
	m_move_up_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveRowUp(); });
	m_move_down_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveRowDown(); });
	button_sizer->Add(m_append_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_insert_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_delete_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_move_up_button, 0, wxRIGHT, 4);
	button_sizer->Add(m_move_down_button, 0);
	vsizer->Add(button_sizer, 0, wxALL, 4);

	m_dvc_ctrl = new wxDataViewCtrl(this, wxID_ANY);
	vsizer->Add(m_dvc_ctrl, 1, wxALL | wxEXPAND, 5);

	// Hyperlinked cutscene/character names: hover shows a hand cursor, single click opens that
	// entry's script tree popup.
#ifdef __WXGTK__
	// wx-level mouse events are unusable here on GTK: the native GtkTreeView consumes left
	// button presses (selection handling) before wx ever raises wxEVT_LEFT_DOWN, and motion
	// positions arrive in bin-window coordinates while GetItemRect() reports widget coordinates
	// (offset by the header height - the "link hot zone one row too low" bug). Hooking the
	// treeview's own signals gets bin-window coordinates that gtk_tree_view_get_path_at_pos()
	// resolves natively, plus (for clicks) the ability to return TRUE and genuinely consume the
	// press so a link click doesn't also select the row or start a cell edit.
	GtkWidget* treeview = m_dvc_ctrl->GtkGetTreeView();
	g_signal_connect(treeview, "button_press_event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventButton* event, gpointer data) -> gboolean
		{
			if (event->button != 1 || event->type != GDK_BUTTON_PRESS
				|| event->window != gtk_tree_view_get_bin_window(GTK_TREE_VIEW(widget)))
			{
				return FALSE;
			}
			auto* ctrl = static_cast<ScriptEditorCtrl*>(data);
			return ctrl->OnTreeViewLeftDown(static_cast<int>(event->x), static_cast<int>(event->y)) ? TRUE : FALSE;
		}), this);
	g_signal_connect(treeview, "motion_notify_event",
		G_CALLBACK(+[](GtkWidget* widget, GdkEventMotion* event, gpointer data) -> gboolean
		{
			int x = static_cast<int>(event->x);
			int y = static_cast<int>(event->y);
			if (event->is_hint)
			{
				gdk_window_get_device_position(event->window, event->device, &x, &y, nullptr);
			}
			if (event->window == gtk_tree_view_get_bin_window(GTK_TREE_VIEW(widget)))
			{
				static_cast<ScriptEditorCtrl*>(data)->OnTreeViewMotion(x, y);
			}
			return FALSE;
		}), this);
#else
	// Bound on the main window (the row area) - same pattern PaletteListFrame uses for its
	// in-row hover behaviour.
	m_dvc_ctrl->GetMainWindow()->Bind(wxEVT_MOTION, &ScriptEditorCtrl::OnMouseMove, this);
	m_dvc_ctrl->GetMainWindow()->Bind(wxEVT_LEFT_DOWN, &ScriptEditorCtrl::OnLeftDown, this);
#endif

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
}

ScriptEditorCtrl::~ScriptEditorCtrl()
{
}

void ScriptEditorCtrl::SetGameData(std::shared_ptr<Landstalker::GameData> gd, int start, int count)
{
	// Freeze BOTH this panel AND m_dvc_ctrl explicitly - freezing a parent isn't guaranteed to
	// reach a wxDataViewCtrl's own internal GTK treeview on every platform, and that control's own
	// intermediate repaints (during ClearColumns()/InsertColumn() x2/AssociateModel(), each
	// independently triggering their own layout pass) turned out to still be visible even with just
	// the panel frozen. The button row above it (a sibling in the same sizer) reflows in response
	// to each of those, which is what looked like the buttons "taking a moment to load" - Thaw()
	// at the end collapses everything to a single final layout.
	Freeze();
	m_dvc_ctrl->Freeze();
	m_gd = gd;
	m_dvc_ctrl->ClearColumns();
	AssociateDataViewModel(m_dvc_ctrl, nullptr);
	m_model = (start >= 0 && count >= 0)
		? new ScriptDataViewModel(gd, static_cast<unsigned int>(start), static_cast<unsigned int>(count))
		: new ScriptDataViewModel(gd);

	m_model->Initialise();
	AssociateDataViewModel(m_dvc_ctrl, m_model);
	m_model->DecRef();
	m_model->InitControl(m_dvc_ctrl);
	m_dvc_ctrl->SetSelections({});
	UpdateUI();
	m_dvc_ctrl->Thaw();
	Thaw();
}

void ScriptEditorCtrl::ClearGameData()
{
	m_gd.reset();
	m_dvc_ctrl->ClearColumns();
}

void ScriptEditorCtrl::Open(int row)
{
	if (row != -1)
	{
		m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(row + 1))));
		m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	}
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
	m_dvc_ctrl->SetCurrentItem(wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(m_model->GetRowCount()))));
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
		m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(1))));
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
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(sel + 1))));
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

void ScriptEditorCtrl::MoveRowUp()
{
	if (IsRowSelected() && m_model->GetRowCount() >= 2)
	{
		std::size_t sel = reinterpret_cast<std::intptr_t>(m_dvc_ctrl->GetSelection().GetID()) - 1;
		if (sel > 0)
		{
			m_model->SwapRows(sel - 1, sel);
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(sel))));
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
			m_dvc_ctrl->Select(wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(sel + 2))));
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
	UpdateButtonStates();
	// The parent is only a ScriptEditorFrame when hosted in the main editor - the segment popup
	// (ScriptEditorDialog) hosts this control directly in a wxDialog.
	if (auto* frame = dynamic_cast<ScriptEditorFrame*>(GetParent()))
	{
		frame->UpdateUI();
	}
}

void ScriptEditorCtrl::UpdateButtonStates()
{
	const bool loaded = m_gd != nullptr;
	const bool selected = loaded && IsRowSelected();
	m_append_button->Enable(loaded);
	m_insert_button->Enable(loaded);
	m_delete_button->Enable(selected);
	m_move_up_button->Enable(selected && !IsSelTop());
	m_move_down_button->Enable(selected && !IsSelBottom());
}

void ScriptEditorCtrl::OnSelectionChange(wxDataViewEvent& evt)
{
	UpdateUI();
	evt.Skip();
}

int ScriptEditorCtrl::RowFromItem(const wxDataViewItem& item) const
{
	if (!item.IsOk())
	{
		return -1;
	}
	return static_cast<int>(reinterpret_cast<std::intptr_t>(item.GetID()) - 1);
}

wxDataViewItem ScriptEditorCtrl::ItemFromRow(int row) const
{
	return wxDataViewItem(reinterpret_cast<void*>(static_cast<std::intptr_t>(row + 1)));
}

void ScriptEditorCtrl::OnContextMenu(wxDataViewEvent& evt)
{
	if (!m_gd)
	{
		return;
	}
	const int row = RowFromItem(evt.GetItem());
	if (row < 0)
	{
		return;
	}
	m_dvc_ctrl->Select(evt.GetItem());
	UpdateUI();

	wxMenu menu;
	// "Add Above"/"Add Below" are pure submenus (like "Change Type" below), not a single click
	// target - matches the "Change Type" pattern rather than defaulting silently to STRING; the
	// Insert key/toolbar button still do that quick-insert for the common case.
	wxMenu* add_above_menu = new wxMenu();
	wxMenu* add_below_menu = new wxMenu();
	wxMenu* type_menu = new wxMenu();
	for (std::size_t i = 0; i < kTypeOptions.size(); ++i)
	{
		add_above_menu->Append(ID_CTX_ADD_ABOVE_BASE + static_cast<int>(i), kTypeOptions[i].label);
		add_below_menu->Append(ID_CTX_ADD_BELOW_BASE + static_cast<int>(i), kTypeOptions[i].label);
		type_menu->Append(ID_CTX_CHANGE_TYPE_BASE + static_cast<int>(i), kTypeOptions[i].label);
	}
	menu.AppendSubMenu(add_above_menu, "Add Above");
	menu.AppendSubMenu(add_below_menu, "Add Below");
	menu.Append(ID_CTX_DELETE, "Delete");
	menu.AppendSeparator();
	menu.Append(ID_CTX_MOVE_UP, "Move Up")->Enable(row > 0);
	menu.Append(ID_CTX_MOVE_DOWN, "Move Down")->Enable(row < static_cast<int>(m_model->GetRowCount()) - 1);
	menu.AppendSeparator();
	menu.Append(ID_CTX_EDIT, "Edit");
	menu.AppendSubMenu(type_menu, "Change Type");

	const int selection = GetPopupMenuSelectionFromUser(menu);
	if (selection < ID_CTX_DELETE)
	{
		return;
	}

	// Checked highest-base-first since the ID ranges are laid out back-to-back in ascending order
	// (CHANGE_TYPE_BASE < ADD_ABOVE_BASE < ADD_BELOW_BASE) - the first (lowest) range's own bound
	// check would otherwise also swallow selections that actually belong to a later range.
	if (selection >= ID_CTX_ADD_BELOW_BASE)
	{
		const std::size_t idx = static_cast<std::size_t>(selection - ID_CTX_ADD_BELOW_BASE);
		if (idx < kTypeOptions.size())
		{
			AddRowAt(row, true, kTypeOptions[idx].type);
		}
		return;
	}

	if (selection >= ID_CTX_ADD_ABOVE_BASE)
	{
		const std::size_t idx = static_cast<std::size_t>(selection - ID_CTX_ADD_ABOVE_BASE);
		if (idx < kTypeOptions.size())
		{
			AddRowAt(row, false, kTypeOptions[idx].type);
		}
		return;
	}

	if (selection >= ID_CTX_CHANGE_TYPE_BASE)
	{
		const std::size_t idx = static_cast<std::size_t>(selection - ID_CTX_CHANGE_TYPE_BASE);
		if (idx < kTypeOptions.size())
		{
			ChangeRowTypeAt(row, kTypeOptions[idx].type);
		}
		return;
	}

	switch (selection)
	{
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
	default:
		break;
	}
}

void ScriptEditorCtrl::AddRowAt(int row, bool below, Landstalker::ScriptTableEntryType type)
{
	const int insert_pos = below ? row + 1 : row;
	m_model->AddRow(insert_pos, type);
	m_dvc_ctrl->Select(ItemFromRow(insert_pos));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	UpdateUI();
}

void ScriptEditorCtrl::DeleteRowAt(int row)
{
	m_model->DeleteRow(row);
	if (m_model->GetRowCount() > 0)
	{
		const int new_sel = std::min(row, static_cast<int>(m_model->GetRowCount()) - 1);
		m_dvc_ctrl->Select(ItemFromRow(new_sel));
		m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	}
	UpdateUI();
}

void ScriptEditorCtrl::MoveRowAt(int row, int direction)
{
	const int other = row + direction;
	if (other < 0 || other >= static_cast<int>(m_model->GetRowCount()))
	{
		return;
	}
	m_model->SwapRows(std::min(row, other), std::max(row, other));
	m_dvc_ctrl->Select(ItemFromRow(other));
	m_dvc_ctrl->EnsureVisible(m_dvc_ctrl->GetSelection());
	UpdateUI();
}

void ScriptEditorCtrl::EditRowAt(int row)
{
	m_dvc_ctrl->EditItem(ItemFromRow(row), m_dvc_ctrl->GetColumn(1));
}

void ScriptEditorCtrl::ChangeRowTypeAt(int row, Landstalker::ScriptTableEntryType type)
{
	m_model->ChangeRowType(row, type);
	UpdateUI();
}

std::optional<ScriptEntryLink::Target> ScriptEditorCtrl::ResolveLinkForCell(int row, const wxPoint& cell_pos) const
{
	if (!m_gd || !m_model || row < 0 || m_dvc_ctrl->GetColumnCount() < 2)
	{
		return std::nullopt;
	}
	const long line = m_model->ToScriptLine(row);
	auto* renderer = static_cast<ScriptDataViewRenderer*>(m_dvc_ctrl->GetColumn(1)->GetRenderer());
	const auto link_rect = renderer->GetLinkHitRect(line);
	if (!link_rect || !link_rect->Contains(cell_pos))
	{
		return std::nullopt;
	}
	// Re-resolve from the live entry rather than trusting the render-time capture alone - rows
	// can change under a stale hit rect between repaints.
	const auto script = m_gd->GetScriptData()->GetScript();
	if (!script || line >= static_cast<long>(script->GetScriptLineCount()))
	{
		return std::nullopt;
	}
	return ScriptEntryLink::Resolve(m_gd, script->GetScriptLine(line));
}

void ScriptEditorCtrl::SetHandCursor(bool over_link)
{
	if (over_link != m_hand_cursor_active)
	{
		m_hand_cursor_active = over_link;
		m_dvc_ctrl->GetMainWindow()->SetCursor(over_link ? wxCursor(wxCURSOR_HAND) : wxCursor(wxCURSOR_DEFAULT));
	}
}

void ScriptEditorCtrl::OpenLinkPopup(const ScriptEntryLink::Target& link)
{
	// Deferred via CallAfter so the click's own event processing fully unwinds before a modal
	// event loop starts - opening the dialog mid-click leaves the dataview's mouse state stale.
	CallAfter([this, link]()
	{
		// Snapshot the viewport before the refresh - the model Reset() inside RefreshData()
		// scrolls the view back to the top otherwise. Captured as row indices, not items: rows
		// can be deleted by editors nested inside the popup, so both are re-clamped afterwards.
		const int top_row = RowFromItem(m_dvc_ctrl->GetTopItem());
		const int sel_row = RowFromItem(m_dvc_ctrl->GetSelection());

		// A cutscene index resolves to its dialogueactions handler (the real target of the index),
		// not the script-VM cutscene table; a character reference stays on the character tree.
		if (link.is_cutscene)
		{
			CutsceneEditorDialog dlg(this, m_gd, link.entry);
			dlg.ShowModal();
		}
		else
		{
			ScriptTableTreeEditorDialog dlg(this, m_gd, ScriptTableTreeCategory::CHARACTER, link.entry);
			dlg.ShowModal();
		}
		// The popup (or editors nested within it) can change script lines this view displays.
		RefreshData();

		const int count = static_cast<int>(m_model->GetRowCount());
		if (sel_row >= 0 && sel_row < count)
		{
			m_dvc_ctrl->Select(ItemFromRow(sel_row));
		}
		if (top_row >= 0 && count > 0)
		{
			// EnsureVisible() only scrolls the minimum needed, so from the post-reset top the
			// old top row would end up parked at the *bottom* edge of the view. Overshoot to
			// the last row first so the second call approaches from below, which lands the old
			// top row back at the top edge - i.e. the exact viewport the user left. Frozen so
			// the intermediate jump to the bottom never paints.
			m_dvc_ctrl->Freeze();
			m_dvc_ctrl->EnsureVisible(ItemFromRow(count - 1));
			m_dvc_ctrl->EnsureVisible(ItemFromRow(std::min(top_row, count - 1)));
			m_dvc_ctrl->Thaw();
		}
	});
}

#ifdef __WXGTK__

std::optional<ScriptEntryLink::Target> ScriptEditorCtrl::ResolveLinkAtBinPos(int x, int y) const
{
	if (!m_gd || !m_model || m_dvc_ctrl->GetColumnCount() < 2)
	{
		return std::nullopt;
	}
	GtkTreeView* treeview = GTK_TREE_VIEW(m_dvc_ctrl->GtkGetTreeView());
	GtkTreePath* path = nullptr;
	GtkTreeViewColumn* column = nullptr;
	gint cell_x = 0;
	gint cell_y = 0;
	if (!gtk_tree_view_get_path_at_pos(treeview, x, y, &path, &column, &cell_x, &cell_y) || !path)
	{
		return std::nullopt;
	}
	std::optional<ScriptEntryLink::Target> target;
	// The Value column's GTK handle - wx stores the GtkTreeViewColumn pointer as its "handle".
	if (gtk_tree_path_get_depth(path) == 1
		&& reinterpret_cast<GtkWidget*>(column) == m_dvc_ctrl->GetColumn(1)->GetGtkHandle())
	{
		const int row = gtk_tree_path_get_indices(path)[0];
		// cell_x/cell_y from gtk_tree_view_get_path_at_pos() are already cell-relative - the
		// exact space the renderer's hit rects are stored in.
		target = ResolveLinkForCell(row, wxPoint(cell_x, cell_y));
	}
	gtk_tree_path_free(path);
	return target;
}

bool ScriptEditorCtrl::OnTreeViewLeftDown(int x, int y)
{
	const auto target = ResolveLinkAtBinPos(x, y);
	if (!target)
	{
		return false;
	}
	// Returning true consumes the press at the GTK level: a link click must not additionally
	// select the row or (on an already-selected row) start the cell editor beneath the popup.
	OpenLinkPopup(*target);
	return true;
}

void ScriptEditorCtrl::OnTreeViewMotion(int x, int y)
{
	SetHandCursor(ResolveLinkAtBinPos(x, y).has_value());
}

#else

std::optional<ScriptEntryLink::Target> ScriptEditorCtrl::ResolveLinkAt(const wxPoint& pos) const
{
	// The event arrives in the main window's space; HitTest()/GetItemRect() work in the
	// control's own client space.
	const wxPoint ctrl_pos = m_dvc_ctrl->ScreenToClient(m_dvc_ctrl->GetMainWindow()->ClientToScreen(pos));
	wxDataViewItem item;
	wxDataViewColumn* col = nullptr;
	m_dvc_ctrl->HitTest(ctrl_pos, item, col);
	if (!item.IsOk() || m_dvc_ctrl->GetColumnCount() < 2 || col != m_dvc_ctrl->GetColumn(1))
	{
		return std::nullopt;
	}
	const wxRect cell_rect = m_dvc_ctrl->GetItemRect(item, col);
	return ResolveLinkForCell(RowFromItem(item), ctrl_pos - cell_rect.GetTopLeft());
}

void ScriptEditorCtrl::OnMouseMove(wxMouseEvent& evt)
{
	SetHandCursor(ResolveLinkAt(evt.GetPosition()).has_value());
	evt.Skip();
}

void ScriptEditorCtrl::OnLeftDown(wxMouseEvent& evt)
{
	const auto target = ResolveLinkAt(evt.GetPosition());
	if (!target)
	{
		evt.Skip();
		return;
	}
	// Deliberately not skipping: the click is consumed by the link, so it must not additionally
	// select the row or (on an already-selected row) start the cell editor beneath the popup.
	OpenLinkPopup(*target);
}

#endif

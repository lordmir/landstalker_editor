#include <audio/SoundEventListColumn.h>

#include <algorithm>

#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

#include <audio/SoundEventEditDialog.h>

namespace
{
	constexpr int TOOLBAR_BUTTON_SIZE = 24;

	wxButton* MakeToolButton(wxWindow* parent, const wxArtID& art, const wxString& tooltip)
	{
		auto* btn = new wxBitmapButton(parent, wxID_ANY,
			wxArtProvider::GetBitmap(art, wxART_BUTTON, wxSize(16, 16)),
			wxDefaultPosition, wxSize(TOOLBAR_BUTTON_SIZE, TOOLBAR_BUTTON_SIZE));
		btn->SetToolTip(tooltip);
		return btn;
	}
}

SoundEventListColumn::SoundEventListColumn(wxWindow* parent, const wxString& label, SoundEventChannelKind kind,
	GetEventsFn get_events, SetEventsFn set_events)
	: m_kind(kind)
	, m_get(std::move(get_events))
	, m_set(std::move(set_events))
{
	m_box = new wxStaticBoxSizer(wxVERTICAL, parent, label);
	wxWindow* box = m_box->GetStaticBox();

	auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
	m_add_btn = MakeToolButton(box, wxART_PLUS, "Add event...");
	m_add_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowMenuOn(m_add_btn, nullptr); });
	toolbar->Add(m_add_btn, 0, wxRIGHT, 2);
	m_del_btn = MakeToolButton(box, wxART_MINUS, "Delete selected event");
	m_del_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DeleteSelected(); });
	toolbar->Add(m_del_btn, 0, wxRIGHT, 2);
	m_up_btn = MakeToolButton(box, wxART_GO_UP, "Move selected event up");
	m_up_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveSelected(-1); });
	toolbar->Add(m_up_btn, 0, wxRIGHT, 2);
	m_down_btn = MakeToolButton(box, wxART_GO_DOWN, "Move selected event down");
	m_down_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { MoveSelected(1); });
	toolbar->Add(m_down_btn, 0);
	m_box->Add(toolbar, 0, wxALL, 3);

	// wxListCtrl parented directly under a wxStaticBoxSizer's box computes a wildly inflated
	// CalcMin() on this wx/GTK combination (measured ~395px wide for a list whose own
	// GetMinSize()/GetBestSize() both correctly report ~169px) - the static box sizer's min-size
	// pass appears to special-case a scrolling child rather than trusting its reported size.
	// Wrapping the list in a plain wxPanel with its own sizer sidesteps that: the panel's CalcMin()
	// (computed the normal way, from its child's real min size) is what the static box sizer sees.
	auto* list_wrap = new wxPanel(box);
	auto* list_wrap_sizer = new wxBoxSizer(wxVERTICAL);
	m_list = new wxListView(list_wrap, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	m_list->AppendColumn("#", wxLIST_FORMAT_RIGHT, 45);
	m_list->AppendColumn("Event", wxLIST_FORMAT_LEFT, 100);
	m_list->SetMinSize(wxSize(45 + 100 + 24, -1));
	m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &SoundEventListColumn::OnItemActivated, this);
	m_list->Bind(wxEVT_CONTEXT_MENU, &SoundEventListColumn::OnContextMenu, this);
	list_wrap_sizer->Add(m_list, 1, wxEXPAND);
	list_wrap->SetSizer(list_wrap_sizer);
	m_box->Add(list_wrap, 1, wxEXPAND | wxALL, 3);
}

wxSizer* SoundEventListColumn::GetSizer() const
{
	return m_box;
}

wxWindow* SoundEventListColumn::GetBoxWindow() const
{
	return m_box->GetStaticBox();
}

void SoundEventListColumn::SetLabel(const wxString& label)
{
	m_box->GetStaticBox()->SetLabel(label);
}

void SoundEventListColumn::SetChannelKind(SoundEventChannelKind kind)
{
	m_kind = kind;
}

void SoundEventListColumn::Refresh()
{
	const auto events = m_get();
	m_list->Freeze();
	m_list->DeleteAllItems();
	for (std::size_t i = 0; i < events.size(); ++i)
	{
		const long row = m_list->InsertItem(static_cast<long>(i), wxString::Format("%zu", i));
		m_list->SetItem(row, 1, DescribeSoundEvent(m_kind, events[i]));
	}
	m_list->Thaw();
}

bool SoundEventListColumn::HasListFocus() const
{
	return m_list->HasFocus();
}

void SoundEventListColumn::SetFocus()
{
	m_list->SetFocus();
	if (m_list->GetItemCount() > 0 && m_list->GetFirstSelected() < 0)
	{
		SelectRow(0);
	}
}

void SoundEventListColumn::SelectRow(long row)
{
	if (row < 0 || row >= m_list->GetItemCount())
	{
		return;
	}
	m_list->SetItemState(row, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	m_list->EnsureVisible(row);
}

void SoundEventListColumn::InsertDefault(SoundEventInsertKind kind)
{
	auto events = m_get();
	const long sel = m_list->GetFirstSelected();
	const std::size_t pos = (sel >= 0) ? static_cast<std::size_t>(sel) + 1 : events.size();
	events.insert(events.begin() + pos, MakeDefaultSoundEvent(kind));
	m_set(events);
	Refresh();
	SelectRow(static_cast<long>(pos));
}

void SoundEventListColumn::DeleteSelected()
{
	const long sel = m_list->GetFirstSelected();
	if (sel < 0)
	{
		return;
	}
	auto events = m_get();
	if (static_cast<std::size_t>(sel) >= events.size())
	{
		return;
	}
	events.erase(events.begin() + sel);
	m_set(events);
	Refresh();
	if (!events.empty())
	{
		SelectRow(std::min<long>(sel, static_cast<long>(events.size()) - 1));
	}
}

void SoundEventListColumn::MoveSelected(int direction)
{
	const long sel = m_list->GetFirstSelected();
	if (sel < 0)
	{
		return;
	}
	auto events = m_get();
	const long target = sel + direction;
	if (target < 0 || target >= static_cast<long>(events.size()))
	{
		return;
	}
	std::swap(events[sel], events[target]);
	m_set(events);
	Refresh();
	SelectRow(target);
}

void SoundEventListColumn::EditSelected()
{
	const long sel = m_list->GetFirstSelected();
	if (sel < 0)
	{
		return;
	}
	auto events = m_get();
	if (static_cast<std::size_t>(sel) >= events.size())
	{
		return;
	}
	SoundEventEditDialog dlg(m_list, m_kind, events[sel]);
	if (dlg.ShowModal() == wxID_OK)
	{
		events[sel] = dlg.GetResult();
		m_set(events);
		Refresh();
		SelectRow(sel);
	}
}

void SoundEventListColumn::BuildInsertMenu(wxMenu& menu, std::vector<SoundEventInsertKind>& kinds_out) const
{
	auto add_item = [&](wxMenu& target, SoundEventInsertKind kind)
	{
		const int id = wxID_HIGHEST + 1 + static_cast<int>(kinds_out.size());
		kinds_out.push_back(kind);
		target.Append(id, SoundEventInsertKindLabel(kind));
	};
	for (auto kind : TopLevelInsertKinds())
	{
		add_item(menu, kind);
	}
	auto* loop_menu = new wxMenu();
	for (auto kind : LoopInsertKinds())
	{
		add_item(*loop_menu, kind);
	}
	menu.AppendSubMenu(loop_menu, "Loop");
	auto* end_menu = new wxMenu();
	for (auto kind : EndInsertKinds())
	{
		add_item(*end_menu, kind);
	}
	menu.AppendSubMenu(end_menu, "End / Jump");
}

void SoundEventListColumn::ShowMenuOn(wxWindow* window, const wxPoint* client_pos)
{
	wxMenu menu;
	std::vector<SoundEventInsertKind> kinds;
	BuildInsertMenu(menu, kinds);
	menu.Bind(wxEVT_MENU, [this, &kinds](wxCommandEvent& evt)
	{
		const std::size_t idx = static_cast<std::size_t>(evt.GetId() - (wxID_HIGHEST + 1));
		if (idx < kinds.size())
		{
			InsertDefault(kinds[idx]);
		}
	});
	if (client_pos)
	{
		window->PopupMenu(&menu, *client_pos);
	}
	else
	{
		window->PopupMenu(&menu);
	}
}

void SoundEventListColumn::OnItemActivated(wxListEvent&)
{
	EditSelected();
}

void SoundEventListColumn::OnContextMenu(wxContextMenuEvent& evt)
{
	wxPoint pos = evt.GetPosition();
	if (pos == wxDefaultPosition)
	{
		pos = m_list->GetScreenPosition() + wxPoint(10, 10);
	}
	const wxPoint client_pos = m_list->ScreenToClient(pos);

	wxMenu menu;
	const long sel = m_list->GetFirstSelected();
	const bool have_selection = sel >= 0;
	wxMenuItem* edit_item = menu.Append(wxID_EDIT, "Edit...");
	edit_item->Enable(have_selection);
	wxMenuItem* delete_item = menu.Append(wxID_DELETE, "Delete");
	delete_item->Enable(have_selection);
	wxMenuItem* up_item = menu.Append(wxID_UP, "Move Up");
	up_item->Enable(have_selection && sel > 0);
	wxMenuItem* down_item = menu.Append(wxID_DOWN, "Move Down");
	down_item->Enable(have_selection && sel + 1 < m_list->GetItemCount());
	menu.AppendSeparator();

	std::vector<SoundEventInsertKind> kinds;
	BuildInsertMenu(menu, kinds);

	menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) { EditSelected(); }, wxID_EDIT);
	menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) { DeleteSelected(); }, wxID_DELETE);
	menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) { MoveSelected(-1); }, wxID_UP);
	menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) { MoveSelected(1); }, wxID_DOWN);
	menu.Bind(wxEVT_MENU, [this, &kinds](wxCommandEvent& e)
	{
		const std::size_t idx = static_cast<std::size_t>(e.GetId() - (wxID_HIGHEST + 1));
		if (idx < kinds.size())
		{
			InsertDefault(kinds[idx]);
		}
	});

	m_list->PopupMenu(&menu, client_pos);
}

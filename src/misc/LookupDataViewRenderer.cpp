#include <misc/LookupDataViewRenderer.h>

#include <algorithm>
#include <optional>
#include <vector>
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/popupwin.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

namespace
{
bool IsDescendantOf(wxWindow* child, const wxWindow* ancestor)
{
	for (wxWindow* w = child; w; w = w->GetParent())
	{
		if (w == ancestor)
		{
			return true;
		}
	}
	return false;
}

#ifdef __WXGTK__
class LookupPopupWindow : public wxPopupTransientWindow
#else
// Windows path uses non-transient popup to avoid premature dismissal while editing.
class LookupPopupWindow : public wxPopupWindow
#endif
{
public:
	explicit LookupPopupWindow(wxWindow* parent)
		#ifdef __WXGTK__
		// GTK requires transient popup semantics for correct focus lifecycle.
		: wxPopupTransientWindow(parent, wxBORDER_SIMPLE | wxPU_CONTAINS_CONTROLS),
		#else
		: wxPopupWindow(parent, wxBORDER_SIMPLE),
		#endif
		  m_list(new wxListBox(this, wxID_ANY))
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(m_list, 1, wxEXPAND);
		SetSizerAndFit(sizer);
	}

	wxListBox* GetList() const
	{
		return m_list;
	}

private:
	wxListBox* m_list;
};

class LookupEditorControl : public wxPanel
{
public:
	LookupEditorControl(wxWindow* parent, const wxRect& rect, const wxString& value, const wxArrayString& choices, wxDataViewRenderer* renderer)
		: wxPanel(parent, wxID_ANY, rect.GetPosition(), rect.GetSize()),
		  m_choices(choices),
		  m_renderer(renderer),
		  m_text(new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER)),
		  m_drop_btn(new wxBitmapButton(this, wxID_ANY,
			wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_BUTTON, wxSize(16, 16)),
			wxDefaultPosition,
#ifdef __WXGTK__
			wxSize(30, -1),
#else
			wxSize(24, -1),
#endif
			wxBU_EXACTFIT)),
		  m_popup(new LookupPopupWindow(this)),
		  m_committed_value(value)
	{
		SetMinSize(rect.GetSize());
		auto* sizer = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(m_text, 1, wxEXPAND);
		sizer->Add(m_drop_btn, 0, wxEXPAND);
		SetSizer(sizer);
		SetAutoLayout(true);
		Layout();

		m_text->Bind(wxEVT_TEXT, &LookupEditorControl::OnText, this);
		#ifdef __WXGTK__
		// GTK delivers list/editor key input differently, so use KEY_DOWN handlers there.
		m_text->Bind(wxEVT_KEY_DOWN, &LookupEditorControl::OnTextKeyDown, this);
		#else
		m_text->Bind(wxEVT_CHAR_HOOK, &LookupEditorControl::OnTextKeyDown, this);
		#endif
		m_text->Bind(wxEVT_TEXT_ENTER, &LookupEditorControl::OnTextEnter, this);
		m_text->Bind(wxEVT_LEFT_DOWN, &LookupEditorControl::OnTextLeftDown, this);
		m_text->Bind(wxEVT_SET_FOCUS, &LookupEditorControl::OnTextFocus, this);
		#ifdef __WXGTK__
		m_text->Bind(wxEVT_KILL_FOCUS, &LookupEditorControl::OnControlKillFocus, this);
		#endif
		m_drop_btn->Bind(wxEVT_BUTTON, &LookupEditorControl::OnDropDownClick, this);
		#ifdef __WXGTK__
		m_drop_btn->Bind(wxEVT_KILL_FOCUS, &LookupEditorControl::OnControlKillFocus, this);
		#endif
		Bind(wxEVT_SIZE, &LookupEditorControl::OnSize, this);
		#ifdef __WXGTK__
		Bind(wxEVT_KILL_FOCUS, &LookupEditorControl::OnControlKillFocus, this);
		#endif

		m_popup->GetList()->Bind(wxEVT_LISTBOX, &LookupEditorControl::OnListSelect, this);
		m_popup->GetList()->Bind(wxEVT_LISTBOX_DCLICK, &LookupEditorControl::OnListActivate, this);
		m_popup->GetList()->Bind(wxEVT_LEFT_DOWN, &LookupEditorControl::OnListLeftDown, this);
		m_popup->GetList()->Bind(wxEVT_LEFT_UP, &LookupEditorControl::OnListLeftUp, this);
		m_popup->GetList()->Bind(wxEVT_MOTION, &LookupEditorControl::OnListMouseMove, this);
		#ifdef __WXGTK__
		m_popup->GetList()->Bind(wxEVT_KEY_DOWN, &LookupEditorControl::OnListKeyDown, this);
		m_popup->GetList()->Bind(wxEVT_SET_FOCUS, &LookupEditorControl::OnListFocus, this);
		m_popup->GetList()->Bind(wxEVT_KILL_FOCUS, &LookupEditorControl::OnControlKillFocus, this);
		#else
		m_popup->GetList()->Bind(wxEVT_CHAR_HOOK, &LookupEditorControl::OnListKeyDown, this);
		#endif

		UpdateFilteredItems();
		m_text->SetFocus();
		m_text->SetInsertionPointEnd();
	}

	~LookupEditorControl() override
	{
		if (m_popup && m_popup->IsShown())
		{
			m_popup->Hide();
		}
	}

	wxString GetValueText() const
	{
		if (m_pending_value)
		{
			return *m_pending_value;
		}
		return m_text->GetValue();
	}

	void SetTextColour(const wxColour& colour)
	{
		m_text->SetForegroundColour(colour);
		m_text->Refresh();
	}

	void CommitPendingSelection()
	{
		auto* list = m_popup->GetList();
		if (m_pending_value)
		{
			AcceptValue(*m_pending_value);
		}
		else if (m_popup->IsShown() && list->GetSelection() != wxNOT_FOUND)
		{
			AcceptSelected();
		}
	}

private:
	int FindChoiceIndex(const wxString& value) const
	{
		const wxString lower = value.Lower();
		for (std::size_t i = 0; i < m_choices.GetCount(); ++i)
		{
			if (m_choices[i].Lower() == lower)
			{
				return static_cast<int>(i);
			}
		}
		return wxNOT_FOUND;
	}

	bool IsCurrentValueValidChoice() const
	{
		return FindChoiceIndex(m_text->GetValue()) != wxNOT_FOUND;
	}

	void RestoreCommittedValue()
	{
		m_text->ChangeValue(m_committed_value);
		m_text->SetInsertionPointEnd();
		m_pending_value.reset();
	}

	void UpdateFilteredItems(bool show_all = false, bool auto_select = true)
	{
		const wxString needle = show_all ? wxString() : m_text->GetValue().Lower();
		const wxString current_value = m_text->GetValue().Lower();
		auto* list = m_popup->GetList();
		m_updating_list = true;
		m_pending_value.reset();
		list->Freeze();
		list->Clear();
		m_filtered_indices.clear();
		int selected_row = wxNOT_FOUND;

		for (std::size_t i = 0; i < m_choices.GetCount(); ++i)
		{
			const wxString& choice = m_choices[i];
			if (needle.IsEmpty() || choice.Lower().Find(needle) != wxNOT_FOUND)
			{
				m_filtered_indices.push_back(static_cast<int>(i));
				list->Append(choice);
				if (selected_row == wxNOT_FOUND && !current_value.IsEmpty() && choice.Lower() == current_value)
				{
					selected_row = static_cast<int>(list->GetCount()) - 1;
				}
			}
		}

		if (auto_select && list->GetCount() > 0)
		{
			if (selected_row == wxNOT_FOUND)
			{
				selected_row = 0;
			}
			list->SetSelection(selected_row);
			list->EnsureVisible(selected_row);
		}
		else
		{
			const int sel = list->GetSelection();
			if (sel != wxNOT_FOUND)
			{
				list->Deselect(sel);
			}
		}

		m_updating_list = false;
		list->Thaw();
	}

	void ShowPopup()
	{
		auto* list = m_popup->GetList();
		if (list->GetCount() <= 0)
		{
			return;
		}

		const int row_height = std::max(list->GetCharHeight() + 6, 18);
		const int visible_rows = std::min(12, static_cast<int>(list->GetCount()));
		const int popup_height = row_height * visible_rows + 8;
		int popup_width = std::max(GetSize().GetWidth(), 300);

		const wxPoint screen_pt = ClientToScreen(wxPoint(0, GetSize().GetHeight()));
		// Clamp to the containing top-level window's right edge - the 300 floor above is sized for
		// a whole dataview column, but a combo embedded in a narrower composite editor (alongside
		// other fields) can sit close enough to the window's edge that an unclamped popup would
		// spill out over whatever's next to it (e.g. the dataview's own scrollbar).
		if (wxWindow* top = wxGetTopLevelParent(this))
		{
			const int right_edge = top->GetScreenRect().GetRight();
			if (screen_pt.x + popup_width > right_edge)
			{
				popup_width = std::max(GetSize().GetWidth(), right_edge - screen_pt.x - 4);
			}
		}
		m_popup->SetSize(screen_pt.x, screen_pt.y, popup_width, popup_height);
		if (!m_popup->IsShown())
		{
			#ifdef __WXGTK__
			m_popup->Popup(m_text);
			#else
			m_popup->Show();
			#endif
		}

		#ifdef __WXGTK__
		CallAfter([this]()
		{
			wxWindow* focus = wxWindow::FindFocus();
			if (IsDescendantOf(focus, m_popup) && focus != m_text)
			{
				m_text->SetFocus();
				m_text->SetInsertionPointEnd();
			}
		});
		#endif

		const int selected = list->GetSelection();
		if (selected != wxNOT_FOUND)
		{
			list->EnsureVisible(selected);
		}
	}

	void HidePopup()
	{
		if (m_popup->IsShown())
		{
			#ifdef __WXGTK__
			m_popup->Dismiss();
			#else
			m_popup->Hide();
			#endif
		}
	}

	void AcceptSelected()
	{
		auto* list = m_popup->GetList();
		const int sel = list->GetSelection();
		if (sel != wxNOT_FOUND && sel >= 0 && sel < static_cast<int>(list->GetCount()))
		{
			AcceptValue(list->GetString(sel));
		}
		HidePopup();
	}

	void AcceptValue(const wxString& value)
	{
		m_committed_value = value;
		m_text->ChangeValue(m_committed_value);
		m_text->SetInsertionPointEnd();
		m_text->SetSelection(m_text->GetLastPosition(), m_text->GetLastPosition());
		m_pending_value.reset();
	}

	void MoveSelection(int delta)
	{
		auto* list = m_popup->GetList();
		if (list->GetCount() <= 0)
		{
			return;
		}

		int sel = list->GetSelection();
		if (sel == wxNOT_FOUND)
		{
			sel = 0;
		}
		else
		{
			sel = std::clamp(sel + delta, 0, static_cast<int>(list->GetCount()) - 1);
		}

		list->SetSelection(sel);
		list->EnsureVisible(sel);
		RememberPendingListSelection();
	}

	void RememberPendingListSelection()
	{
		auto* list = m_popup->GetList();
		const int sel = list->GetSelection();
		if (!m_updating_list && sel != wxNOT_FOUND && sel >= 0 && sel < static_cast<int>(list->GetCount()))
		{
			m_pending_value = list->GetString(sel);
		}
	}

	void OnText(wxCommandEvent& evt)
	{
		#ifdef __WXGTK__
		UpdateFilteredItems(false, false);
		#else
		UpdateFilteredItems();
		#endif
		if (m_popup->GetList()->GetCount() > 0)
		{
			ShowPopup();
		}
		else if (m_popup->IsShown())
		{
			HidePopup();
		}

		#ifdef __WXGTK__
		if (wxWindow::FindFocus() == m_text)
		{
			const long pos = m_text->GetInsertionPoint();
			m_text->SetSelection(pos, pos);
		}
		#endif
		evt.Skip();
	}

	void OnTextLeftDown(wxMouseEvent& evt)
	{
		evt.Skip();
		if (!m_popup->IsShown() && m_popup->GetList()->GetCount() > 0)
		{
			ShowPopup();
		}
		m_select_all_on_focus = false;
		m_text->CallAfter([text = m_text]()
		{
			if (text)
			{
				text->SelectAll();
			}
		});
	}

	void OnTextFocus(wxFocusEvent& evt)
	{
		evt.Skip();
		#ifdef __WXGTK__
		if (!m_select_all_on_focus)
		{
			return;
		}
		m_select_all_on_focus = false;
		#endif
		m_text->CallAfter([text = m_text]()
		{
			if (text)
			{
				text->SelectAll();
			}
		});
	}

	void OnSize(wxSizeEvent& evt)
	{
		Layout();
		evt.Skip();
	}

	void OnControlKillFocus(wxFocusEvent& evt)
	{
		#ifdef __WXGTK__
		evt.Skip();
		CallAfter([this]()
		{
			wxWindow* focus = wxWindow::FindFocus();
			if (IsDescendantOf(focus, this) || IsDescendantOf(focus, m_popup))
			{
				return;
			}

			if (!IsCurrentValueValidChoice())
			{
				RestoreCommittedValue();
			}

			HidePopup();
		});
		#else
		evt.Skip();
		CallAfter([this]()
		{
			wxWindow* focus = wxWindow::FindFocus();
			if (IsDescendantOf(focus, this) || IsDescendantOf(focus, m_popup))
			{
				return;
			}

			if (!IsCurrentValueValidChoice())
			{
				RestoreCommittedValue();
			}

			HidePopup();
		});
		#endif
	}

	void OnTextKeyDown(wxKeyEvent& evt)
	{
		switch (evt.GetKeyCode())
		{
		case WXK_DOWN:
			if (!m_popup->IsShown())
			{
				ShowPopup();
			}
			else
			{
				MoveSelection(+1);
			}
			return;
		case WXK_UP:
			if (m_popup->IsShown())
			{
				MoveSelection(-1);
				return;
			}
			break;
		case WXK_ESCAPE:
			HidePopup();
			RestoreCommittedValue();
			// Reverting the displayed text isn't enough on its own - without this, the cell just
			// stays in edit mode showing the (now reverted) value instead of actually closing, so
			// Escape looked like it didn't do anything until the user clicked away separately.
			if (m_renderer)
			{
				m_renderer->CancelEditing();
			}
			return;
		case WXK_RETURN:
		case WXK_NUMPAD_ENTER:
			if (m_popup->IsShown())
			{
				AcceptSelected();
			}
			if (GetParent())
			{
				GetParent()->SetFocus();
			}
			return;
		case WXK_TAB:
			if (m_popup->IsShown())
			{
				AcceptSelected();
				evt.Skip();
				return;
			}
			break;
		case WXK_RIGHT:
			if (m_popup->IsShown())
			{
				AcceptSelected();
				return;
			}
			break;
		default:
			break;
		}

		evt.Skip();
	}

	void OnTextEnter(wxCommandEvent& evt)
	{
		if (m_popup->IsShown())
		{
			AcceptSelected();
		}
		if (GetParent())
		{
			GetParent()->SetFocus();
		}
		evt.Skip(false);
	}

	void OnListSelect(wxCommandEvent& evt)
	{
		#ifdef __WXGTK__
		// GTK can emit selection changes during list rebuild; only explicit actions should commit.
		// Selection changes can be triggered programmatically on GTK while filtering.
		// Commit only on explicit user actions (enter/double-click/click release).
		RememberPendingListSelection();
		#else
		AcceptSelected();
		#endif
		evt.Skip();
	}

	void OnListActivate(wxCommandEvent& evt)
	{
		AcceptSelected();
		evt.Skip();
	}

	void OnListLeftDown(wxMouseEvent& evt)
	{
		auto* list = m_popup->GetList();
		#ifdef __WXGTK__
		const int hit = list->HitTest(evt.GetPosition());
		if (hit != wxNOT_FOUND && hit >= 0 && hit < static_cast<int>(list->GetCount()))
		{
			list->SetSelection(hit);
			RememberPendingListSelection();
			m_list_click_candidate = hit;
			return;
		}

		m_list_click_candidate = wxNOT_FOUND;
		evt.Skip();
		#else
		const int hit = list->HitTest(evt.GetPosition());
		if (hit != wxNOT_FOUND && hit >= 0 && hit < static_cast<int>(list->GetCount()))
		{
			list->SetSelection(hit);
			RememberPendingListSelection();
			AcceptSelected();
			m_text->SetFocus();
			return;
		}
		evt.Skip();
		#endif
	}

	void OnListLeftUp(wxMouseEvent& evt)
	{
		#ifdef __WXGTK__
		auto* list = m_popup->GetList();
		const int hit = list->HitTest(evt.GetPosition());
		if (m_list_click_candidate != wxNOT_FOUND && hit == m_list_click_candidate &&
			hit >= 0 && hit < static_cast<int>(list->GetCount()))
		{
			list->SetSelection(hit);
			RememberPendingListSelection();
			AcceptSelected();
			m_text->SetFocus();
			m_list_click_candidate = wxNOT_FOUND;
			return;
		}

		m_list_click_candidate = wxNOT_FOUND;
		evt.Skip();
		#else
		evt.Skip();
		CallAfter([this]()
		{
			if (!m_popup || !m_popup->IsShown())
			{
				return;
			}
			if (m_popup->GetList()->GetSelection() != wxNOT_FOUND)
			{
				AcceptSelected();
				m_text->SetFocus();
			}
		});
		#endif
	}

	void OnListMouseMove(wxMouseEvent& evt)
	{
		#ifdef __WXGTK__
		if (wxWindow::FindFocus() == m_text)
		{
			evt.Skip();
			return;
		}
		#endif

		auto* list = m_popup->GetList();
		const int hit = list->HitTest(evt.GetPosition());
		if (hit != wxNOT_FOUND && hit >= 0 && hit < static_cast<int>(list->GetCount()))
		{
			if (list->GetSelection() != hit)
			{
				list->SetSelection(hit);
				RememberPendingListSelection();
			}
		}
		evt.Skip();
	}

	void OnListFocus(wxFocusEvent& evt)
	{
		#ifdef __WXGTK__
		evt.Skip();
		CallAfter([this]()
		{
			if (m_text)
			{
				m_text->SetFocus();
				m_text->SetInsertionPointEnd();
				const long pos = m_text->GetInsertionPoint();
				m_text->SetSelection(pos, pos);
			}
		});
		#else
		evt.Skip();
		#endif
	}

	void OnListKeyDown(wxKeyEvent& evt)
	{
		#ifdef __WXGTK__
		if (evt.GetKeyCode() == WXK_DOWN)
		{
			MoveSelection(+1);
			return;
		}
		if (evt.GetKeyCode() == WXK_UP)
		{
			MoveSelection(-1);
			return;
		}
		if (evt.GetKeyCode() == WXK_ESCAPE)
		{
			HidePopup();
			m_text->SetFocus();
			return;
		}
		if (evt.GetKeyCode() == WXK_RETURN || evt.GetKeyCode() == WXK_NUMPAD_ENTER)
		{
			AcceptSelected();
			m_text->SetFocus();
			return;
		}
		if (evt.GetKeyCode() == WXK_TAB || evt.GetKeyCode() == WXK_RIGHT)
		{
			AcceptSelected();
			m_text->SetFocus();
			if (evt.GetKeyCode() == WXK_TAB)
			{
				evt.Skip();
			}
			return;
		}

		// If GTK routes printable/edit keys to the list, replay into text control.
		if (m_text)
		{
			m_text->SetFocus();
			m_text->SetInsertionPointEnd();
			const long pos = m_text->GetInsertionPoint();
			m_text->SetSelection(pos, pos);
			m_text->EmulateKeyPress(evt);
			return;
		}
		#endif
		evt.Skip();
	}

	void OnDropDownClick(wxCommandEvent& evt)
	{
		if (m_popup->IsShown())
		{
			HidePopup();
			m_select_all_on_focus = false;
			m_text->SetFocus();
			m_text->SelectAll();
			return;
		}

		UpdateFilteredItems(true);
		if (m_popup->GetList()->GetCount() <= 0)
		{
			HidePopup();
			m_select_all_on_focus = false;
			m_text->SetFocus();
			m_text->SelectAll();
			return;
		}

		ShowPopup();
		m_select_all_on_focus = false;
		m_text->SetFocus();
		m_text->SelectAll();
		evt.Skip(false);
	}

	wxArrayString m_choices;
	std::vector<int> m_filtered_indices;
	wxDataViewRenderer* m_renderer;
	wxTextCtrl* m_text;
	wxBitmapButton* m_drop_btn;
	LookupPopupWindow* m_popup;
	wxString m_committed_value;
	std::optional<wxString> m_pending_value;
	int m_list_click_candidate = wxNOT_FOUND;
	bool m_updating_list = false;
	bool m_select_all_on_focus = true;
};
}

namespace LookupEditor
{
wxWindow* Create(wxWindow* parent, const wxRect& rect, const wxString& value,
                  const wxArrayString& choices, wxDataViewRenderer* renderer)
{
	return new LookupEditorControl(parent, rect, value, choices, renderer);
}

void CommitPendingSelection(wxWindow* ctrl)
{
	if (auto* editor = dynamic_cast<LookupEditorControl*>(ctrl))
	{
		editor->CommitPendingSelection();
	}
}

wxString GetValueText(wxWindow* ctrl)
{
	if (auto* editor = dynamic_cast<LookupEditorControl*>(ctrl))
	{
		return editor->GetValueText();
	}
	return wxString();
}

wxString FormatLabel(const wxArrayString& choices, long index)
{
	if (index >= 0 && index < static_cast<long>(choices.GetCount()))
	{
		return choices[static_cast<std::size_t>(index)];
	}
	return wxString::Format("[%04ld] ???", index);
}

long ParseValue(const wxArrayString& choices, const wxString& text, long fallback)
{
	wxString t = text;
	t.Trim(true);
	t.Trim(false);

	if (t.empty())
	{
		return fallback;
	}

	if (t.StartsWith("["))
	{
		const int close = t.Find(']');
		if (close != wxNOT_FOUND)
		{
			const wxString idx = t.SubString(1, close - 1);
			// Match the bracketed id against each choice's own prefix before trying to read
			// it as a number. The lists are not all in the same base - character ids are
			// formatted as hex, flags and slots as decimal - so parsing the id as decimal
			// either fails outright on a digit like A or, worse, silently resolves "[010]"
			// to entry 10 when the label means 0x10.
			for (std::size_t i = 0; i < choices.GetCount(); ++i)
			{
				const wxString& choice = choices[i];
				const int choice_close = choice.StartsWith("[") ? choice.Find(']') : wxNOT_FOUND;
				if (choice_close != wxNOT_FOUND &&
					choice.SubString(1, choice_close - 1).CmpNoCase(idx) == 0)
				{
					return static_cast<long>(i);
				}
			}
			long parsed = 0;
			if (idx.ToLong(&parsed) && parsed >= 0 && parsed < static_cast<long>(choices.GetCount()))
			{
				return parsed;
			}
			// Last resort, for an id typed without the label's leading zeros.
			if (idx.ToLong(&parsed, 16) && parsed >= 0 && parsed < static_cast<long>(choices.GetCount()))
			{
				return parsed;
			}
		}
	}

	long parsed = 0;
	if (t.ToLong(&parsed) && parsed >= 0 && parsed < static_cast<long>(choices.GetCount()))
	{
		return parsed;
	}

	const wxString lower = t.Lower();
	for (std::size_t i = 0; i < choices.GetCount(); ++i)
	{
		if (choices[i].Lower() == lower)
		{
			return static_cast<long>(i);
		}
	}

	for (std::size_t i = 0; i < choices.GetCount(); ++i)
	{
		if (choices[i].Lower().Find(lower) != wxNOT_FOUND)
		{
			return static_cast<long>(i);
		}
	}

	return fallback;
}

void SetTextColour(wxWindow* ctrl, const wxColour& colour)
{
	if (auto* editor = dynamic_cast<LookupEditorControl*>(ctrl))
	{
		editor->SetTextColour(colour);
	}
}
}

LookupDataViewRenderer::LookupDataViewRenderer(wxDataViewCellMode mode, wxArrayString choices)
	: wxDataViewCustomRenderer("long", mode, wxALIGN_LEFT),
	  m_choices(std::move(choices)),
	  m_value(0),
	  m_size_cached(false),
	  m_cached_size(80, 18)
{
}

bool LookupDataViewRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	RenderText(FormatLabel(m_value), 2, rect, dc, state);
	return true;
}

bool LookupDataViewRenderer::ActivateCell(const wxRect& /*cell*/, wxDataViewModel* /*model*/, const wxDataViewItem& /*item*/,
	unsigned int /*col*/, const wxMouseEvent* /*mouseEvent*/)
{
	return false;
}

wxSize LookupDataViewRenderer::GetSize() const
{
	if (!m_size_cached)
	{
		int max_width = 0;
		int max_height = 0;

		if (m_choices.IsEmpty())
		{
			const wxSize sz = GetTextExtent("[0000] ???");
			max_width = sz.GetWidth();
			max_height = sz.GetHeight();
		}
		else
		{
			for (const auto& choice : m_choices)
			{
				const wxSize sz = GetTextExtent(choice);
				max_width = std::max(max_width, sz.GetWidth());
				max_height = std::max(max_height, sz.GetHeight());
			}
		}

		m_cached_size = { std::max(max_width + 6, 80), std::max(max_height + 2, 18) };
		m_size_cached = true;
	}

	return m_cached_size;
}

bool LookupDataViewRenderer::SetValue(const wxVariant& value)
{
	m_value = value.GetLong();
	return true;
}

bool LookupDataViewRenderer::GetValue(wxVariant& value) const
{
	value = m_value;
	return true;
}

bool LookupDataViewRenderer::HasEditorCtrl() const
{
	return true;
}

wxWindow* LookupDataViewRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	m_value = value.GetLong();
	return new LookupEditorControl(parent, labelRect, FormatLabel(m_value), m_choices, this);
}

bool LookupDataViewRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
	auto* editor = dynamic_cast<LookupEditorControl*>(ctrl);
	if (!editor)
	{
		return false;
	}
	editor->CommitPendingSelection();
	value = ParseValue(editor->GetValueText());
	return true;
}

wxString LookupDataViewRenderer::FormatLabel(long index) const
{
	if (IsValidIndex(index))
	{
		return m_choices[static_cast<std::size_t>(index)];
	}
	return wxString::Format("[%04ld] ???", index);
}

long LookupDataViewRenderer::ParseValue(const wxString& text) const
{
	wxString t = text;
	t.Trim(true);
	t.Trim(false);

	if (t.empty())
	{
		return m_value;
	}

	if (t.StartsWith("["))
	{
		const int close = t.Find(']');
		if (close != wxNOT_FOUND)
		{
			wxString idx = t.SubString(1, close - 1);
			long parsed = 0;
			if (idx.ToLong(&parsed) && IsValidIndex(parsed))
			{
				return parsed;
			}
		}
	}

	long parsed = 0;
	if (t.ToLong(&parsed) && IsValidIndex(parsed))
	{
		return parsed;
	}

	const wxString lower = t.Lower();
	for (std::size_t i = 0; i < m_choices.GetCount(); ++i)
	{
		if (m_choices[i].Lower() == lower)
		{
			return static_cast<long>(i);
		}
	}

	for (std::size_t i = 0; i < m_choices.GetCount(); ++i)
	{
		if (m_choices[i].Lower().Find(lower) != wxNOT_FOUND)
		{
			return static_cast<long>(i);
		}
	}

	return m_value;
}

bool LookupDataViewRenderer::IsValidIndex(long index) const
{
	return index >= 0 && index < static_cast<long>(m_choices.GetCount());
}

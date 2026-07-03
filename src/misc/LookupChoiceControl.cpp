#include <misc/LookupChoiceControl.h>

#include <algorithm>
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/event.h>
#include <wx/listbox.h>
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
}

LookupChoiceControl::LookupChoiceControl(wxWindow* parent, wxWindowID id, const wxString& value, const wxArrayString& choices,
    const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size),
      m_choices(choices),
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
#ifdef __WXGTK__
      // GTK needs a transient popup with focus-aware dismissal handling.
      m_popup(new wxPopupTransientWindow(this, wxBORDER_SIMPLE | wxPU_CONTAINS_CONTROLS)),
#else
      // Windows is more reliable with a regular popup and explicit Show/Hide.
      m_popup(new wxPopupWindow(this, wxBORDER_SIMPLE)),
#endif
      m_list(new wxListBox(m_popup, wxID_ANY)),
      m_committed_value(value)
{
    m_selection = FindChoiceIndex(value);

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_text, 1, wxEXPAND);
    sizer->Add(m_drop_btn, 0, wxEXPAND);
    SetSizer(sizer);

    auto* popup_sizer = new wxBoxSizer(wxVERTICAL);
    popup_sizer->Add(m_list, 1, wxEXPAND);
    m_popup->SetSizer(popup_sizer);

    m_text->Bind(wxEVT_TEXT, &LookupChoiceControl::OnText, this);
#ifdef __WXGTK__
    // GTK routes key events differently in popup/list contexts.
    m_text->Bind(wxEVT_KEY_DOWN, &LookupChoiceControl::OnTextKeyDown, this);
#else
    m_text->Bind(wxEVT_CHAR_HOOK, &LookupChoiceControl::OnTextKeyDown, this);
#endif
    m_text->Bind(wxEVT_TEXT_ENTER, &LookupChoiceControl::OnTextEnter, this);
    m_text->Bind(wxEVT_LEFT_DOWN, &LookupChoiceControl::OnTextLeftDown, this);
    m_text->Bind(wxEVT_SET_FOCUS, &LookupChoiceControl::OnTextFocus, this);
    m_text->Bind(wxEVT_KILL_FOCUS, &LookupChoiceControl::OnControlKillFocus, this);
    m_drop_btn->Bind(wxEVT_BUTTON, &LookupChoiceControl::OnDropDownClick, this);
    m_drop_btn->Bind(wxEVT_KILL_FOCUS, &LookupChoiceControl::OnControlKillFocus, this);
    Bind(wxEVT_SIZE, &LookupChoiceControl::OnSize, this);
    Bind(wxEVT_KILL_FOCUS, &LookupChoiceControl::OnControlKillFocus, this);

    m_list->Bind(wxEVT_LISTBOX, &LookupChoiceControl::OnListSelect, this);
    m_list->Bind(wxEVT_LISTBOX_DCLICK, &LookupChoiceControl::OnListActivate, this);
    m_list->Bind(wxEVT_LEFT_DOWN, &LookupChoiceControl::OnListLeftDown, this);
    m_list->Bind(wxEVT_LEFT_UP, &LookupChoiceControl::OnListLeftUp, this);
    m_list->Bind(wxEVT_MOTION, &LookupChoiceControl::OnListMouseMove, this);
#ifdef __WXGTK__
    m_list->Bind(wxEVT_KEY_DOWN, &LookupChoiceControl::OnListKeyDown, this);
    m_list->Bind(wxEVT_SET_FOCUS, &LookupChoiceControl::OnListFocus, this);
#else
    m_list->Bind(wxEVT_CHAR_HOOK, &LookupChoiceControl::OnListKeyDown, this);
#endif
    m_list->Bind(wxEVT_KILL_FOCUS, &LookupChoiceControl::OnControlKillFocus, this);

    UpdateFilteredItems();
    ShowTextFromStart();
}

LookupChoiceControl::~LookupChoiceControl()
{
    HidePopup();
}

wxString LookupChoiceControl::GetValue() const
{
    return m_text->GetValue();
}

void LookupChoiceControl::ChangeValue(const wxString& value)
{
    m_text->ChangeValue(value);
    const int idx = FindChoiceIndex(value);
    if (idx != wxNOT_FOUND)
    {
        m_selection = idx;
        m_committed_value = value;
    }
    ShowTextFromStart();
}

void LookupChoiceControl::SetSelection(int selection)
{
    if (selection >= 0 && selection < static_cast<int>(m_choices.GetCount()))
    {
        m_selection = selection;
        m_committed_value = m_choices[static_cast<std::size_t>(selection)];
        m_text->ChangeValue(m_committed_value);
        ShowTextFromStart();
    }
}

int LookupChoiceControl::GetSelection() const
{
    const int current = FindChoiceIndex(m_text->GetValue());
    if (current != wxNOT_FOUND)
    {
        return current;
    }
    return m_selection;
}

unsigned int LookupChoiceControl::GetCount() const
{
    return m_choices.GetCount();
}

wxString LookupChoiceControl::GetString(unsigned int index) const
{
    if (index >= m_choices.GetCount())
    {
        return wxString();
    }
    return m_choices[index];
}

bool LookupChoiceControl::Enable(bool enable)
{
    if (!enable)
    {
        HidePopup();
    }

    const bool changed = wxPanel::Enable(enable);
    m_text->Enable(enable);
    m_drop_btn->Enable(enable);
    m_list->Enable(enable);
    m_popup->Enable(enable);
    return changed;
}

int LookupChoiceControl::FindChoiceIndex(const wxString& value) const
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

bool LookupChoiceControl::IsCurrentValueValidChoice() const
{
    return FindChoiceIndex(m_text->GetValue()) != wxNOT_FOUND;
}

bool LookupChoiceControl::CommitCurrentTextIfValid()
{
    const int idx = FindChoiceIndex(m_text->GetValue());
    if (idx == wxNOT_FOUND)
    {
        return false;
    }

    const int previous_selection = m_selection;
    m_selection = idx;
    m_committed_value = m_choices[static_cast<std::size_t>(idx)];
    if (m_text->GetValue() != m_committed_value)
    {
        m_text->ChangeValue(m_committed_value);
        ShowTextFromStart();
    }
    if (m_selection != previous_selection)
    {
        SendSelectionChangedEvent();
    }
    return true;
}

void LookupChoiceControl::RestoreCommittedValue()
{
    m_text->ChangeValue(m_committed_value);
    ShowTextFromStart();
    m_selection = FindChoiceIndex(m_committed_value);
}

void LookupChoiceControl::SendSelectionChangedEvent()
{
    wxCommandEvent evt(wxEVT_COMMAND_CHOICE_SELECTED, GetId());
    evt.SetEventObject(this);
    evt.SetInt(m_selection);
    evt.SetString(m_committed_value);
    ProcessWindowEvent(evt);
}

void LookupChoiceControl::ShowTextFromStart()
{
    m_text->SetInsertionPoint(0);
    m_text->SetSelection(0, 0);
}

void LookupChoiceControl::UpdateFilteredItems(bool show_all, bool auto_select)
{
    const wxString needle = show_all ? wxString() : m_text->GetValue().Lower();
    const wxString current_value = m_text->GetValue().Lower();
    m_updating_list = true;
    m_list->Freeze();
    m_list->Clear();
    m_filtered_indices.clear();

    int selected_row = wxNOT_FOUND;
    for (std::size_t i = 0; i < m_choices.GetCount(); ++i)
    {
        const wxString& choice = m_choices[i];
        if (needle.IsEmpty() || choice.Lower().Find(needle) != wxNOT_FOUND)
        {
            m_list->Append(choice);
            m_filtered_indices.push_back(static_cast<int>(i));

            if (selected_row == wxNOT_FOUND && m_selection == static_cast<int>(i))
            {
                selected_row = static_cast<int>(m_list->GetCount()) - 1;
            }
            else if (selected_row == wxNOT_FOUND && !current_value.IsEmpty() && choice.Lower() == current_value)
            {
                selected_row = static_cast<int>(m_list->GetCount()) - 1;
            }
        }
    }

    if (auto_select && m_list->GetCount() > 0)
    {
        if (selected_row == wxNOT_FOUND)
        {
            selected_row = 0;
        }
        m_list->SetSelection(selected_row);
        m_list->EnsureVisible(selected_row);
    }
    else
    {
        const int sel = m_list->GetSelection();
        if (sel != wxNOT_FOUND)
        {
            m_list->Deselect(sel);
        }
    }

    m_updating_list = false;
    m_list->Thaw();
}

void LookupChoiceControl::ShowPopup()
{
    if (!IsEnabled())
    {
        return;
    }

    if (m_list->GetCount() <= 0)
    {
        return;
    }

    const int row_height = std::max(m_list->GetCharHeight() + 6, 18);
    const int visible_rows = std::min(12, static_cast<int>(m_list->GetCount()));
    const int popup_height = row_height * visible_rows + 8;
    const int popup_width = std::max(GetSize().GetWidth(), 300);

    const wxPoint screen_pt = ClientToScreen(wxPoint(0, GetSize().GetHeight()));
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

    const int selected = m_list->GetSelection();
    if (selected != wxNOT_FOUND)
    {
        m_list->EnsureVisible(selected);
    }
}

void LookupChoiceControl::HidePopup()
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

void LookupChoiceControl::AcceptSelected()
{
    const int row = m_list->GetSelection();
    if (row != wxNOT_FOUND && row >= 0 && row < static_cast<int>(m_filtered_indices.size()))
    {
        const int previous_selection = m_selection;
        m_selection = m_filtered_indices[static_cast<std::size_t>(row)];
        m_committed_value = m_choices[static_cast<std::size_t>(m_selection)];
        m_text->ChangeValue(m_committed_value);
        ShowTextFromStart();
        if (m_selection != previous_selection)
        {
            SendSelectionChangedEvent();
        }
    }
    HidePopup();
}

void LookupChoiceControl::MoveSelection(int delta)
{
    if (m_list->GetCount() <= 0)
    {
        return;
    }

    int sel = m_list->GetSelection();
    if (sel == wxNOT_FOUND)
    {
        sel = 0;
    }
    else
    {
        sel = std::clamp(sel + delta, 0, static_cast<int>(m_list->GetCount()) - 1);
    }

    m_list->SetSelection(sel);
    m_list->EnsureVisible(sel);
}

void LookupChoiceControl::OnText(wxCommandEvent& evt)
{
#ifdef __WXGTK__
    UpdateFilteredItems(false, false);
#else
    UpdateFilteredItems();
#endif
    if (m_list->GetCount() > 0)
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

void LookupChoiceControl::OnTextLeftDown(wxMouseEvent& evt)
{
    evt.Skip();
    if (!m_popup->IsShown() && m_list->GetCount() > 0)
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

void LookupChoiceControl::OnTextFocus(wxFocusEvent& evt)
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

void LookupChoiceControl::OnTextKeyDown(wxKeyEvent& evt)
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
        return;
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        if (m_popup->IsShown())
        {
            AcceptSelected();
            return;
        }
        break;
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

void LookupChoiceControl::OnTextEnter(wxCommandEvent& evt)
{
    if (m_popup->IsShown())
    {
        AcceptSelected();
        return;
    }

    if (!CommitCurrentTextIfValid())
    {
        RestoreCommittedValue();
    }

    evt.Skip();
}

void LookupChoiceControl::OnSize(wxSizeEvent& evt)
{
    if (m_text && m_drop_btn)
    {
        const wxSize client = GetClientSize();
        const int button_w = std::max(m_drop_btn->GetBestSize().GetWidth(), 24);
        const int text_w = std::max(0, client.GetWidth() - button_w);

        m_text->SetSize(0, 0, text_w, client.GetHeight());
        m_drop_btn->SetSize(text_w, 0, button_w, client.GetHeight());
        if (wxWindow::FindFocus() != m_text && IsCurrentValueValidChoice())
        {
            CallAfter([this]()
            {
                if (m_text && wxWindow::FindFocus() != m_text && IsCurrentValueValidChoice())
                {
                    ShowTextFromStart();
                }
            });
        }
    }

    Layout();
    evt.Skip();
}

void LookupChoiceControl::OnControlKillFocus(wxFocusEvent& evt)
{
    evt.Skip();
    CallAfter([this]()
    {
        wxWindow* focus = wxWindow::FindFocus();
        if (IsDescendantOf(focus, this) || IsDescendantOf(focus, m_popup))
        {
            return;
        }

        if (!CommitCurrentTextIfValid())
        {
            RestoreCommittedValue();
        }

        HidePopup();
    });
}

void LookupChoiceControl::OnListSelect(wxCommandEvent& evt)
{
#ifdef __WXGTK__
    // On GTK, selection events can fire during filtering; commit only on explicit user action.
    // Selection changes can be triggered programmatically on GTK while filtering.
    // Commit only on explicit user actions (enter/double-click/click release).
#else
    // On Windows, listbox selection notifications map cleanly to intentional user selection.
    AcceptSelected();
#endif
    evt.Skip();
}

void LookupChoiceControl::OnListActivate(wxCommandEvent& evt)
{
    AcceptSelected();
    evt.Skip();
}

void LookupChoiceControl::OnListLeftDown(wxMouseEvent& evt)
{
#ifdef __WXGTK__
    const int hit = m_list->HitTest(evt.GetPosition());
    if (hit != wxNOT_FOUND && hit >= 0 && hit < static_cast<int>(m_list->GetCount()))
    {
        m_list->SetSelection(hit);
        m_list_click_candidate = hit;
        return;
    }

    m_list_click_candidate = wxNOT_FOUND;
    evt.Skip();
#else
    const int hit = m_list->HitTest(evt.GetPosition());
    if (hit != wxNOT_FOUND && hit >= 0 && hit < static_cast<int>(m_list->GetCount()))
    {
        m_list->SetSelection(hit);
        AcceptSelected();
        m_text->SetFocus();
        return;
    }
    evt.Skip();
#endif
}

void LookupChoiceControl::OnListLeftUp(wxMouseEvent& evt)
{
#ifdef __WXGTK__
    const int hit = m_list->HitTest(evt.GetPosition());
    if (m_list_click_candidate != wxNOT_FOUND && hit == m_list_click_candidate &&
        hit >= 0 && hit < static_cast<int>(m_list->GetCount()))
    {
        m_list->SetSelection(hit);
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
        if (m_list->GetSelection() != wxNOT_FOUND)
        {
            AcceptSelected();
            m_text->SetFocus();
        }
    });
#endif
}

void LookupChoiceControl::OnListMouseMove(wxMouseEvent& evt)
{
#ifdef __WXGTK__
    if (wxWindow::FindFocus() == m_text)
    {
        evt.Skip();
        return;
    }
#endif

    const int hit = m_list->HitTest(evt.GetPosition());
    if (hit != wxNOT_FOUND && hit >= 0 && hit < static_cast<int>(m_list->GetCount()))
    {
        if (m_list->GetSelection() != hit)
        {
            m_list->SetSelection(hit);
        }
    }
    evt.Skip();
}

void LookupChoiceControl::OnListFocus(wxFocusEvent& evt)
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

void LookupChoiceControl::OnListKeyDown(wxKeyEvent& evt)
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

void LookupChoiceControl::OnDropDownClick(wxCommandEvent& evt)
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
    if (m_list->GetCount() <= 0)
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

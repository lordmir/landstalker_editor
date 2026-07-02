#include <misc/LookupChoiceControl.h>

#include <algorithm>
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
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
      m_popup(new wxPopupTransientWindow(this, wxBORDER_SIMPLE | wxPU_CONTAINS_CONTROLS)),
      m_list(new wxListBox(m_popup, wxID_ANY))
{
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_text, 1, wxEXPAND);
    sizer->Add(m_drop_btn, 0, wxEXPAND);
    SetSizer(sizer);

    auto* popup_sizer = new wxBoxSizer(wxVERTICAL);
    popup_sizer->Add(m_list, 1, wxEXPAND);
    m_popup->SetSizer(popup_sizer);

    m_text->Bind(wxEVT_TEXT, &LookupChoiceControl::OnText, this);
    m_text->Bind(wxEVT_KEY_DOWN, &LookupChoiceControl::OnTextKeyDown, this);
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
    m_list->Bind(wxEVT_KEY_DOWN, &LookupChoiceControl::OnListKeyDown, this);
    m_list->Bind(wxEVT_SET_FOCUS, &LookupChoiceControl::OnListFocus, this);
    m_list->Bind(wxEVT_KILL_FOCUS, &LookupChoiceControl::OnControlKillFocus, this);

    UpdateFilteredItems();
    m_text->SetFocus();
    m_text->SetInsertionPointEnd();
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
}

void LookupChoiceControl::SetSelection(int selection)
{
    if (selection >= 0 && selection < static_cast<int>(m_choices.GetCount()))
    {
        m_text->ChangeValue(m_choices[static_cast<std::size_t>(selection)]);
        m_text->SetInsertionPointEnd();
    }
}

int LookupChoiceControl::GetSelection() const
{
    const wxString value = m_text->GetValue().Lower();
    for (std::size_t i = 0; i < m_choices.GetCount(); ++i)
    {
        if (m_choices[i].Lower() == value)
        {
            return static_cast<int>(i);
        }
    }
    return wxNOT_FOUND;
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

void LookupChoiceControl::UpdateFilteredItems(bool show_all, bool auto_select)
{
    const wxString needle = show_all ? wxString() : m_text->GetValue().Lower();
    const wxString current_value = m_text->GetValue().Lower();
    m_updating_list = true;
    m_list->Freeze();
    m_list->Clear();

    int selected_row = wxNOT_FOUND;
    for (std::size_t i = 0; i < m_choices.GetCount(); ++i)
    {
        const wxString& choice = m_choices[i];
        if (needle.IsEmpty() || choice.Lower().Find(needle) != wxNOT_FOUND)
        {
            m_list->Append(choice);
            if (selected_row == wxNOT_FOUND && !current_value.IsEmpty() && choice.Lower() == current_value)
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
        m_popup->Popup(m_text);
    }

    CallAfter([this]()
    {
        wxWindow* focus = wxWindow::FindFocus();
        if (IsDescendantOf(focus, m_popup) && focus != m_text)
        {
            m_text->SetFocus();
            m_text->SetInsertionPointEnd();
        }
    });

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
        m_popup->Dismiss();
    }
}

void LookupChoiceControl::AcceptSelected()
{
    const int sel = m_list->GetSelection();
    if (sel != wxNOT_FOUND && sel >= 0 && sel < static_cast<int>(m_list->GetCount()))
    {
        m_text->ChangeValue(m_list->GetString(sel));
        m_text->SetInsertionPointEnd();
        m_text->SetSelection(m_text->GetLastPosition(), m_text->GetLastPosition());
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
    UpdateFilteredItems(false, false);
    if (m_list->GetCount() > 0)
    {
        ShowPopup();
    }
    else if (m_popup->IsShown())
    {
        HidePopup();
    }

    if (wxWindow::FindFocus() == m_text)
    {
        const long pos = m_text->GetInsertionPoint();
        m_text->SetSelection(pos, pos);
    }
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
    if (!m_select_all_on_focus)
    {
        return;
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
        if (m_popup->IsShown())
        {
            HidePopup();
            return;
        }
        break;
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
    evt.Skip();
}

void LookupChoiceControl::OnSize(wxSizeEvent& evt)
{
    Layout();
    evt.Skip();
}

void LookupChoiceControl::OnControlKillFocus(wxFocusEvent& evt)
{
    evt.Skip();
    CallAfter([this]()
    {
        if (!m_popup || !m_popup->IsShown())
        {
            return;
        }

        wxWindow* focus = wxWindow::FindFocus();
        if (IsDescendantOf(focus, this) || IsDescendantOf(focus, m_popup))
        {
            return;
        }

        HidePopup();
    });
}

void LookupChoiceControl::OnListSelect(wxCommandEvent& evt)
{
    // Selection changes can be triggered programmatically on GTK while filtering.
    // Commit only on explicit user actions (enter/double-click/click release).
    evt.Skip();
}

void LookupChoiceControl::OnListActivate(wxCommandEvent& evt)
{
    AcceptSelected();
    evt.Skip();
}

void LookupChoiceControl::OnListLeftDown(wxMouseEvent& evt)
{
    const int hit = m_list->HitTest(evt.GetPosition());
    if (hit != wxNOT_FOUND && hit >= 0 && hit < static_cast<int>(m_list->GetCount()))
    {
        m_list->SetSelection(hit);
        m_list_click_candidate = hit;
        return;
    }

    m_list_click_candidate = wxNOT_FOUND;
    evt.Skip();
}

void LookupChoiceControl::OnListLeftUp(wxMouseEvent& evt)
{
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
}

void LookupChoiceControl::OnListMouseMove(wxMouseEvent& evt)
{
    if (wxWindow::FindFocus() == m_text)
    {
        evt.Skip();
        return;
    }

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
}

void LookupChoiceControl::OnListKeyDown(wxKeyEvent& evt)
{
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

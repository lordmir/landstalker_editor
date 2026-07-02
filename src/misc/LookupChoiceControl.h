#ifndef _LOOKUP_CHOICE_CONTROL_H_
#define _LOOKUP_CHOICE_CONTROL_H_

#include <wx/arrstr.h>
#include <wx/panel.h>

class wxBitmapButton;
class wxListBox;
class wxPopupTransientWindow;
class wxTextCtrl;
class wxCommandEvent;
class wxFocusEvent;
class wxKeyEvent;
class wxMouseEvent;
class wxSizeEvent;

class LookupChoiceControl : public wxPanel
{
public:
    LookupChoiceControl(wxWindow* parent, wxWindowID id, const wxString& value, const wxArrayString& choices,
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    ~LookupChoiceControl() override;

    wxString GetValue() const;
    void ChangeValue(const wxString& value);
    void SetSelection(int selection);
    int GetSelection() const;
    unsigned int GetCount() const;
    wxString GetString(unsigned int index) const;

private:
    void UpdateFilteredItems(bool show_all = false, bool auto_select = true);
    void ShowPopup();
    void HidePopup();
    void AcceptSelected();
    void MoveSelection(int delta);

    void OnText(wxCommandEvent& evt);
    void OnTextLeftDown(wxMouseEvent& evt);
    void OnTextFocus(wxFocusEvent& evt);
    void OnControlKillFocus(wxFocusEvent& evt);
    void OnTextKeyDown(wxKeyEvent& evt);
    void OnTextEnter(wxCommandEvent& evt);
    void OnSize(wxSizeEvent& evt);

    void OnListSelect(wxCommandEvent& evt);
    void OnListActivate(wxCommandEvent& evt);
    void OnListFocus(wxFocusEvent& evt);
    void OnListLeftDown(wxMouseEvent& evt);
    void OnListLeftUp(wxMouseEvent& evt);
    void OnListMouseMove(wxMouseEvent& evt);
    void OnListKeyDown(wxKeyEvent& evt);

    void OnDropDownClick(wxCommandEvent& evt);

    wxArrayString m_choices;
    wxTextCtrl* m_text;
    wxBitmapButton* m_drop_btn;
    wxPopupTransientWindow* m_popup;
    wxListBox* m_list;
    int m_list_click_candidate = wxNOT_FOUND;
    bool m_updating_list = false;
    bool m_select_all_on_focus = true;
};

#endif // _LOOKUP_CHOICE_CONTROL_H_

#ifndef _SCRIPT_TREE_ACTION_EDITORS_H_
#define _SCRIPT_TREE_ACTION_EDITORS_H_

#include <wx/wx.h>
#include <wx/dataview.h>
#include <script/ScriptTreeNode.h>
#include <script/ScriptTreeDataViewModel.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/valtext.h>
#include <wx/combobox.h>
#include <wx/popupwin.h>
#include <wx/stattext.h>
#include <wx/timer.h>

class ScriptActionEditorCtrl : public wxPanel
{
public:
    ScriptActionEditorCtrl(wxWindow* parent, const wxRect& rect, bool is_function, uint16_t script_id, const wxString& function_name, ScriptTreeDataViewModel* model);

    bool IsFunctionMode() const;
    bool IsNewFunctionMode() const;
    wxString GetFunctionName() const;
    wxString GetNewFunctionName() const;
    uint16_t GetScriptId() const;
    // Resolves the function combo's pending (highlighted-but-not-yet-committed) popup selection,
    // if any - call before GetFunctionName() when reading the final committed value.
    void CommitFunctionSelection();

private:
    void OnModeChanged(wxCommandEvent& event);
    void OnIdTextChanged(wxCommandEvent& event);
    void OnFunctionTextChanged(wxCommandEvent& event);
    void OnNewFunctionTextChanged(wxCommandEvent& event);
    void OnSpinUp(wxSpinEvent& event);
    void OnSpinDown(wxSpinEvent& event);
    void OnPreviewHideTimer(wxTimerEvent& event);
    void UpdateLayout();
    void UpdateIdColour();
    void UpdateFunctionColour();
    void UpdateNewFunctionColour();
    void ShowPreviewPopup();
    void HidePreviewPopup();
    void UpdatePreviewPopup();
    uint16_t ParseId() const;

    ScriptTreeDataViewModel* m_model;
    wxChoice* m_mode;
    wxWindow* m_function_combo;
    wxTextCtrl* m_id_text;
    wxSpinButton* m_id_spin;
    wxTextCtrl* m_new_function_text;
    wxPopupWindow* m_preview_popup = nullptr;
    wxStaticText* m_preview_text = nullptr;
    wxTimer m_preview_hide_timer;
};

class ScriptActionRenderer : public wxDataViewCustomRenderer
{
public:
    ScriptActionRenderer();

    virtual bool SetValue(const wxVariant& value) override;
    virtual bool GetValue(wxVariant& value) const override;
    virtual bool Render(wxRect rect, wxDC* dc, int state) override;
    virtual wxSize GetSize() const override;
    virtual bool HasEditorCtrl() const override;
    virtual wxWindow* CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value) override;
    virtual bool GetValueFromEditorCtrl(wxWindow* editorCtrl, wxVariant& value) override;

private:
    // The ScriptTreeNode behind the row currently being edited (wxDataViewRendererBase::m_item is set
    // for the duration of an edit) - editors are chosen and pre-filled from its type and
    // typed payload, never by parsing display text.
    ScriptTreeNode* GetEditedNode() const;
    // The same renderer instance is reused across many entries whose models are swapped in
    // and out of the owning wxDataViewCtrl over time, so the model can't be cached at
    // construction time - always resolve whichever model is currently associated.
    ScriptTreeDataViewModel* GetTreeModel() const;

    wxString m_value;
};

#endif // _SCRIPT_TREE_ACTION_EDITORS_H_

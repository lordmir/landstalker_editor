#ifndef _BEHAVIOUR_COMMAND_EDITOR_H_
#define _BEHAVIOUR_COMMAND_EDITOR_H_

#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/behaviours/Behaviours.h>

// The Behaviour Script Editor's floating in-row command editor. Unlike the Main Script Editor's
// per-entry-type editor subclasses (ScriptEntryEditors.h), a single metadata-driven editor covers
// every behaviour command: the parameter fields are built from Behaviours::CommandDefinition, and
// rebuilt in place whenever the user picks a different command from the dropdown - with ~105
// command types, in-editor type switching replaces the context menu's per-type submenus.
class BehaviourCommandEditorCtrl : public wxPanel
{
public:
    // `max_label_target` bounds LABEL (GotoInstruction target) parameters: the current number of
    // commands in the script, since targets are 1-based instruction numbers.
    BehaviourCommandEditorCtrl(wxWindow* parent, const wxRect& rect,
        const Landstalker::Behaviours::Command& command, int max_label_target);
    virtual ~BehaviourCommandEditorCtrl() = default;

    Landstalker::Behaviours::Command GetValue() const;

    // The renderer owning this editor - needed so parameter fields created by a mid-edit command
    // change can get the same Escape/Enter/focus-loss wiring the initial fields got from
    // CreateEditorCtrl()'s recursive BindDataViewEditorEscapeEnter() call.
    void SetRenderer(wxDataViewRenderer* renderer);

    // Same self-drawn Index-cell copy as ScriptEntryEditorCtrl::SetIndexCellReference() - see
    // that class's comments for why the strip this window overlaps needs it.
    void SetIndexCellReference(wxDataViewCtrl* dvc, const wxDataViewItem& item,
        wxDataViewColumn* col, const wxString& index_text);

    void SetFocus() override;

private:
    struct ParamField
    {
        std::string name;
        Landstalker::Behaviours::ParamType type;
        wxWindow* ctrl;
        // For LookupChoiceControl-backed fields: the id of the list's first entry (non-zero for
        // HIGH_CUTSCENE, whose ids start at 256) and the value to keep when nothing is selected.
        int base = 0;
        int fallback = 0;
    };

    void BuildParamFields(const Landstalker::Behaviours::Command& command);
    // Applies the platform-appropriate window size after (re)building fields: GTK shrinks to the
    // content's natural minimum, MSW keeps the full cell rect (see the .cpp comments).
    void FitToContent();
    Landstalker::Behaviours::Command CollectCommand() const;
    void OnCommandChange(wxCommandEvent& evt);
    void OnPaint(wxPaintEvent& evt);

    wxChoice* m_command_choice;
    wxBoxSizer* m_params_sizer;
    std::vector<ParamField> m_fields;
    wxDataViewRenderer* m_renderer = nullptr;
    int m_max_label_target;
    // The cell rect this editor was opened over - FitToContent()'s sizing reference on MSW.
    wxRect m_cell_rect;

    wxDataViewCtrl* m_index_cell_dvc = nullptr;
    wxDataViewItem m_index_cell_item;
    wxDataViewColumn* m_index_cell_col = nullptr;
    wxString m_index_text;
};

#endif // _BEHAVIOUR_COMMAND_EDITOR_H_

#ifndef _BEHAVIOUR_COMMAND_DATA_VIEW_RENDERER_H_
#define _BEHAVIOUR_COMMAND_DATA_VIEW_RENDERER_H_

#include <functional>
#include <optional>
#include <string>

#include <wx/dataview.h>
#include <wx/longlong.h>

#include <landstalker/behaviours/Behaviours.h>

// How a behaviour command travels through the wxVariant value pipeline (model -> renderer ->
// editor -> model): its single-command YAML form, the exact representation the YAML
// import/export already round-trips via BehaviourYamlConverter. Unlike the Main Script Editor's
// 16-bit ToBytes() encoding, a behaviour command has variable-count, variable-type (int/double)
// parameters, so a self-contained string is the only variant type that can carry one whole.
namespace BehaviourCommand
{
    std::string Serialise(const Landstalker::Behaviours::Command& cmd);
    // nullopt on malformed input (rendered as INVALID rather than throwing into wx internals).
    std::optional<Landstalker::Behaviours::Command> Deserialise(const std::string& yaml);
    // A command of the given type with every parameter present at a sensible default (0/0.0;
    // 1 for LABEL - instruction numbers are 1-based; 256 for HIGH_CUTSCENE - its encodable floor).
    Landstalker::Behaviours::Command MakeDefault(Landstalker::Behaviours::CommandType type);

    // Bubble colours, grouped into rough functional categories (movement, facing, flags, ...).
    struct Style
    {
        wxColour bg;
        wxColour fg;
    };
    Style GetStyle(Landstalker::Behaviours::CommandType type);
}

class BehaviourCommandDataViewRenderer : public wxDataViewCustomRenderer
{
public:
    // `row_count` reports the live command count of the script being displayed - used to give
    // GotoInstruction's target parameter a valid range (editor) and flag dangling targets (render).
    BehaviourCommandDataViewRenderer(wxDataViewCellMode mode, std::function<unsigned int()> row_count);
    virtual ~BehaviourCommandDataViewRenderer() {}

    virtual bool Render(wxRect rect, wxDC* dc, int state) override;
    virtual bool ActivateCell(const wxRect& cell, wxDataViewModel* model, const wxDataViewItem& item,
                              unsigned int col, const wxMouseEvent* mouseEvent) override;
    virtual wxSize GetSize() const override;
    virtual bool SetValue(const wxVariant& value) override;
    virtual bool GetValue(wxVariant& value) const override;
    virtual bool HasEditorCtrl() const override;
    virtual wxWindow* CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value) override;
    virtual bool GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value) override;

    // Same rapid finish/restart debounce guard as ScriptDataViewRenderer - see that class's
    // comment for the GTK-level cascade it breaks.
    virtual bool StartEditing(const wxDataViewItem& item, wxRect labelRect) override;
    virtual bool FinishEditing() override;
    virtual void CancelEditing() override;

private:
    // Same incremental drawing helpers as ScriptDataViewRenderer (see that class for the GTK
    // text-colour caveat the attribute juggling handles).
    void InsertRenderLabel(wxRect& rect, wxDC* dc, int state, const wxString& text, int min_width = 0,
                           const wxFont* font = nullptr, const wxColour* text_colour = nullptr);
    void InsertRenderBubble(wxRect& rect, wxDC* dc, int state, const wxString& text, const wxColour& colour,
                            int min_width = 0, const wxFont* font = nullptr, const wxColour* text_colour = nullptr);

    void RenderParameter(wxRect& rect, wxDC* dc, int state, const Landstalker::Behaviours::Parameter& param);

    std::optional<Landstalker::Behaviours::Command> m_value;
    // The serialised form m_value was parsed from - lets SetValue() skip re-parsing when asked
    // to display the same value again (which happens on every hover/selection repaint).
    wxString m_raw;
    std::function<unsigned int()> m_row_count;
    wxLongLong m_lastEditEndTimeMs = 0;

    // Stashed by StartEditing() for CreateEditorCtrl()'s index-cell handling - see
    // ScriptDataViewRenderer::m_editingItem's comment.
    wxDataViewItem m_editingItem;
};

#endif // _BEHAVIOUR_COMMAND_DATA_VIEW_RENDERER_H_

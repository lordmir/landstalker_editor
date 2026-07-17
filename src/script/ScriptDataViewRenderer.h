#ifndef _SCRIPT_DATA_VIEW_RENDERER_
#define _SCRIPT_DATA_VIEW_RENDERER_

#include <wx/dataview.h>
#include <wx/longlong.h>
#include <map>
#include <optional>
#include <landstalker/script/ScriptTableEntry.h>

class ScriptDataViewRenderer : public wxDataViewCustomRenderer
{
public:
    ScriptDataViewRenderer(wxDataViewCellMode mode, std::shared_ptr<Landstalker::GameData> gd);
    virtual ~ScriptDataViewRenderer() {}
    virtual bool Render(wxRect rect, wxDC* dc, int state) override;
    bool RenderLabel(wxRect rect, wxDC* dc, int state);

    // `text_colour`: see InsertRenderBubble()'s comment - the same GTK caveat applies here.
    void InsertRenderLabel(wxRect& rect, wxDC* dc, int state, const wxString& text, int min_width = 0, const wxFont* font = nullptr, const wxColour* text_colour = nullptr);
    // Draws the row's cutscene/character display name: as an underlined, link-blue hyperlink
    // (recording its cell-relative hit rect for ScriptEditorCtrl's click/cursor handling) when
    // the row's entry resolves to another entry's own script tree (see ScriptEntryLink), or as
    // the standard italic name label otherwise.
    void InsertRenderName(wxRect& rect, wxDC* dc, int state, const wxString& name, int min_width = 0);
    // `text_colour`, if given, forces the bubble's label to that colour - needed because
    // wxDataViewCustomRenderer::RenderText() on GTK renders through a native GtkCellRendererText
    // driven by GetAttr()/SetAttr(), ignoring `dc` entirely, so a plain dc->SetTextForeground()
    // around this call has no effect on Linux (see the .cpp for the mechanism used instead).
    void InsertRenderBubble(wxRect& rect, wxDC* dc, int state, const wxString& text, const wxColour& colour, int min_width = 0, const wxFont* font = nullptr, const wxColour* text_colour = nullptr);
    void InsertRenderCheckbox(wxRect& rect, wxDC* dc, int state, const wxString& text, bool checkstate, int min_width = 0, const wxFont* font = nullptr);

    bool RenderInvalidProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderStringProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderCutsceneProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderSetItemProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderSetNumberProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderGiveItemProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderGiveMoneyProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderSetFlagProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderPlayBGMProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderSetSpeakerProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderSetGlobalSpeakerProperties(wxRect& rect, wxDC* dc, int state);
    bool RenderLoadGlobalSpeakerProperties(wxRect& rect, wxDC* dc, int state);

    virtual bool ActivateCell(const wxRect& cell, wxDataViewModel* model, const wxDataViewItem& item,
                              unsigned int col, const wxMouseEvent* mouseEvent) override;
    virtual wxSize GetSize() const override;
    virtual bool SetValue(const wxVariant& value) override;
    virtual bool GetValue(wxVariant& value) const override;
    virtual bool HasEditorCtrl() const override;
    virtual wxWindow* CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value) override;
    virtual bool GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value) override;

    // Debounce guard: something beneath wx's own event layer (confirmed via gdb - not something wx-
    // level event handling can intercept) can retrigger StartEditing() on this same renderer several
    // times within a single rapid click burst, each racing the previous editor's GTK-level teardown
    // against a brand new one's construction and eventually segfaulting on a dangling child widget.
    // Refusing to start a new edit session immediately after one just ended breaks that cascade.
    virtual bool StartEditing(const wxDataViewItem& item, wxRect labelRect) override;
    virtual bool FinishEditing() override;
    virtual void CancelEditing() override;

    // The cell-relative hit rect of the hyperlinked name captured during the given script
    // line's last render (see InsertRenderName), or nullopt when that row has no link. Only
    // rendered (i.e. visible) rows have entries, which is exactly the set a mouse can hit.
    std::optional<wxRect> GetLinkHitRect(long row) const;
private:
    std::unique_ptr<Landstalker::ScriptTableEntry> m_value;
    long m_index;
    std::shared_ptr<Landstalker::GameData> m_gd;
    wxLongLong m_lastEditEndTimeMs = 0;

    // Stashed by StartEditing() so CreateEditorCtrl() can query the Index column's actual on-screen
    // cell rect via wxDataViewCtrl::GetItemRect() - unlike a column's GetWidth(), that call is scroll-
    // aware, which turned out to be exactly what was missing (see ScriptEntryEditorCtrl::
    // SetIndexCellReference()'s comment).
    wxDataViewItem m_editingItem;

    // Hyperlink hit rects (cell-relative, keyed by absolute script line), rebuilt as rows render -
    // see InsertRenderName()/GetLinkHitRect(). m_cell_rect is the cell being rendered right now,
    // captured at the top of Render() so the name's position can be made cell-relative.
    std::map<long, wxRect> m_link_rects;
    wxRect m_cell_rect;

    static const int INDEX_Y_OFFSET = 2;
    static const int TYPE_Y_OFFSET = 40;
    static const int OPTION_Y_OFFSET = 100;
    static const int PREVIEW_Y_OFFSET = 400;
};

#endif // _SCRIPT_DATA_VIEW_RENDERER_

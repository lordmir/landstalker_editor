#ifndef _SCRIPT_DATA_VIEW_RENDERER_
#define _SCRIPT_DATA_VIEW_RENDERER_

#include <wx/dataview.h>
#include <optional>
#include <landstalker/script/include/ScriptTableEntry.h>

class ScriptDataViewRenderer : public wxDataViewCustomRenderer
{
public:
    ScriptDataViewRenderer(wxDataViewCellMode mode, std::shared_ptr<GameData> gd);
    virtual ~ScriptDataViewRenderer() {}
    virtual bool Render(wxRect rect, wxDC* dc, int state) override;
    bool RenderLabel(wxRect rect, wxDC* dc, int state);

    void InsertRenderLabel(wxRect& rect, wxDC* dc, int state, const wxString& text, int min_width = 0, const wxFont* font = nullptr);
    void InsertRenderBubble(wxRect& rect, wxDC* dc, int state, const wxString& text, const wxColour& colour, int min_width = 0, const wxFont* font = nullptr);
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
private:
    std::unique_ptr<ScriptTableEntry> m_value;
    long m_index;
    std::shared_ptr<GameData> m_gd;

    static const int INDEX_Y_OFFSET = 2;
    static const int TYPE_Y_OFFSET = 40;
    static const int OPTION_Y_OFFSET = 100;
    static const int PREVIEW_Y_OFFSET = 400;
};

#endif // _SCRIPT_DATA_VIEW_RENDERER_

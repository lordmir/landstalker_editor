#ifndef _SCRIPT_DATA_VIEW_RENDERER_
#define _SCRIPT_DATA_VIEW_RENDERER_

#include <wx/dataview.h>

#include <landstalker/script/include/ScriptTableEntry.h>

class ScriptDataViewRenderer : public wxDataViewCustomRenderer
{
public:
    explicit ScriptDataViewRenderer(wxDataViewCellMode model, std::shared_ptr<GameData> gd);
    virtual bool Render(wxRect rect, wxDC* dc, int state) override;
    bool RenderString(wxRect rect, wxDC* dc, int state);
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
    std::shared_ptr<GameData> m_gd;
};

#endif // _SCRIPT_DATA_VIEW_RENDERER_

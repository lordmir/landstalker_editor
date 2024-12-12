#ifndef _DATA_VIEW_SCRIPT_ACTION_RENDERER_H_
#define _DATA_VIEW_SCRIPT_ACTION_RENDERER_H_

#include <wx/dataview.h>
#include <optional>
#include <landstalker/main/include/GameData.h>
#include <landstalker/script/include/ScriptTable.h>

class DataViewScriptActionRenderer : public wxDataViewCustomRenderer
{
public:
    DataViewScriptActionRenderer(wxDataViewCellMode mode, std::shared_ptr<GameData> gd);
    virtual ~DataViewScriptActionRenderer() {}
    virtual bool Render(wxRect rect, wxDC* dc, int state) override;
    virtual bool ActivateCell(const wxRect& cell, wxDataViewModel* model, const wxDataViewItem& item,
        unsigned int col, const wxMouseEvent* mouseEvent) override;
    virtual wxSize GetSize() const override;
    virtual bool SetValue(const wxVariant& value) override;
    virtual bool GetValue(wxVariant& value) const override;
    virtual bool HasEditorCtrl() const override;
    virtual wxWindow* CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value) override;
    virtual bool GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value) override;
private:
    ScriptTable::Action m_value;
    long m_index;
    std::shared_ptr<GameData> m_gd;
};

#endif // _DATA_VIEW_SCRIPT_ACTION_RENDERER_H_

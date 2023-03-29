#ifndef _DATA_VIEW_CTRL_PALETTE_RENDERER_H_
#define _DATA_VIEW_CTRL_PALETTE_RENDERER_H_

#include <wx/dataview.h>
#include <wx/colordlg.h>
#include "DataTypes.h"

class DataViewCtrlPaletteRenderer : public wxDataViewCustomRenderer
{
public:
    DataViewCtrlPaletteRenderer(wxWindow* owner, wxDataViewCellMode mode);
    virtual ~DataViewCtrlPaletteRenderer() {}

    virtual bool Render(wxRect rect, wxDC* dc, int state) override;

    virtual bool ActivateCell(const wxRect& cell, wxDataViewModel* model, const wxDataViewItem& item,
        unsigned int col, const wxMouseEvent* evt) override;

    virtual wxSize GetSize() const override;

    int GetTotalWidth(int num_elems) const;

    int HitColour(const wxPoint& point, bool range_check = true) const;

    virtual bool SetValue(const wxVariant& value) override;

    virtual bool GetValue(wxVariant& value) const override { return true; }

    virtual bool HasEditorCtrl() const override { return false; }

    virtual wxWindow* CreateEditorCtrl(wxWindow* parent, wxRect labelRect,
            const wxVariant& value) override;

    virtual bool GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value) override;

    void Reset();

    void MoveCursorLeft();
    void MoveCursorRight();
    int GetCursorPosition() const;
    void SetCursorPosition(int pos);
    int GetCursorPaletteIndex();
    Palette::Colour GetCursorColour();

    static const int SWATCH_HEIGHT = 30;
    static const int SWATCH_WIDTH = 45;
    static const int SWATCH_SEPARATION = 2;

private:
    bool SetColour();

    PaletteEntry* m_pal;
    wxWindow* m_owner;
    void* m_sel_id;
    int m_cursor;
    int m_cursor_limit;
};

#endif // _DATA_VIEW_CTRL_PALETTE_RENDERER_H_
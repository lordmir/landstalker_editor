#include "DataViewCtrlPaletteRenderer.h"

#include <wx/dc.h>
#include <wx/wx.h>

DataViewCtrlPaletteRenderer::DataViewCtrlPaletteRenderer(wxWindow* owner, wxDataViewCellMode mode)
    : wxDataViewCustomRenderer("void*", mode, wxALIGN_LEFT),
    m_pal(nullptr),
    m_sel_id(nullptr),
    m_cursor(0),
    m_cursor_limit(0),
    m_owner(owner),
    m_been_activated(false)
{
}

bool DataViewCtrlPaletteRenderer::Render(wxRect rect, wxDC* dc, int state)
{
    bool active = (state & 1) != 0;
    int sel = m_cursor;
    if (m_pal && m_pal->GetData()->GetSize() > 0)
    {
        wxLogDebug("%s %d %d", m_pal->GetName().c_str(), state, m_cursor);
        const auto& palette = m_pal->GetData();
        std::size_t palette_size = palette->GetSize();
        if (active == true)
        {
            m_cursor_limit = palette_size - 1;
            if (m_cursor > m_cursor_limit)
            {
                sel = m_cursor_limit;
            }
        }

        const wxSize sz = wxSize(SWATCH_WIDTH, SWATCH_HEIGHT - SWATCH_SEPARATION);
        wxRect r = wxRect(rect.GetPosition(), sz);
        r.y += SWATCH_SEPARATION / 2;
        wxRect sel_r = wxRect(r).Deflate(1);

        for (int i = 0; i < palette_size; ++i)
        {
            bool selected = active && i == sel;
            dc->SetPen(selected ? *wxRED_PEN : *wxBLACK_PEN);
            dc->SetBrush(wxColor(palette->GetNthUnlockedColour(i).GetBGR(false)));
            dc->DrawRectangle(r);
            if (selected)
            {
                dc->SetBrush(*wxTRANSPARENT_BRUSH);
                dc->SetPen(*wxWHITE_PEN);
                dc->DrawRectangle(sel_r);
            }
            sel_r.x += r.GetWidth() + SWATCH_SEPARATION;
            r.x += r.GetWidth() + SWATCH_SEPARATION;
        }
    }
    return true;
}

bool DataViewCtrlPaletteRenderer::ActivateCell(const wxRect& cell, wxDataViewModel* model, const wxDataViewItem& item, unsigned int col, const wxMouseEvent* evt)
{
    if (item.GetID() != m_sel_id)
    {
        m_sel_id = item.GetID();
    }
    if (m_pal && m_pal->GetData()->GetSize() > 0)
    {
        if (evt == nullptr)
        {
            return SetColour();
        }
        else
        {
            int col_idx = HitColour({ evt->GetX(), evt->GetY() });
            if (col_idx != -1 && evt->LeftDown())
            {
                return SetColour();
            }
        }
    }
    return false;
}

wxSize DataViewCtrlPaletteRenderer::GetSize() const
{
    if (m_pal)
    {
        if (m_pal->GetData()->GetSize() > 0)
        {
            return wxSize((SWATCH_WIDTH + SWATCH_SEPARATION) * m_pal->GetData()->GetSize() - SWATCH_SEPARATION, SWATCH_HEIGHT);
        }
    }
    return wxSize(SWATCH_WIDTH, SWATCH_HEIGHT);
}

int DataViewCtrlPaletteRenderer::GetTotalWidth(int num_elems) const
{
    return (SWATCH_WIDTH + SWATCH_SEPARATION) * num_elems - SWATCH_SEPARATION;
}

int DataViewCtrlPaletteRenderer::HitColour(const wxPoint& point, bool range_check) const
{
    if (m_pal != nullptr && point.x >= 0)
    {
        int col_idx = point.x / (SWATCH_WIDTH + SWATCH_SEPARATION);
        const auto& palette = m_pal->GetData();
        std::size_t palette_size = palette->GetSize();
        if (!range_check || col_idx < palette_size)
        {
            return col_idx;
        }
    }
    return -1;
}

bool DataViewCtrlPaletteRenderer::SetValue(const wxVariant& value)
{
    if (value.GetAny().CheckType<void*>())
    {
        m_pal = static_cast<PaletteEntry*>(value.GetAny().As<void*>());
    }
    return true;
}

wxWindow* DataViewCtrlPaletteRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
    return nullptr;
}

bool DataViewCtrlPaletteRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
    return false;
}

void DataViewCtrlPaletteRenderer::Reset()
{
    m_pal = nullptr;
    m_sel_id = nullptr;
    m_cursor = 0;
    m_cursor_limit = 0;
}

void DataViewCtrlPaletteRenderer::MoveCursorLeft()
{
    if (m_cursor >= m_cursor_limit)
    {
        m_cursor = m_cursor_limit - 1;
    }
    else if (m_cursor > 0)
    {
        m_cursor--;
    }
}

void DataViewCtrlPaletteRenderer::MoveCursorRight()
{
    if (m_cursor < m_cursor_limit)
    {
        m_cursor++;
    }
}

int DataViewCtrlPaletteRenderer::GetCursorPosition() const
{
    if (m_pal)
    {
        if (m_cursor <= m_cursor_limit)
        {
            return m_cursor;
        }
        else
        {
            return m_cursor_limit;
        }
    }
    return -1;
}

void DataViewCtrlPaletteRenderer::SetCursorPosition(int pos)
{
    if (m_pal)
    {
        m_cursor = pos;
    }
}

int DataViewCtrlPaletteRenderer::GetCursorPaletteIndex()
{
    int pos = GetCursorPosition();
    if (m_pal && pos != -1)
    {
        return m_pal->GetData()->GetNthUnlockedIndex(m_cursor);
    }
    return -1;
}

Palette::Colour DataViewCtrlPaletteRenderer::GetCursorColour()
{
    int pos = GetCursorPosition();
    if (m_pal && pos != -1)
    {
        return m_pal->GetData()->GetNthUnlockedColour(m_cursor);
    }
    return -1;
}

bool DataViewCtrlPaletteRenderer::SetColour()
{
    const auto& palette = m_pal->GetData();
    std::size_t palette_size = palette->GetSize();
    wxColourData data;
    data.SetChooseFull(true);
    for (int i = 0; i < std::min<int>(16, palette_size); i++)
    {
        data.SetCustomColour(i, wxColor(palette->GetNthUnlockedColour(i).GetBGR(false)));
    }
    data.SetColour(wxColor(palette->GetNthUnlockedColour(m_cursor).GetBGR(false)));
    wxColourDialog dialog(m_owner, &data);
    if (dialog.ShowModal() == wxID_OK)
    {
        const auto& colour = dialog.GetColourData().GetColour();
        auto result = Palette::Colour::CreateFromBGRA(colour.GetRGBA());
        palette->SetNthUnlockedGenesisColour(m_cursor, result.GetGenesis());
    }
    return true;
}

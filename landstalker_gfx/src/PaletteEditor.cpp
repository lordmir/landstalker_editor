#include "PaletteEditor.h"

#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/colordlg.h>
#include <numeric>

wxBEGIN_EVENT_TABLE(PaletteEditor, wxWindow)
EVT_PAINT(PaletteEditor::OnPaint)
EVT_SIZE(PaletteEditor::OnSize)
EVT_LEFT_DOWN(PaletteEditor::OnMouseDown)
EVT_RIGHT_DOWN(PaletteEditor::OnMouseDown)
EVT_LEFT_DCLICK(PaletteEditor::OnDoubleClick)
EVT_MOTION(PaletteEditor::OnMouseMove)
EVT_LEAVE_WINDOW(PaletteEditor::OnMouseLeave)
EVT_ENTER_WINDOW(PaletteEditor::OnMouseEnter)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_PALETTE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_PALETTE_COLOUR_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_PALETTE_COLOUR_HOVER, wxCommandEvent);

PaletteEditor::PaletteEditor(wxWindow* parent)
	: wxWindow(parent, wxID_ANY),
	  m_pri_colour(1),
	  m_sec_colour(0),
	  m_hovered_colour(-1),
	  m_bpp(4),
	  m_disabled{},
	  m_locked{},
	  m_gd(nullptr)
{
	m_disabled.assign(16, false);
	m_indicies.assign(16, 0);
	std::iota(m_indicies.begin(), m_indicies.end(), 0);
	InitialiseBrushes();
}

PaletteEditor::~PaletteEditor()
{
	delete m_alpha_brush;
}

void PaletteEditor::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	if (m_gd == nullptr)
	{
		m_selected_palette = nullptr;
		m_selected_palette_entry = nullptr;
		m_selected_palette_name = "";
	}
}

void PaletteEditor::SelectPalette(const std::string& name)
{
	if (m_selected_palette_name != name)
	{
		m_selected_palette_name = name;
		m_selected_palette_entry = m_gd->GetPalette(m_selected_palette_name);
		m_selected_palette = m_selected_palette_entry->GetData();
		m_locked = m_selected_palette->GetLockedColours();
		m_indicies.resize(1 << m_bpp);
		std::iota(m_indicies.begin(), m_indicies.end(), 0);
		ForceRedraw();
	}
}

void PaletteEditor::SetBitsPerPixel(uint8_t bpp)
{
	if (bpp != m_bpp)
	{
		m_bpp = bpp;
		m_indicies.resize(1 << m_bpp);
		std::iota(m_indicies.begin(), m_indicies.end(), 0);
		if (m_pri_colour >= (1 << m_bpp))
		{
			SetPrimaryColour(1, true);
		}
		if (m_sec_colour >= (1 << m_bpp))
		{
			SetSecondaryColour(0, true);
		}
		ForceRedraw();
	}
}

void PaletteEditor::SetColourIndicies(const std::vector<uint8_t>& entries)
{
	if (entries != m_indicies)
	{
		std::iota(m_indicies.begin(), m_indicies.end(), 0);
		std::copy_n(entries.cbegin(), std::min(entries.size(), m_indicies.size()), m_indicies.begin());
		m_indicies = entries;
		ForceRedraw();
	}
}

void PaletteEditor::DisableEntries(const std::vector<bool>& entries)
{
	m_disabled = entries;
	if ((m_pri_colour >= 0) && (m_disabled[m_pri_colour] == true))
	{
		m_pri_colour = -1;
		for (std::size_t i = entries.size(); i > 0; --i)
		{
			if (!entries[i - 1] && m_sec_colour != (i - 1))
			{
				SetPrimaryColour(i - 1);
				break;
			}
		}
	}
	if ((m_sec_colour >= 0) && (m_disabled[m_sec_colour] == true))
	{
		m_sec_colour = m_pri_colour;
		for (std::size_t i = 0; i < entries.size(); ++i)
		{
			if (!entries[i] && m_pri_colour != i)
			{
				SetSecondaryColour(i);
				break;
			}
		}
	}
	Refresh();
}

const std::vector<bool>& PaletteEditor::GetDisabledEntries() const
{
	return m_disabled;
}

void PaletteEditor::EnableAllEntries()
{
	m_disabled.assign(16, false);
	Refresh();
}

bool PaletteEditor::SetPrimaryColour(int colour, bool send_event)
{
	bool retval = false;
	if ((colour >= 0) && (colour < 16))
	{
		if (!m_disabled[colour])
		{
			m_pri_colour = colour;
			Refresh();
			if (send_event)
			{
				FireEvent(EVT_PALETTE_COLOUR_SELECT, "");
			}
			retval = true;
		}
	}
	return retval;
}

int PaletteEditor::GetPrimaryColour() const
{
	return m_pri_colour;
}

bool PaletteEditor::SetSecondaryColour(int colour, bool send_event)
{
	bool retval = false;
	if ((colour >= 0) && (colour < 16))
	{
		if (!m_disabled[colour])
		{
			m_sec_colour = colour;
			Refresh();
			if (send_event)
			{
				FireEvent(EVT_PALETTE_COLOUR_SELECT, "");
			}
			retval = true;
		}
	}
	return retval;
}

int PaletteEditor::GetSecondaryColour() const
{
	return m_sec_colour;
}

int PaletteEditor::GetHoveredColour() const
{
	return m_hovered_colour;
}

void PaletteEditor::OnDraw(wxDC& dc)
{
	int ctrlwidth, ctrlheight;
	this->GetClientSize(&ctrlwidth, &ctrlheight);
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();
	dc.SetTextForeground(*wxLIGHT_GREY);
	dc.SetBackgroundMode(wxTRANSPARENT);
	wxFont font = dc.GetFont();
	dc.SetFont(font.MakeBold());

	auto draw_text_br = [&](const std::string& label, int x, int y, int w, int h)
	{
		int lblx = x + w;
		int lbly = y + h;
		auto extent = dc.GetTextExtent(label);
		lblx -= extent.GetWidth() + 2;
		lbly -= extent.GetHeight() + 1;
		dc.SetTextForeground(*wxWHITE);
		dc.DrawText(label, lblx - 1, lbly);
		dc.DrawText(label, lblx + 1, lbly);
		dc.DrawText(label, lblx, lbly - 1);
		dc.DrawText(label, lblx, lbly + 1);
		dc.SetTextForeground(*wxBLACK);
		dc.DrawText(label, lblx, lbly);
	};

	int sel_colour_box_width = std::max(16, std::min(ctrlwidth / 19, ctrlheight/2 - 1));

	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(GetBrush(m_sec_colour));
	dc.DrawRectangle(0, 0, sel_colour_box_width * 3 + 1, sel_colour_box_width * 2 + 1);
	draw_text_br(std::to_string(GetColour(m_sec_colour)), 0, 0, sel_colour_box_width * 3 + 1, sel_colour_box_width * 2 + 1);
	dc.SetBrush(GetBrush(m_pri_colour));
	dc.DrawRectangle(sel_colour_box_width / 2, sel_colour_box_width / 2, 4 * sel_colour_box_width / 2 + 1, sel_colour_box_width + 1);
	draw_text_br(std::to_string(GetColour(m_pri_colour)), sel_colour_box_width / 2, sel_colour_box_width / 2, 4 * sel_colour_box_width / 2 + 1, sel_colour_box_width + 1);

	dc.SetBackgroundMode(wxTRANSPARENT);

	int w = sel_colour_box_width * 2;
	int h = sel_colour_box_width;
	if (m_bpp < 4)
	{
		h <<= 1;
	}
	if (m_bpp < 3)
	{
		w <<= (3 - m_bpp);
	}
	int x_begin = 3 * sel_colour_box_width;
	int x = x_begin;
	int y = 0;
	for (int i = 0; i < (1 << m_bpp); ++i)
	{
		dc.SetBrush(GetBrush(i));
		dc.DrawRectangle(x, y, w + 1, h + 1);
		if (m_disabled[GetColour(i)] == true)
		{
			dc.DrawLine(x, y + h + 1, x + w + 1, y);
		}
		if (m_locked[GetColour(i)] == true)
		{
			draw_text_br("L", x, y, w + 1, h + 1);
		}
		x += w;
		if (i == 7)
		{
			x = x_begin;
			y += h;
		}
	}
}

void PaletteEditor::OnPaint(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void PaletteEditor::OnSize(wxSizeEvent& evt)
{
	Refresh(true);
	evt.Skip();
}

void PaletteEditor::OnMouseDown(wxMouseEvent& evt)
{
	int colour_selected = ConvertXYToColour(evt.GetPosition());
	if ((colour_selected != -1) && (!m_disabled[colour_selected]))
	{
		int* colour_to_update = nullptr;
		if (evt.GetButton() == wxMOUSE_BTN_LEFT)
		{
			colour_to_update = &m_pri_colour;
		}
		else if (evt.GetButton() == wxMOUSE_BTN_RIGHT)
		{
			colour_to_update = &m_sec_colour;
		}
		if (colour_to_update && *colour_to_update != colour_selected)
		{
			*colour_to_update = colour_selected;
			FireEvent(EVT_PALETTE_COLOUR_SELECT, "");
			Refresh();
		}
	}
	evt.Skip();
}

void PaletteEditor::OnDoubleClick(wxMouseEvent& evt)
{
	int colour_selected = ConvertXYToColour(evt.GetPosition());
	if ((colour_selected != -1) && (!m_locked[colour_selected]))
	{
		wxColourData cd;
		uint16_t orig_colour = m_selected_palette->getGenesisColour(colour_selected);
		cd.SetColour(m_selected_palette->getBGRA(colour_selected));
		cd.SetChooseFull(true);
		for (int i = 0; i < 16; ++i)
		{
			cd.SetCustomColour(i, m_selected_palette->getBGRA(i));
		}
		wxColourDialog dlg(this, &cd);
		if (dlg.ShowModal() == wxID_OK)
		{
			auto colour = dlg.GetColourData().GetColour();
			auto result = Palette::Colour::CreateFromBGRA(colour.GetRGBA());
			if (result != orig_colour)
			{
				m_selected_palette->setGenesisColour(colour_selected, result.GetGenesis());
				FireEvent(EVT_PALETTE_CHANGE, "");
				Refresh();
			}
		}
	}
	evt.Skip();
}

void PaletteEditor::OnMouseMove(wxMouseEvent& evt)
{
	OnHover(ConvertXYToColour(evt.GetPosition()));
}

void PaletteEditor::OnMouseLeave(wxMouseEvent& evt)
{
	OnHover(-1);
}

void PaletteEditor::OnMouseEnter(wxMouseEvent& evt)
{
	OnHover(ConvertXYToColour(evt.GetPosition()));
}

void PaletteEditor::ForceRedraw()
{
	Update();
	Refresh(true);
}

int PaletteEditor::ConvertXYToColour(const wxPoint& p) const
{
	int colour_selected = -1;
	int ctrlwidth, ctrlheight;
	this->GetClientSize(&ctrlwidth, &ctrlheight);
	int sel_colour_box_width = std::max(16, std::min(ctrlwidth / 19, ctrlheight / 2 - 1));

	if (p.x < 3 * sel_colour_box_width)
	{
		if ((p.x > sel_colour_box_width / 2) && (p.x < 5 * sel_colour_box_width / 2) &&
			(p.y > sel_colour_box_width / 2) && (p.y < 3 * sel_colour_box_width / 2))
		{
			return GetColour(m_pri_colour);
		}
		else if ((p.x > 0) && (p.x < 3 * sel_colour_box_width) && (p.y > 0) && (p.y < sel_colour_box_width * 2))
		{
			return GetColour(m_sec_colour);
		}
	}
	int x = (p.x - 3 * sel_colour_box_width) / (sel_colour_box_width * 2);
	int y = p.y / sel_colour_box_width;
	if ((x >= 0) && (x < 8) && (y >= 0) && (y < 2))
	{
		if (m_bpp < 4)
		{
			y = 0;
		}
		if (m_bpp < 3)
		{
			x /= (3 - m_bpp) * 2;
		}
		colour_selected = y * 8 + x;
	}
	if (colour_selected < (1 << m_bpp))
	{
		return GetColour(colour_selected);
	}
	return -1;
}

wxBrush PaletteEditor::GetBrush(int index)
{
	if (index < 0) return *wxGREY_BRUSH;
	auto colour = m_selected_palette->getBGRA(GetColour(index));
	if ((colour & 0xFF000000) == 0)
	{
		return *m_alpha_brush;
	}
	else
	{
		return wxBrush(wxColour(colour));
	}
}

void PaletteEditor::InitialiseBrushes()
{
	m_alpha_brush = new wxBrush(*wxBLACK);
	wxBitmap* stipple = new wxBitmap(6, 6);
	wxMemoryDC* imagememDC = new wxMemoryDC();
	imagememDC->SelectObject(*stipple);
	imagememDC->SetBackground(*wxGREY_BRUSH);
	imagememDC->Clear();
	imagememDC->SetBrush(*wxLIGHT_GREY_BRUSH);
	imagememDC->SetPen(*wxTRANSPARENT_PEN);
	imagememDC->DrawRectangle(0, 0, 3, 3);
	imagememDC->DrawRectangle(3, 3, 5, 5);
	imagememDC->SelectObject(wxNullBitmap);
	m_alpha_brush->SetStyle(wxBRUSHSTYLE_STIPPLE_MASK);
	m_alpha_brush->SetStipple(*stipple);
	delete stipple;
	delete imagememDC;
}

void PaletteEditor::FireEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetClientData(this);
	wxPostEvent(this->GetParent(), evt);
}

void PaletteEditor::OnHover(int colour)
{
	if (colour != m_hovered_colour)
	{
		m_hovered_colour = colour;
		FireEvent(EVT_PALETTE_COLOUR_HOVER, "");
	}
}

int PaletteEditor::GetColour(int index) const
{
	if (index < m_indicies.size())
	{
		return m_indicies[index];
	}
	return index;
}

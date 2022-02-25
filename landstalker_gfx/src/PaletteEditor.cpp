#include "PaletteEditor.h"

#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/colordlg.h>

wxBEGIN_EVENT_TABLE(PaletteEditor, wxWindow)
EVT_PAINT(PaletteEditor::OnPaint)
EVT_SIZE(PaletteEditor::OnSize)
EVT_LEFT_DOWN(PaletteEditor::OnMouseDown)
EVT_RIGHT_DOWN(PaletteEditor::OnMouseDown)
EVT_LEFT_DCLICK(PaletteEditor::OnDoubleClick)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_PALETTE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_PALETTE_COLOUR_SELECT, wxCommandEvent);

PaletteEditor::PaletteEditor(wxWindow* parent)
	: wxWindow(parent, wxID_ANY),
	  m_pri_colour(1),
	  m_sec_colour(0),
	  m_palettes(nullptr),
	  m_disabled{false},
	  m_locked{false}
{
	InitialiseBrushes();
}

PaletteEditor::~PaletteEditor()
{
	delete m_alpha_brush;
}

void PaletteEditor::SetPalettes(std::shared_ptr<std::map<std::string, Palette>> palettes)
{
	m_palettes = palettes;
	Refresh(true);
}

void PaletteEditor::SelectPalette(const std::string& name)
{
	if (m_selected_palette != name)
	{
		m_selected_palette = name;
		m_locked = GetSelectedPalette().GetLockedColours();
		Refresh();
	}
}

void PaletteEditor::DisableEntries(const std::array<bool, 16>& entries)
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

const std::array<bool, 16>& PaletteEditor::GetDisabledEntries() const
{
	return m_disabled;
}

void PaletteEditor::EnableAllEntries()
{
	m_disabled.fill(false);
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

void PaletteEditor::OnDraw(wxDC& dc)
{
	int ctrlwidth, ctrlheight;
	this->GetClientSize(&ctrlwidth, &ctrlheight);
	dc.Clear();

	int sel_colour_box_width = std::max(16, std::min(ctrlwidth / 19, ctrlheight/2 - 1));

	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(GetBrush(m_sec_colour));
	dc.DrawRectangle(0, 0, sel_colour_box_width * 3 + 1, sel_colour_box_width * 2 + 1);
	dc.SetBrush(GetBrush(m_pri_colour));
	dc.DrawRectangle(sel_colour_box_width / 2, sel_colour_box_width / 2, 4 * sel_colour_box_width / 2 + 1, sel_colour_box_width + 1);

	for (int i = 0; i < 16; ++i)
	{
		int x = 3 * sel_colour_box_width + (i % 8) * sel_colour_box_width * 2;
		int y = (i / 8) * sel_colour_box_width;

		dc.SetBrush(GetBrush(i));
		dc.DrawRectangle(x, y, sel_colour_box_width * 2 + 1, sel_colour_box_width + 1);
	}
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(*wxRED_PEN);
	dc.SetTextBackground(*wxRED);
	dc.SetTextForeground(*wxYELLOW);
	dc.SetBackgroundMode(wxSOLID);
	wxFont font = dc.GetFont();
	dc.SetFont(font.MakeBold());
	wxString label = "L";
	for (int i = 0; i < 16; ++i)
	{
		if (m_disabled[i] == true)
		{
			int x1 = 3 * sel_colour_box_width + (i % 8) * sel_colour_box_width * 2 + 1;
			int y1 = (i / 8) * sel_colour_box_width + 1;
			int x2 = x1 + sel_colour_box_width * 2;
			int y2 = y1 + sel_colour_box_width;

			dc.DrawLine(x1, y2, x2, y1);
		}
		if (m_locked[i] == true)
		{
			int x = 5 * sel_colour_box_width + (i % 8) * sel_colour_box_width * 2 - 1;
			int y = ((i / 8) + 1) * sel_colour_box_width;
			auto extent = dc.GetTextExtent(label);
			x -= extent.GetWidth();
			y -= extent.GetHeight();
			dc.DrawText(label, x, y);
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
		uint16_t orig_colour = GetSelectedPalette().getGenesisColour(colour_selected);
		cd.SetColour(GetSelectedPalette().getRGBA(colour_selected));
		cd.SetChooseFull(true);
		for (int i = 0; i < 16; ++i)
		{
			cd.SetCustomColour(i, GetSelectedPalette().getRGBA(i));
		}
		wxColourDialog dlg(this, &cd);
		if (dlg.ShowModal() == wxID_OK)
		{
			auto colour = dlg.GetColourData().GetColour();
			auto result = Palette::ToGenesisColour({ colour.Red(), colour.Green(), colour.Blue() });
			if (result != orig_colour)
			{
				GetSelectedPalette().setGenesisColour(colour_selected, result);
				FireEvent(EVT_PALETTE_CHANGE, "");
				Refresh();
			}
		}
	}
	evt.Skip();
}

Palette& PaletteEditor::GetSelectedPalette()
{
	if (m_palettes)
	{
		auto it = m_palettes->find(m_selected_palette);
		if (it != m_palettes->end())
		{
			return it->second;
		}
	}
	return m_default_palette;
}

int PaletteEditor::ConvertXYToColour(const wxPoint& p) const
{
	int colour_selected = -1;
	int ctrlwidth, ctrlheight;
	this->GetClientSize(&ctrlwidth, &ctrlheight);
	int sel_colour_box_width = std::max(16, std::min(ctrlwidth / 19, ctrlheight / 2 - 1));
	int x = (p.x - 3 * sel_colour_box_width) / (sel_colour_box_width * 2);
	int y = p.y / sel_colour_box_width;
	if ((x >= 0) && (x < 8) && (y >= 0) && (y < 2))
	{
		colour_selected = y * 8 + x;
	}
	return colour_selected;
}

wxBrush PaletteEditor::GetBrush(int index)
{
	if (index < 0) return *wxGREY_BRUSH;
	auto colour = GetSelectedPalette().getRGBA(index);
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
	m_alpha_brush = new wxBrush();
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
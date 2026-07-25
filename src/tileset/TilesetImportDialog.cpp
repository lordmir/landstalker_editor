#include <tileset/TilesetImportDialog.h>
#include <misc/LookupChoiceControl.h>

#include <wx/spinctrl.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/dcmemory.h>
#include <algorithm>

#include <landstalker/palettes/Palette.h>

namespace
{
	enum { ID_ROOM_CHOICE = wxID_HIGHEST + 1 };

	// A room palette holds 13 editable colours at indices 2-14; 0/1/15 are the fixed slots.
	constexpr int ROOM_BASE = 2;
	constexpr int ROOM_SIZE = 13;

	wxColour GenesisToWx(uint16_t genesis)
	{
		const Landstalker::Palette::Colour c(genesis);
		return wxColour(c.GetR(), c.GetG(), c.GetB());
	}
}

TilesetImportDialog::TilesetImportDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
	const Landstalker::ImageBuffer::IndexedImage& image, int tile_w, int tile_h, int bpp,
	int max_tiles, PaletteMode mode, const wxArrayString& palette_names,
	const std::string& overwrite_name, const std::array<uint16_t, 16>& current_palette)
	: wxDialog(parent, wxID_ANY, "Import Tileset from PNG", wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  m_gd(gd),
	  m_image(image),
	  m_current_palette(current_palette),
	  m_mode(mode),
	  m_tile_w(tile_w > 0 ? tile_w : 8),
	  m_tile_h(tile_h > 0 ? tile_h : 8),
	  m_bpp(bpp)
{
	m_image_palette.fill(0);
	Preprocess(palette_names);
	BuildControls(max_tiles, palette_names, overwrite_name);
	UpdatePaletteEnable();
	RebuildPreview();
	m_orig_swatch->SetBitmap(MakeSwatchBitmap(m_image_palette));
	CentreOnParent();
}

void TilesetImportDialog::Preprocess(const wxArrayString& palette_names)
{
	for (int i = 0; i < 16; ++i)
	{
		const uint32_t rgb = (static_cast<std::size_t>(i) < m_image.palette.size()) ? m_image.palette[i] : 0;
		Landstalker::Palette::Colour c;
		c.FromRGB(rgb);
		m_image_palette[i] = c.GetGenesis();
	}
	m_image_palette[0] = 0x0000;   // transparent
	m_image_palette[1] = 0x0CCC;   // fixed light grey
	m_image_palette[15] = 0x0000;  // black
	m_room_names = palette_names;
}

void TilesetImportDialog::BuildControls(int max_tiles, const wxArrayString& palette_names,
	const std::string& overwrite_name)
{
	auto* outer = new wxBoxSizer(wxVERTICAL);
	auto* top = new wxBoxSizer(wxHORIZONTAL);

	m_preview_scroll = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(460, 460),
		wxHSCROLL | wxVSCROLL | wxBORDER_SUNKEN);
	m_preview_scroll->SetScrollRate(8, 8);
	auto* preview_sizer = new wxBoxSizer(wxVERTICAL);
	m_preview_bmp = new wxStaticBitmap(m_preview_scroll, wxID_ANY, wxNullBitmap);
	preview_sizer->Add(m_preview_bmp, 0, wxALL, 0);
	m_preview_scroll->SetSizer(preview_sizer);
	top->Add(m_preview_scroll, 1, wxEXPAND | wxALL, 6);

	auto* right = new wxBoxSizer(wxVERTICAL);
	right->Add(new wxStaticText(this, wxID_ANY,
		wxString::Format("Tiles: %dx%d, %d bpp", m_tile_w, m_tile_h, m_bpp)), 0, wxALL, 6);

	auto* geo = new wxFlexGridSizer(2, 4, 4);
	geo->AddGrowableCol(1, 1);
	geo->Add(new wxStaticText(this, wxID_ANY, "Tile count"), 0, wxALIGN_CENTER_VERTICAL);
	m_count = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxSP_ARROW_KEYS, 1, std::max(1, max_tiles), std::max(1, max_tiles));
	geo->Add(m_count, 1, wxEXPAND);
	right->Add(geo, 0, wxEXPAND | wxALL, 6);

	right->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxALL, 6);

	if (m_mode == PaletteMode::RoomMatch)
	{
		right->Add(new wxStaticText(this, wxID_ANY, "Room palette"), 0, wxLEFT | wxTOP, 6);
		m_room_choice = new LookupChoiceControl(this, ID_ROOM_CHOICE, wxEmptyString, palette_names);
		right->Add(m_room_choice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
		const int match = MatchRoomPalette();
		if (!palette_names.IsEmpty())
		{
			m_room_choice->SetSelection(match >= 0 ? match : 0);
		}
		Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { RebuildPreview(); }, ID_ROOM_CHOICE);
	}
	else if (m_mode == PaletteMode::Overwrite)
	{
		m_overwrite_check = new wxCheckBox(this, wxID_ANY,
			wxString::Format("Overwrite palette '%s' from image", wxString::FromUTF8(overwrite_name)));
		right->Add(m_overwrite_check, 0, wxALL, 6);
		m_overwrite_check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { RebuildPreview(); });
	}
	else
	{
		right->Add(new wxStaticText(this, wxID_ANY,
			"No palette can be imported for a tileset below 4 bits per pixel."),
			0, wxALL, 6);
	}

	if (m_mode != PaletteMode::None)
	{
		right->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxALL, 6);
		right->Add(new wxStaticText(this, wxID_ANY, "Original palette"), 0, wxLEFT | wxTOP, 6);
		m_orig_swatch = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
		right->Add(m_orig_swatch, 0, wxLEFT | wxBOTTOM, 6);
		right->Add(new wxStaticText(this, wxID_ANY, "Selected palette"), 0, wxLEFT, 6);
		m_sel_swatch = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
		right->Add(m_sel_swatch, 0, wxLEFT | wxBOTTOM, 6);
	}
	else
	{
		// Swatches are meaningless without a palette import, but the members must exist for the
		// preview code; park them, hidden.
		m_orig_swatch = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
		m_sel_swatch = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
		m_orig_swatch->Hide();
		m_sel_swatch->Hide();
	}

	right->AddStretchSpacer(1);
	top->Add(right, 0, wxEXPAND | wxALL, 6);
	outer->Add(top, 1, wxEXPAND);
	outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 8);
	SetSizerAndFit(outer);
	SetMinSize(wxSize(720, 520));

	m_count->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { RebuildPreview(); });
	m_count->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { RebuildPreview(); });
}

void TilesetImportDialog::UpdatePaletteEnable()
{
	// Nothing to enable/disable in the current layout; kept for symmetry with the sprite dialog.
}

std::vector<uint16_t> TilesetImportDialog::ImageRoomColours() const
{
	std::vector<uint16_t> out;
	out.reserve(ROOM_SIZE);
	for (int i = 0; i < ROOM_SIZE; ++i)
	{
		out.push_back(m_image_palette[ROOM_BASE + i]);
	}
	return out;
}

int TilesetImportDialog::FindRoomPaletteIndex(const wxString& name) const
{
	const int idx = m_room_names.Index(name);
	return idx == wxNOT_FOUND ? -1 : idx;
}

int TilesetImportDialog::MatchRoomPalette() const
{
	const auto want = ImageRoomColours();
	const auto& palettes = m_gd->GetRoomData()->GetRoomPalettes();
	for (int i = 0; i < static_cast<int>(palettes.size()); ++i)
	{
		const auto pal = palettes[i]->GetData();
		bool equal = true;
		for (int n = 0; n < ROOM_SIZE; ++n)
		{
			if (pal->GetNthUnlockedColour(static_cast<uint8_t>(n)).GetGenesis() != want[n])
			{
				equal = false;
				break;
			}
		}
		if (equal)
		{
			return i;
		}
	}
	return -1;
}

std::vector<uint16_t> TilesetImportDialog::CurrentRoomColours() const
{
	if (!m_room_choice)
	{
		return ImageRoomColours();
	}
	const int idx = FindRoomPaletteIndex(m_room_choice->GetValue());
	if (idx < 0)
	{
		return ImageRoomColours();  // a typed / unmatched name means a new palette from the image
	}
	const auto pal = m_gd->GetRoomData()->GetRoomPalette(static_cast<uint8_t>(idx))->GetData();
	std::vector<uint16_t> out;
	out.reserve(ROOM_SIZE);
	for (int n = 0; n < ROOM_SIZE; ++n)
	{
		out.push_back(pal->GetNthUnlockedColour(static_cast<uint8_t>(n)).GetGenesis());
	}
	return out;
}

std::array<uint16_t, 16> TilesetImportDialog::DisplayPalette() const
{
	if (m_mode == PaletteMode::RoomMatch)
	{
		std::array<uint16_t, 16> pal{};
		pal.fill(0);
		pal[1] = 0x0CCC;
		const auto room = CurrentRoomColours();
		for (int i = 0; i < ROOM_SIZE && i < static_cast<int>(room.size()); ++i)
		{
			pal[ROOM_BASE + i] = room[i];
		}
		return pal;
	}
	if (m_mode == PaletteMode::Overwrite && m_overwrite_check && m_overwrite_check->GetValue())
	{
		return m_image_palette;
	}
	return m_current_palette;
}

int TilesetImportDialog::ChooseZoom() const
{
	const int w = static_cast<int>(m_image.width);
	const int h = static_cast<int>(m_image.height);
	int z = 1;
	while (z < 4 && w * (z + 1) <= 512 && h * (z + 1) <= 512)
	{
		++z;
	}
	return z;
}

wxBitmap TilesetImportDialog::MakeSheetBitmap(const std::array<uint16_t, 16>& pal, int zoom) const
{
	const int w = static_cast<int>(m_image.width);
	const int h = static_cast<int>(m_image.height);
	const uint8_t mask = static_cast<uint8_t>((1u << m_bpp) - 1);
	wxImage img(std::max(1, w), std::max(1, h));
	unsigned char* d = img.GetData();
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			// A 4bpp tileset masks to 0-15; clamp so a higher bit depth cannot index past the
			// 16-entry preview palette.
			const uint8_t v = static_cast<uint8_t>(std::min<uint32_t>(
				m_image.pixels[static_cast<std::size_t>(y) * w + x] & mask, 15u));
			unsigned char r, g, b;
			if (v == 0)
			{
				const bool light = (((x / 4) + (y / 4)) & 1) != 0;
				r = g = b = light ? 210 : 170;
			}
			else
			{
				const wxColour c = GenesisToWx(pal[v]);
				r = c.Red();
				g = c.Green();
				b = c.Blue();
			}
			const std::size_t o = (static_cast<std::size_t>(y) * w + x) * 3;
			d[o] = r;
			d[o + 1] = g;
			d[o + 2] = b;
		}
	}
	if (zoom > 1)
	{
		img = img.Scale(w * zoom, h * zoom, wxIMAGE_QUALITY_NEAREST);
	}
	return wxBitmap(img);
}

wxBitmap TilesetImportDialog::MakeSwatchBitmap(const std::array<uint16_t, 16>& pal) const
{
	const int sq = 16;
	wxBitmap bmp(sq * 16 + 1, sq + 1);
	wxMemoryDC dc(bmp);
	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();
	dc.SetPen(*wxBLACK_PEN);
	for (int i = 0; i < 16; ++i)
	{
		if (i == 0)
		{
			dc.SetBrush(wxBrush(wxColour(210, 210, 210)));
			dc.DrawRectangle(i * sq, 0, sq + 1, sq + 1);
			dc.SetBrush(wxBrush(wxColour(170, 170, 170)));
			dc.DrawRectangle(i * sq, 0, sq / 2, sq / 2);
			dc.DrawRectangle(i * sq + sq / 2, sq / 2, sq / 2, sq / 2);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(i * sq, 0, sq + 1, sq + 1);
		}
		else
		{
			dc.SetBrush(wxBrush(GenesisToWx(pal[i])));
			dc.DrawRectangle(i * sq, 0, sq + 1, sq + 1);
		}
	}
	dc.SelectObject(wxNullBitmap);
	return bmp;
}

void TilesetImportDialog::RebuildPreview()
{
	if (m_updating || !m_preview_bmp)
	{
		return;
	}
	m_updating = true;

	const int cols = static_cast<int>(m_image.width) / m_tile_w;
	const int rows = static_cast<int>(m_image.height) / m_tile_h;
	const int total = std::max(1, cols * rows);
	const int count = std::min(m_count->GetValue(), total);
	const auto pal = DisplayPalette();
	const int zoom = ChooseZoom();

	wxBitmap bmp = MakeSheetBitmap(pal, zoom);
	if (cols > 0 && rows > 0)
	{
		wxMemoryDC dc(bmp);
		dc.SetPen(wxPen(wxColour(0, 128, 255)));
		for (int c = 0; c <= cols; ++c)
		{
			dc.DrawLine(c * m_tile_w * zoom, 0, c * m_tile_w * zoom, rows * m_tile_h * zoom);
		}
		for (int r = 0; r <= rows; ++r)
		{
			dc.DrawLine(0, r * m_tile_h * zoom, cols * m_tile_w * zoom, r * m_tile_h * zoom);
		}
		dc.SetPen(wxPen(wxColour(220, 40, 40)));
		for (int i = count; i < total; ++i)
		{
			const int c = i % cols, r = i / cols;
			const int x0 = c * m_tile_w * zoom, y0 = r * m_tile_h * zoom;
			const int x1 = (c + 1) * m_tile_w * zoom, y1 = (r + 1) * m_tile_h * zoom;
			dc.DrawLine(x0, y0, x1, y1);
			dc.DrawLine(x0, y1, x1, y0);
		}
		dc.SelectObject(wxNullBitmap);
	}

	m_preview_bmp->SetBitmap(bmp);
	m_preview_bmp->SetSize(bmp.GetWidth(), bmp.GetHeight());
	m_preview_scroll->SetVirtualSize(bmp.GetWidth(), bmp.GetHeight());
	if (m_sel_swatch)
	{
		m_sel_swatch->SetBitmap(MakeSwatchBitmap(pal));
	}
	m_preview_scroll->Layout();
	m_updating = false;
}

int TilesetImportDialog::GetTileCount() const { return m_count->GetValue(); }

bool TilesetImportDialog::RoomCreateNew() const
{
	return m_room_choice && FindRoomPaletteIndex(m_room_choice->GetValue()) < 0;
}

int TilesetImportDialog::RoomExistingIndex() const
{
	return m_room_choice ? FindRoomPaletteIndex(m_room_choice->GetValue()) : -1;
}

std::wstring TilesetImportDialog::RoomNewName() const
{
	return m_room_choice ? m_room_choice->GetValue().ToStdWstring() : std::wstring();
}

std::vector<uint16_t> TilesetImportDialog::RoomColours() const { return ImageRoomColours(); }

bool TilesetImportDialog::Overwrite() const
{
	return m_overwrite_check && m_overwrite_check->GetValue();
}

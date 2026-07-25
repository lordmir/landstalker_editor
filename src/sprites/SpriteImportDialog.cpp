#include <sprites/SpriteImportDialog.h>
#include <misc/LookupChoiceControl.h>

#include <wx/spinctrl.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/dcmemory.h>
#include <algorithm>

#include <landstalker/palettes/Palette.h>

namespace
{
	enum
	{
		ID_LOW_CHOICE = wxID_HIGHEST + 1,
		ID_HIGH_CHOICE
	};

	// Low palette occupies indices 2-7 (6 colours), high 8-14 (7); 0/1/15 are the fixed slots.
	constexpr int LOW_BASE = 2;
	constexpr int LOW_SIZE = 6;
	constexpr int HIGH_BASE = 8;
	constexpr int HIGH_SIZE = 7;

	wxColour GenesisToWx(uint16_t genesis)
	{
		const Landstalker::Palette::Colour c(genesis);
		return wxColour(c.GetR(), c.GetG(), c.GetB());
	}
}

SpriteImportDialog::SpriteImportDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
	const Landstalker::ImageBuffer::IndexedImage& image, const std::string& suggested_name,
	int def_cell_w, int def_cell_h, int def_count, int def_origin_x, int def_origin_y,
	bool metadata_found, const std::vector<std::string>& animation_names,
	bool editable_name, const std::wstring& display_suggestion)
	: wxDialog(parent, wxID_ANY, "Import Sprite Sheet", wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  m_gd(gd),
	  m_image(image),
	  m_editable_name(editable_name)
{
	m_image_palette.fill(0);
	Preprocess();
	BuildControls(suggested_name, def_cell_w, def_cell_h, def_count, def_origin_x, def_origin_y,
		metadata_found, animation_names, editable_name, display_suggestion);
	DetectPaletteUsage();
	UpdatePaletteEnable();
	RebuildPreview();
	m_orig_swatch->SetBitmap(MakeSwatchBitmap(m_image_palette));
	CentreOnParent();
}

void SpriteImportDialog::Preprocess()
{
	// Quantise every PNG palette entry to Mega Drive depth, then force the fixed slots.
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

	for (int i = 0; i < m_gd->GetSpriteData()->GetLoPaletteCount(); ++i)
	{
		m_low_names.Add(wxString(Landstalker::SpriteData::GetSpriteLowPaletteDisplayName(static_cast<uint8_t>(i))));
	}
	for (int i = 0; i < m_gd->GetSpriteData()->GetHiPaletteCount(); ++i)
	{
		m_high_names.Add(wxString(Landstalker::SpriteData::GetSpriteHighPaletteDisplayName(static_cast<uint8_t>(i))));
	}
}

void SpriteImportDialog::BuildControls(const std::string& suggested_name, int def_cell_w,
	int def_cell_h, int def_count, int def_ox, int def_oy, bool metadata_found,
	const std::vector<std::string>& animation_names, bool editable_name,
	const std::wstring& display_suggestion)
{
	auto* outer = new wxBoxSizer(wxVERTICAL);
	auto* top = new wxBoxSizer(wxHORIZONTAL);

	// --- Left: scrollable sheet preview ---
	m_preview_scroll = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(460, 460),
		wxHSCROLL | wxVSCROLL | wxBORDER_SUNKEN);
	m_preview_scroll->SetScrollRate(8, 8);
	auto* preview_sizer = new wxBoxSizer(wxVERTICAL);
	m_preview_bmp = new wxStaticBitmap(m_preview_scroll, wxID_ANY, wxNullBitmap);
	preview_sizer->Add(m_preview_bmp, 0, wxALL, 0);
	m_preview_scroll->SetSizer(preview_sizer);
	top->Add(m_preview_scroll, 1, wxEXPAND | wxALL, 6);

	// --- Right: settings ---
	auto* right = new wxBoxSizer(wxVERTICAL);

	auto* names = new wxFlexGridSizer(2, 4, 4);
	names->AddGrowableCol(1, 1);
	names->Add(new wxStaticText(this, wxID_ANY, "Internal name"), 0, wxALIGN_CENTER_VERTICAL);
	m_internal = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(suggested_name));
	m_internal->SetMaxLength(30);
	names->Add(m_internal, 1, wxEXPAND);
	names->Add(new wxStaticText(this, wxID_ANY, "Display name"), 0, wxALIGN_CENTER_VERTICAL);
	m_display = new wxTextCtrl(this, wxID_ANY,
		display_suggestion.empty() ? wxString::FromUTF8(suggested_name) : wxString(display_suggestion));
	names->Add(m_display, 1, wxEXPAND);
	if (!editable_name)
	{
		// Replacing an existing sprite keeps its identity; show the names but do not let them change.
		m_internal->Disable();
		m_display->Disable();
	}
	right->Add(names, 0, wxEXPAND | wxALL, 6);

	// Metadata status: whether a YAML was found, and the animations it lists.
	auto* status = new wxStaticText(this, wxID_ANY,
		metadata_found ? "Metadata Loaded" : "No Metadata Found!");
	status->SetForegroundColour(metadata_found ? wxColour(0, 128, 0) : wxColour(192, 0, 0));
	wxFont status_font = status->GetFont();
	status_font.MakeBold();
	status->SetFont(status_font);
	right->Add(status, 0, wxLEFT | wxRIGHT | wxTOP, 6);

	if (metadata_found)
	{
		right->Add(new wxStaticText(this, wxID_ANY,
			wxString::Format("Animations (%d):", static_cast<int>(animation_names.size()))),
			0, wxLEFT | wxTOP, 6);
		wxArrayString anims;
		for (const auto& n : animation_names)
		{
			anims.Add(wxString::FromUTF8(n));
		}
		// Enabled so it scrolls; it is informational, so its selection is never read.
		auto* anim_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 80), anims,
			wxLB_SINGLE | wxLB_HSCROLL);
		right->Add(anim_list, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
	}

	auto* geo = new wxFlexGridSizer(2, 4, 4);
	geo->AddGrowableCol(1, 1);
	const auto add_spin = [&](const wxString& label, int value, int lo, int hi) {
		geo->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
		auto* spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			wxSP_ARROW_KEYS, lo, hi, value);
		geo->Add(spin, 1, wxEXPAND);
		return spin;
	};
	const int img_w = static_cast<int>(m_image.width);
	const int img_h = static_cast<int>(m_image.height);
	m_tile_w = add_spin("Tile width", def_cell_w > 0 ? def_cell_w : 32, 1, std::max(1, img_w));
	m_tile_h = add_spin("Tile height", def_cell_h > 0 ? def_cell_h : 32, 1, std::max(1, img_h));
	m_count = add_spin("Frame count", 1, 1, 4096);
	m_origin_x = add_spin("Origin X", def_ox, -256, 512);
	m_origin_y = add_spin("Origin Y", def_oy, -256, 512);
	right->Add(geo, 0, wxEXPAND | wxALL, 6);

	// Frame count default: the YAML value if given, otherwise every whole cell.
	{
		const int cw = m_tile_w->GetValue(), ch = m_tile_h->GetValue();
		const int total = std::max(1, (img_w / cw) * (img_h / ch));
		m_count->SetRange(1, total);
		m_count->SetValue(def_count > 0 ? std::min(def_count, total) : total);
	}

	right->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxALL, 6);

	// --- Palette selectors ---
	const auto add_palette = [&](const wxString& label, wxCheckBox*& check, LookupChoiceControl*& choice,
		int id, const wxArrayString& choices) {
		auto* row = new wxBoxSizer(wxHORIZONTAL);
		check = new wxCheckBox(this, wxID_ANY, label);
		row->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
		choice = new LookupChoiceControl(this, id, wxEmptyString, choices);
		row->Add(choice, 1, wxALIGN_CENTER_VERTICAL);
		right->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
	};
	add_palette("Use low palette", m_low_check, m_low_choice, ID_LOW_CHOICE, m_low_names);
	add_palette("Use high palette", m_high_check, m_high_choice, ID_HIGH_CHOICE, m_high_names);

	right->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxALL, 6);

	// --- Palette swatch previews ---
	right->Add(new wxStaticText(this, wxID_ANY, "Original palette"), 0, wxLEFT | wxTOP, 6);
	m_orig_swatch = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
	right->Add(m_orig_swatch, 0, wxLEFT | wxBOTTOM, 6);
	right->Add(new wxStaticText(this, wxID_ANY, "Selected palette"), 0, wxLEFT, 6);
	m_sel_swatch = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
	right->Add(m_sel_swatch, 0, wxLEFT | wxBOTTOM, 6);

	right->AddStretchSpacer(1);
	top->Add(right, 0, wxEXPAND | wxALL, 6);
	outer->Add(top, 1, wxEXPAND);
	outer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 8);
	SetSizerAndFit(outer);
	SetMinSize(wxSize(760, 560));

	// Any geometry change re-renders the preview (both arrow clicks and typed values).
	for (wxSpinCtrl* spin : { m_tile_w, m_tile_h, m_count, m_origin_x, m_origin_y })
	{
		spin->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { RebuildPreview(); });
		spin->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { RebuildPreview(); });
	}
	m_low_check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { UpdatePaletteEnable(); RebuildPreview(); });
	m_high_check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { UpdatePaletteEnable(); RebuildPreview(); });
	Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { RebuildPreview(); }, ID_LOW_CHOICE);
	Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { RebuildPreview(); }, ID_HIGH_CHOICE);
	Bind(wxEVT_BUTTON, &SpriteImportDialog::OnOk, this, wxID_OK);
}

void SpriteImportDialog::DetectPaletteUsage()
{
	// A half is "used" when any of its colours is not black after preprocessing.
	bool low_used = false;
	for (int i = 0; i < LOW_SIZE; ++i)
	{
		low_used = low_used || m_image_palette[LOW_BASE + i] != 0;
	}
	bool high_used = false;
	for (int i = 0; i < HIGH_SIZE; ++i)
	{
		high_used = high_used || m_image_palette[HIGH_BASE + i] != 0;
	}
	m_low_check->SetValue(low_used);
	m_high_check->SetValue(high_used);

	const int low_match = MatchExisting(false);
	if (!m_low_names.IsEmpty())
	{
		m_low_choice->SetSelection(low_match >= 0 ? low_match : 0);
	}
	const int high_match = MatchExisting(true);
	if (!m_high_names.IsEmpty())
	{
		m_high_choice->SetSelection(high_match >= 0 ? high_match : 0);
	}
}

void SpriteImportDialog::UpdatePaletteEnable()
{
	m_low_choice->Enable(m_low_check->GetValue());
	m_high_choice->Enable(m_high_check->GetValue());
}

std::vector<uint16_t> SpriteImportDialog::ImageHalfColours(bool high) const
{
	const int base = high ? HIGH_BASE : LOW_BASE;
	const int size = high ? HIGH_SIZE : LOW_SIZE;
	std::vector<uint16_t> out;
	out.reserve(size);
	for (int i = 0; i < size; ++i)
	{
		out.push_back(m_image_palette[base + i]);
	}
	return out;
}

int SpriteImportDialog::FindExistingPaletteIndex(bool high, const wxString& name) const
{
	const wxArrayString& names = high ? m_high_names : m_low_names;
	const int idx = names.Index(name);
	return idx == wxNOT_FOUND ? -1 : idx;
}

int SpriteImportDialog::MatchExisting(bool high) const
{
	const auto want = ImageHalfColours(high);
	const int count = high ? m_gd->GetSpriteData()->GetHiPaletteCount()
		: m_gd->GetSpriteData()->GetLoPaletteCount();
	for (int i = 0; i < count; ++i)
	{
		const auto pal = high ? m_gd->GetSpriteData()->GetHiPalette(static_cast<uint8_t>(i))
			: m_gd->GetSpriteData()->GetLoPalette(static_cast<uint8_t>(i));
		bool equal = true;
		for (int n = 0; n < static_cast<int>(want.size()); ++n)
		{
			if (pal->GetData()->GetNthUnlockedColour(static_cast<uint8_t>(n)).GetGenesis() != want[n])
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

std::vector<uint16_t> SpriteImportDialog::CurrentHalfColours(bool high) const
{
	LookupChoiceControl* choice = high ? m_high_choice : m_low_choice;
	const int idx = FindExistingPaletteIndex(high, choice->GetValue());
	if (idx < 0)
	{
		// A typed / unmatched name means a new palette taken from the image itself.
		return ImageHalfColours(high);
	}
	const auto pal = high ? m_gd->GetSpriteData()->GetHiPalette(static_cast<uint8_t>(idx))
		: m_gd->GetSpriteData()->GetLoPalette(static_cast<uint8_t>(idx));
	const int size = high ? HIGH_SIZE : LOW_SIZE;
	std::vector<uint16_t> out;
	out.reserve(size);
	for (int n = 0; n < size; ++n)
	{
		out.push_back(pal->GetData()->GetNthUnlockedColour(static_cast<uint8_t>(n)).GetGenesis());
	}
	return out;
}

std::array<uint16_t, 16> SpriteImportDialog::BuildSelectedPalette() const
{
	std::array<uint16_t, 16> pal{};
	pal.fill(0);
	pal[1] = 0x0CCC;
	pal[15] = 0x0000;
	if (m_low_check->GetValue())
	{
		const auto low = CurrentHalfColours(false);
		for (int i = 0; i < LOW_SIZE && i < static_cast<int>(low.size()); ++i)
		{
			pal[LOW_BASE + i] = low[i];
		}
	}
	if (m_high_check->GetValue())
	{
		const auto high = CurrentHalfColours(true);
		for (int i = 0; i < HIGH_SIZE && i < static_cast<int>(high.size()); ++i)
		{
			pal[HIGH_BASE + i] = high[i];
		}
	}
	return pal;
}

int SpriteImportDialog::ChooseZoom() const
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

wxBitmap SpriteImportDialog::MakeSheetBitmap(const std::array<uint16_t, 16>& pal, int zoom) const
{
	const int w = static_cast<int>(m_image.width);
	const int h = static_cast<int>(m_image.height);
	wxImage img(std::max(1, w), std::max(1, h));
	unsigned char* d = img.GetData();
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const uint8_t v = static_cast<uint8_t>(m_image.pixels[static_cast<std::size_t>(y) * w + x] & 0x0F);
			unsigned char r, g, b;
			if (v == 0)
			{
				// Transparent: a light/dark checkerboard so the shape is still legible.
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

wxBitmap SpriteImportDialog::MakeSwatchBitmap(const std::array<uint16_t, 16>& pal) const
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
			// Transparent slot: a checkerboard chip.
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

void SpriteImportDialog::RebuildPreview()
{
	if (m_updating || !m_preview_bmp)
	{
		return;
	}
	m_updating = true;

	const int cw = std::max(1, m_tile_w->GetValue());
	const int ch = std::max(1, m_tile_h->GetValue());
	const int cols = static_cast<int>(m_image.width) / cw;
	const int rows = static_cast<int>(m_image.height) / ch;
	const int total = std::max(1, cols * rows);
	m_count->SetRange(1, total);
	int count = std::min(m_count->GetValue(), total);
	if (count != m_count->GetValue())
	{
		m_count->SetValue(count);
	}
	const Landstalker::Point origin{ m_origin_x->GetValue(), m_origin_y->GetValue() };
	const auto pal = BuildSelectedPalette();
	const int zoom = ChooseZoom();

	wxBitmap bmp = MakeSheetBitmap(pal, zoom);
	if (cols > 0 && rows > 0)
	{
		wxMemoryDC dc(bmp);
		// Blue page-division gridlines.
		dc.SetPen(wxPen(wxColour(0, 128, 255)));
		for (int c = 0; c <= cols; ++c)
		{
			dc.DrawLine(c * cw * zoom, 0, c * cw * zoom, rows * ch * zoom);
		}
		for (int r = 0; r <= rows; ++r)
		{
			dc.DrawLine(0, r * ch * zoom, cols * cw * zoom, r * ch * zoom);
		}
		// Cross out the trailing, unused cells.
		dc.SetPen(wxPen(wxColour(220, 40, 40)));
		for (int i = count; i < total; ++i)
		{
			const int c = i % cols, r = i / cols;
			const int x0 = c * cw * zoom, y0 = r * ch * zoom;
			const int x1 = (c + 1) * cw * zoom, y1 = (r + 1) * ch * zoom;
			dc.DrawLine(x0, y0, x1, y1);
			dc.DrawLine(x0, y1, x1, y0);
		}
		// Green "+" at each used frame's origin.
		dc.SetPen(wxPen(wxColour(0, 200, 0)));
		for (int i = 0; i < count; ++i)
		{
			const int c = i % cols, r = i / cols;
			const int ox = (c * cw + origin.x) * zoom;
			const int oy = (r * ch + origin.y) * zoom;
			dc.DrawLine(ox - 4, oy, ox + 5, oy);
			dc.DrawLine(ox, oy - 4, ox, oy + 5);
		}
		dc.SelectObject(wxNullBitmap);
	}

	m_preview_bmp->SetBitmap(bmp);
	m_preview_bmp->SetSize(bmp.GetWidth(), bmp.GetHeight());
	m_preview_scroll->SetVirtualSize(bmp.GetWidth(), bmp.GetHeight());
	m_sel_swatch->SetBitmap(MakeSwatchBitmap(pal));
	m_preview_scroll->Layout();
	m_updating = false;
}

void SpriteImportDialog::OnOk(wxCommandEvent& evt)
{
	// Only a new sprite needs a fresh, valid name; replacing an existing one keeps its identity.
	if (m_editable_name)
	{
		const std::string name = m_internal->GetValue().ToStdString();
		if (!Landstalker::SpriteData::IsValidSpriteName(name) || m_gd->GetSpriteData()->IsSpriteNameInUse(name))
		{
			wxMessageBox("The internal name must be unique, start with a letter, contain only A-Z, "
				"a-z, 0-9 and _, and be at most 30 characters.", "Import Sprite Sheet",
				wxOK | wxICON_ERROR, this);
			return;
		}
		if (m_display->GetValue().Trim().IsEmpty())
		{
			m_display->ChangeValue(m_internal->GetValue());
		}
	}
	evt.Skip();  // let the default handler close the dialog with wxID_OK
}

int SpriteImportDialog::GetCellWidth() const { return m_tile_w->GetValue(); }
int SpriteImportDialog::GetCellHeight() const { return m_tile_h->GetValue(); }
int SpriteImportDialog::GetFrameCount() const { return m_count->GetValue(); }
Landstalker::Point SpriteImportDialog::GetOrigin() const
{
	return Landstalker::Point{ m_origin_x->GetValue(), m_origin_y->GetValue() };
}
std::string SpriteImportDialog::GetInternalName() const { return m_internal->GetValue().ToStdString(); }
std::wstring SpriteImportDialog::GetDisplayName() const
{
	const auto d = m_display->GetValue().ToStdWstring();
	return d.empty() ? m_internal->GetValue().ToStdWstring() : d;
}

SpriteImportDialog::PaletteResult SpriteImportDialog::HalfResult(bool high) const
{
	PaletteResult r;
	wxCheckBox* check = high ? m_high_check : m_low_check;
	r.used = check->GetValue();
	if (!r.used)
	{
		return r;
	}
	LookupChoiceControl* choice = high ? m_high_choice : m_low_choice;
	const wxString value = choice->GetValue();
	const int idx = FindExistingPaletteIndex(high, value);
	if (idx >= 0)
	{
		r.existing_index = idx;
	}
	else
	{
		r.create_new = true;
		r.new_name = value.ToStdWstring();
		r.colours = ImageHalfColours(high);
	}
	return r;
}

SpriteImportDialog::PaletteResult SpriteImportDialog::GetLowResult() const { return HalfResult(false); }
SpriteImportDialog::PaletteResult SpriteImportDialog::GetHighResult() const { return HalfResult(true); }

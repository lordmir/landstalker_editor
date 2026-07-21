#include <text/CharsetEditorFrame.h>

#include <algorithm>
#include <array>
#include <wx/sizer.h>

#include <landstalker/main/RomLabels.h>
#include <landstalker/misc/Utils.h>
#include <main/ImageBufferWx.h>

namespace {

enum ToolbarIDs
{
	ID_FILE_EXPORT_YML = 20000,
	ID_FILE_IMPORT_YML
};

const int GLYPH_SCALE = 2;
const int MAX_CODE = 256;

// Numeric control character values sort before named ones; BEGIN_TALK always first
bool ControlNameLess(const std::pair<std::string, int>& lhs, const std::pair<std::string, int>& rhs)
{
	if ((lhs.first == "BEGIN_TALK") != (rhs.first == "BEGIN_TALK"))
	{
		return lhs.first == "BEGIN_TALK";
	}
	if (lhs.second != rhs.second)
	{
		return lhs.second < rhs.second;
	}
	return lhs.first < rhs.first;
}

int ControlValueOf(const Landstalker::LSString::StringType& value)
{
	if (!value.empty() && std::all_of(value.begin(), value.end(),
		[](wchar_t c) { return c >= L'0' && c <= L'9'; }))
	{
		return std::stoi(value);
	}
	return -1;
}

// The character a code maps to in the given charset, or blank when it maps to nothing.
wxString LookupGlyph(const Landstalker::LSString::CharacterSet& charset, int code)
{
	if (code < 0 || code > 0xFF)
	{
		return wxString();
	}
	const auto it = charset.find(static_cast<uint8_t>(code));
	return (it == charset.cend()) ? wxString() : wxString(it->second);
}

} // namespace

CharsetEditorFrame::CharsetEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	m_notebook = new wxNotebook(this, wxID_ANY);
	m_notebook->AddPage(CreateFontPage(FontPage::MAIN), "Main Font");
	m_notebook->AddPage(CreateFontPage(FontPage::MENU), "Menu Font");
	m_notebook->AddPage(CreateFontPage(FontPage::INTRO), "Intro Font");
	m_notebook->AddPage(CreateFontPage(FontPage::CREDITS), "End Credits Font");
	m_notebook->AddPage(CreateControlCharPage(), "Control Characters");
	m_notebook->AddPage(CreateConstantPage(), "Constants");
	m_notebook->AddPage(CreateDiacriticPage(), "Diacritics");

	m_mgr.AddPane(m_notebook, wxAuiPaneInfo().CenterPane());
	m_mgr.Update();
}

CharsetEditorFrame::~CharsetEditorFrame()
{
}

wxWindow* CharsetEditorFrame::CreateFontPage(FontPage page)
{
	auto* view = new wxDataViewListCtrl(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxDV_ROW_LINES | wxDV_VERT_RULES);
	view->AppendTextColumn("Code", wxDATAVIEW_CELL_INERT, 100, wxALIGN_RIGHT);
	view->AppendBitmapColumn("Glyph", 1, wxDATAVIEW_CELL_INERT, 80, wxALIGN_CENTER);
	view->AppendTextColumn("Character(s)", wxDATAVIEW_CELL_EDITABLE, -1, wxALIGN_LEFT);
	view->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &CharsetEditorFrame::OnFontValueChanged, this);
	m_font_views[page] = view;
	return view;
}

wxWindow* CharsetEditorFrame::CreateControlCharPage()
{
	m_control_view = new wxDataViewListCtrl(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxDV_ROW_LINES | wxDV_VERT_RULES);
	m_control_view->AppendTextColumn("Name", wxDATAVIEW_CELL_INERT, 250, wxALIGN_LEFT);
	m_control_view->AppendTextColumn("Value", wxDATAVIEW_CELL_EDITABLE, -1, wxALIGN_LEFT);
	m_control_view->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &CharsetEditorFrame::OnControlValueChanged, this);
	return m_control_view;
}

wxWindow* CharsetEditorFrame::CreateConstantPage()
{
	m_constant_view = new wxDataViewListCtrl(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxDV_ROW_LINES | wxDV_VERT_RULES);
	m_constant_view->AppendTextColumn("Name", wxDATAVIEW_CELL_EDITABLE, 250, wxALIGN_LEFT);
	m_constant_view->AppendTextColumn("Value", wxDATAVIEW_CELL_EDITABLE, 80, wxALIGN_RIGHT);
	// Which charset a constant indexes depends on where the game uses it, which the name alone
	// does not tell us, so show the character its value maps to in both rather than guess. A
	// blank means the code is not a printable character in that charset - control codes, mostly.
	m_constant_view->AppendTextColumn("Main Font", wxDATAVIEW_CELL_INERT, 100, wxALIGN_LEFT);
	m_constant_view->AppendTextColumn("Menu Font", wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT);
	m_constant_view->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &CharsetEditorFrame::OnConstantValueChanged, this);
	return m_constant_view;
}

wxWindow* CharsetEditorFrame::CreateDiacriticPage()
{
	auto* panel = new wxPanel(m_notebook, wxID_ANY);
	auto* sizer = new wxBoxSizer(wxVERTICAL);
	m_diacritic_view = new wxDataViewListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxDV_ROW_LINES | wxDV_VERT_RULES);
	m_diacritic_view->AppendTextColumn("Diacritic Mark", wxDATAVIEW_CELL_EDITABLE, 150, wxALIGN_LEFT);
	m_diacritic_view->AppendTextColumn("Base Character", wxDATAVIEW_CELL_EDITABLE, 150, wxALIGN_LEFT);
	m_diacritic_view->AppendTextColumn("Combined Character", wxDATAVIEW_CELL_EDITABLE, -1, wxALIGN_LEFT);
	m_diacritic_view->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &CharsetEditorFrame::OnDiacriticValueChanged, this);
	sizer->Add(m_diacritic_view, 1, wxEXPAND | wxALL, 2);
	auto* buttons = new wxBoxSizer(wxHORIZONTAL);
	auto* add_btn = new wxButton(panel, wxID_ANY, "Add");
	auto* del_btn = new wxButton(panel, wxID_ANY, "Delete");
	add_btn->Bind(wxEVT_BUTTON, &CharsetEditorFrame::OnAddDiacritic, this);
	del_btn->Bind(wxEVT_BUTTON, &CharsetEditorFrame::OnDeleteDiacritic, this);
	buttons->Add(add_btn, 0, wxALL, 2);
	buttons->Add(del_btn, 0, wxALL, 2);
	sizer->Add(buttons, 0, wxALIGN_LEFT);
	panel->SetSizer(sizer);
	return panel;
}

bool CharsetEditorFrame::Open()
{
	if (!m_gd || !m_gd->GetStringData())
	{
		return false;
	}
	m_charsets = m_gd->GetStringData()->GetCharsets();
	Populate();
	return true;
}

void CharsetEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	EditorFrame::SetGameData(gd);
}

void CharsetEditorFrame::ClearGameData()
{
	for (auto& view : m_font_views)
	{
		view.second->DeleteAllItems();
	}
	m_control_view->DeleteAllItems();
	m_constant_view->DeleteAllItems();
	m_diacritic_view->DeleteAllItems();
	m_control_names.clear();
	m_constant_rows.clear();
	EditorFrame::ClearGameData();
}

void CharsetEditorFrame::Populate()
{
	PopulateFontPage(FontPage::MAIN);
	PopulateFontPage(FontPage::MENU);
	PopulateFontPage(FontPage::INTRO);
	PopulateFontPage(FontPage::CREDITS);
	PopulateControlChars();
	PopulateConstants();
	PopulateDiacritics();
}

Landstalker::LSString::CharacterSet& CharsetEditorFrame::GetCharsetFor(FontPage page)
{
	switch (page)
	{
	case FontPage::MENU:
		return m_charsets.menu;
	case FontPage::INTRO:
		return m_charsets.intro;
	case FontPage::CREDITS:
		return m_charsets.credits;
	case FontPage::MAIN:
	default:
		return m_charsets.main;
	}
}

std::shared_ptr<Landstalker::Tileset> CharsetEditorFrame::GetFontFor(FontPage page) const
{
	if (!m_gd)
	{
		return nullptr;
	}
	std::string name;
	switch (page)
	{
	case FontPage::MAIN:
		name = Landstalker::RomLabels::Graphics::MAIN_FONT;
		break;
	case FontPage::MENU:
		name = Landstalker::RomLabels::Graphics::INV_FONT;
		break;
	case FontPage::INTRO:
		name = Landstalker::RomLabels::Tilesets::INTRO_FONT;
		break;
	case FontPage::CREDITS:
		name = Landstalker::RomLabels::Graphics::END_CREDITS_FONT;
		break;
	}
	const auto& tilesets = m_gd->GetAllTilesets();
	auto it = tilesets.find(name);
	if (it == tilesets.end() || it->second == nullptr)
	{
		return nullptr;
	}
	return it->second->GetData();
}

std::vector<std::shared_ptr<Landstalker::Palette>> CharsetEditorFrame::GetPalettesFor(FontPage page) const
{
	if (m_gd)
	{
		std::string name;
		switch (page)
		{
		case FontPage::MAIN:
			name = Landstalker::RomLabels::Graphics::MAIN_FONT;
			break;
		case FontPage::MENU:
			name = Landstalker::RomLabels::Graphics::INV_FONT;
			break;
		case FontPage::INTRO:
			name = Landstalker::RomLabels::Tilesets::INTRO_FONT;
			break;
		case FontPage::CREDITS:
			name = Landstalker::RomLabels::Graphics::END_CREDITS_FONT;
			break;
		}
		try
		{
			const auto& tilesets = m_gd->GetAllTilesets();
			auto it = tilesets.find(name);
			if (it != tilesets.end() && it->second != nullptr)
			{
				auto pal = m_gd->GetPalette(it->second->GetDefaultPalette());
				if (pal != nullptr)
				{
					return { pal->GetData() };
				}
			}
		}
		catch (const std::exception&)
		{
		}
	}
	return { std::make_shared<Landstalker::Palette>() };
}

wxBitmap CharsetEditorFrame::RenderGlyph(const std::shared_ptr<Landstalker::Tileset>& font,
	const std::vector<std::shared_ptr<Landstalker::Palette>>& palettes, int glyph_index) const
{
	if (font == nullptr || glyph_index < 0 || glyph_index >= static_cast<int>(font->GetTileCount()))
	{
		return wxBitmap();
	}
	const int w = static_cast<int>(font->GetTileWidth());
	const int h = static_cast<int>(font->GetTileHeight());
	ImageBufferWx buf(w, h);
	buf.InsertTile(0, 0, 0, Landstalker::Tile(static_cast<uint16_t>(glyph_index)), *font);
	wxImage img = buf.MakeImage(palettes);
	img.Rescale(w * GLYPH_SCALE, h * GLYPH_SCALE);
	return wxBitmap(img);
}

int CharsetEditorFrame::FirstCode(FontPage page)
{
	return (page == FontPage::CREDITS) ? 1 : 0;
}

int CharsetEditorFrame::CodeForRow(FontPage page, int row)
{
	return row + FirstCode(page);
}

int CharsetEditorFrame::GlyphIndexForCode(FontPage page, int code)
{
	return (page == FontPage::CREDITS) ? code - 1 : code;
}

void CharsetEditorFrame::PopulateFontPage(FontPage page)
{
	auto* view = m_font_views[page];
	view->DeleteAllItems();
	if (!m_gd)
	{
		return;
	}
	const auto& charset = GetCharsetFor(page);
	auto font = GetFontFor(page);
	auto palettes = GetPalettesFor(page);
	if (font != nullptr)
	{
		view->SetRowHeight(std::max(22, static_cast<int>(font->GetTileHeight()) * GLYPH_SCALE + 4));
	}
	// List every possible code, not just those with glyphs, so codepoints without
	// a corresponding glyph (e.g. the end credit logo codes) can still be mapped.
	for (int code = FirstCode(page); code < MAX_CODE; ++code)
	{
		wxVector<wxVariant> row;
		row.push_back(wxVariant(wxString::Format("%d (0x%02X)", code, code)));
		wxVariant glyph;
		glyph << RenderGlyph(font, palettes, GlyphIndexForCode(page, code));
		row.push_back(glyph);
		auto it = charset.find(static_cast<uint8_t>(code));
		row.push_back(wxVariant(it != charset.end() ? wxString(it->second) : wxString()));
		view->AppendItem(row);
	}
}

void CharsetEditorFrame::PopulateControlChars()
{
	m_control_view->DeleteAllItems();
	m_control_names.clear();
	if (!m_gd)
	{
		return;
	}
	std::vector<std::pair<std::string, int>> ordered;
	for (const auto& entry : m_charsets.control_chars)
	{
		ordered.emplace_back(entry.first, ControlValueOf(entry.second));
	}
	std::sort(ordered.begin(), ordered.end(), ControlNameLess);
	for (const auto& entry : ordered)
	{
		m_control_names.push_back(entry.first);
		wxVector<wxVariant> row;
		row.push_back(wxVariant(wxString(entry.first)));
		row.push_back(wxVariant(wxString(m_charsets.control_chars.at(entry.first))));
		m_control_view->AppendItem(row);
	}
}

void CharsetEditorFrame::PopulateConstants()
{
	m_constant_view->DeleteAllItems();
	m_constant_rows.clear();
	if (!m_gd)
	{
		return;
	}
	for (std::size_t i = 0; i < m_charsets.constants.size(); ++i)
	{
		m_constant_rows.push_back(i);
	}
	// Listed in value order, matching the generated charset_*.inc include file.
	const auto& constants = m_charsets.constants;
	std::stable_sort(m_constant_rows.begin(), m_constant_rows.end(),
		[&constants](std::size_t lhs, std::size_t rhs)
		{
			if (constants[lhs].value != constants[rhs].value)
			{
				return constants[lhs].value < constants[rhs].value;
			}
			return constants[lhs].name < constants[rhs].name;
		});
	for (const auto index : m_constant_rows)
	{
		wxVector<wxVariant> row;
		row.push_back(wxVariant(wxString(constants[index].name)));
		row.push_back(wxVariant(wxString::Format("%d", constants[index].value)));
		row.push_back(wxVariant(LookupGlyph(m_charsets.main, constants[index].value)));
		row.push_back(wxVariant(LookupGlyph(m_charsets.menu, constants[index].value)));
		m_constant_view->AppendItem(row);
	}
}

void CharsetEditorFrame::PopulateDiacritics()
{
	m_diacritic_view->DeleteAllItems();
	if (!m_gd)
	{
		return;
	}
	std::vector<std::array<std::wstring, 3>> rows;
	for (const auto& mark : m_charsets.diacritics)
	{
		for (const auto& combo : mark.second)
		{
			rows.push_back({ mark.first, combo.first, combo.second });
		}
	}
	std::sort(rows.begin(), rows.end());
	for (const auto& entry : rows)
	{
		wxVector<wxVariant> row;
		row.push_back(wxVariant(wxString(entry[0])));
		row.push_back(wxVariant(wxString(entry[1])));
		row.push_back(wxVariant(wxString(entry[2])));
		m_diacritic_view->AppendItem(row);
	}
}

void CharsetEditorFrame::OnFontValueChanged(wxDataViewEvent& evt)
{
	auto* view = static_cast<wxDataViewListCtrl*>(evt.GetEventObject());
	auto page_it = std::find_if(m_font_views.begin(), m_font_views.end(),
		[view](const auto& entry) { return entry.second == view; });
	if (page_it == m_font_views.end() || !m_gd)
	{
		return;
	}
	int row = view->ItemToRow(evt.GetItem());
	if (row == wxNOT_FOUND)
	{
		return;
	}
	auto& charset = GetCharsetFor(page_it->first);
	// Rows are not codes: the credits page starts listing at code 1.
	const int code = CodeForRow(page_it->first, row);
	auto mapping = view->GetTextValue(row, 2).ToStdWstring();
	if (mapping.empty())
	{
		charset.erase(static_cast<uint8_t>(code));
	}
	else
	{
		charset[static_cast<uint8_t>(code)] = mapping;
	}
	ApplyToGameData();
	evt.Skip();
}

void CharsetEditorFrame::OnControlValueChanged(wxDataViewEvent& evt)
{
	if (!m_gd)
	{
		return;
	}
	int row = m_control_view->ItemToRow(evt.GetItem());
	if (row == wxNOT_FOUND || row >= static_cast<int>(m_control_names.size()))
	{
		return;
	}
	const auto& name = m_control_names[row];
	auto value = m_control_view->GetTextValue(row, 1).ToStdWstring();
	m_charsets.control_chars[name] = value;
	if (name == "STRING_BEGIN")
	{
		int eos = ControlValueOf(value);
		if (eos >= 0 && eos <= 0xFF)
		{
			m_charsets.eos_marker = static_cast<uint8_t>(eos);
		}
	}
	ApplyToGameData();
	evt.Skip();
}

void CharsetEditorFrame::OnConstantValueChanged(wxDataViewEvent& evt)
{
	if (!m_gd || m_updating)
	{
		return;
	}
	int row = m_constant_view->ItemToRow(evt.GetItem());
	if (row == wxNOT_FOUND || row >= static_cast<int>(m_constant_rows.size()))
	{
		return;
	}
	auto& constant = m_charsets.constants[m_constant_rows[row]];
	// Names are held with the "CHR_" prefix the assembler sees. The YAML drops it and puts it
	// back on load, so a name typed without one has to be normalised or the generated include
	// file would define a symbol the assembly does not reference.
	auto name = m_constant_view->GetTextValue(row, 0).ToStdString();
	if (!name.empty() && name.rfind("CHR_", 0) != 0)
	{
		name = "CHR_" + name;
	}
	if (!name.empty())
	{
		constant.name = name;
	}
	// Reject anything that is not a byte and put the old value back, rather than
	// silently writing a constant the include file would have to skip.
	long value = 0;
	if (m_constant_view->GetTextValue(row, 1).ToLong(&value) && value >= 0 && value <= 0xFF)
	{
		constant.value = static_cast<int>(value);
	}
	m_updating = true;
	m_constant_view->SetTextValue(wxString(constant.name), row, 0);
	m_constant_view->SetTextValue(wxString::Format("%d", constant.value), row, 1);
	m_constant_view->SetTextValue(LookupGlyph(m_charsets.main, constant.value), row, 2);
	m_constant_view->SetTextValue(LookupGlyph(m_charsets.menu, constant.value), row, 3);
	m_updating = false;
	// The string-begin marker doubles as the Huffman EOS marker.
	if (constant.name == "CHR_STR_BEGIN")
	{
		m_charsets.eos_marker = static_cast<uint8_t>(constant.value);
	}
	ApplyToGameData();
	evt.Skip();
}

void CharsetEditorFrame::OnDiacriticValueChanged(wxDataViewEvent& evt)
{
	if (!m_gd)
	{
		return;
	}
	RebuildDiacriticsFromGrid();
	ApplyToGameData();
	evt.Skip();
}

void CharsetEditorFrame::OnAddDiacritic(wxCommandEvent& /*evt*/)
{
	wxVector<wxVariant> row;
	row.push_back(wxVariant(wxString()));
	row.push_back(wxVariant(wxString()));
	row.push_back(wxVariant(wxString()));
	m_diacritic_view->AppendItem(row);
}

void CharsetEditorFrame::OnDeleteDiacritic(wxCommandEvent& /*evt*/)
{
	int row = m_diacritic_view->GetSelectedRow();
	if (row == wxNOT_FOUND)
	{
		return;
	}
	m_diacritic_view->DeleteItem(row);
	if (m_gd)
	{
		RebuildDiacriticsFromGrid();
		ApplyToGameData();
	}
}

void CharsetEditorFrame::RebuildDiacriticsFromGrid()
{
	m_charsets.diacritics.clear();
	for (int row = 0; row < m_diacritic_view->GetItemCount(); ++row)
	{
		auto mark = m_diacritic_view->GetTextValue(row, 0).ToStdWstring();
		auto base = m_diacritic_view->GetTextValue(row, 1).ToStdWstring();
		auto combined = m_diacritic_view->GetTextValue(row, 2).ToStdWstring();
		if (!mark.empty() && !base.empty() && !combined.empty())
		{
			m_charsets.diacritics[mark][base] = combined;
		}
	}
}

void CharsetEditorFrame::ApplyToGameData()
{
	if (m_gd && m_gd->GetStringData())
	{
		m_gd->GetStringData()->SetCharsets(m_charsets);
	}
}

void CharsetEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YML, "Export Character Set as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YML, "Import Character Set from YAML...");

	m_mgr.Update();
	UpdateUI();
}

void CharsetEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void CharsetEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_YML:
		OnExportYml();
		break;
	case ID_FILE_IMPORT_YML:
		OnImportYml();
		break;
	}
	UpdateUI();
}

void CharsetEditorFrame::OnExportYml()
{
	if (!m_gd || !m_gd->GetStringData())
	{
		return;
	}
	// Offer the name the loader looks for, so an export dropped into a disassembly's
	// metadata directory is picked up without renaming.
	const wxString default_file = wxString::Format("charset_%s.yaml",
		Landstalker::Charset::GetCharsetYamlName(m_gd->GetStringData()->GetRegion()));
	wxFileDialog fd(this, _("Export Character Set as YAML"), "", default_file,
		"YAML file (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	if (!Landstalker::Charset::SaveCharsetsToYaml(fd.GetPath().ToStdWstring(), m_charsets))
	{
		wxMessageBox("Failed to write \"" + fd.GetPath() + "\".", "Export Character Set",
			wxOK | wxICON_ERROR, this);
	}
}

void CharsetEditorFrame::OnImportYml()
{
	if (!m_gd || !m_gd->GetStringData())
	{
		return;
	}
	wxFileDialog fd(this, _("Import Character Set from YAML"), "", "",
		"YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	// A failed parse leaves the target in an unspecified state, so load into a copy and
	// only adopt it if the whole file read cleanly.
	auto imported = m_charsets;
	if (!Landstalker::Charset::LoadCharsetsFromYaml(fd.GetPath().ToStdWstring(), imported))
	{
		wxMessageBox("Could not parse \"" + fd.GetPath() + "\" as a character set YAML file.",
			"Import Character Set", wxOK | wxICON_ERROR, this);
		return;
	}
	m_charsets = imported;
	Populate();
	ApplyToGameData();
}

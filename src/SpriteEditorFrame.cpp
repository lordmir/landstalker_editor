#include "SpriteEditorFrame.h"
#include "MainFrame.h"

#include <fstream>
#include "Utils.h"

wxBEGIN_EVENT_TABLE(SpriteEditorFrame, wxWindow)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_SELECT, SpriteEditorFrame::OnTileSelected)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_SELECT, SpriteEditorFrame::OnPaletteColourSelect)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_HOVER, SpriteEditorFrame::OnPaletteColourHover)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_HOVER, SpriteEditorFrame::OnTileHovered)
EVT_COMMAND(wxID_ANY, EVT_TILE_PIXEL_HOVER, SpriteEditorFrame::OnTilePixelHover)
EVT_COMMAND(wxID_ANY, EVT_TILE_CHANGE, SpriteEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_SPRITE_FRAME_EDIT_REQUEST, SpriteEditorFrame::OnTileEditRequested)
wxEND_EVENT_TABLE()

enum MENU_IDS
{
	ID_FILE_EXPORT_FRM = 20000,
	ID_FILE_EXPORT_TILES,
	ID_FILE_EXPORT_VDPMAP,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_FRM,
	ID_FILE_IMPORT_TILES,
	ID_FILE_IMPORT_VDPMAP,
	ID_TOGGLE_GRIDLINES = 30000,
	ID_TOGGLE_TILE_NOS,
	ID_TOGGLE_ALPHA,
	ID_ADD_TILE_AFTER_SEL,
	ID_ADD_TILE_BEFORE_SEL,
	ID_EXTEND_TILESET,
	ID_DELETE_TILE,
	ID_SWAP_TILES,
	ID_CUT_TILE,
	ID_COPY_TILE,
	ID_PASTE_TILE,
	ID_EDIT_TILE
};

SpriteEditorFrame::SpriteEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	m_spriteeditor = new SpriteEditorCtrl(this);
	m_paledit = new PaletteEditor(this);
	m_tileedit = new TileEditor(this);

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.3, 0.3);
	m_mgr.AddPane(m_tileedit, wxAuiPaneInfo().Left().Layer(1).MinSize(100, 100).BestSize(450, 450).FloatingSize(450, 450).Caption("Editor"));
	m_mgr.AddPane(m_paledit, wxAuiPaneInfo().Bottom().Layer(1).MinSize(180, 40).BestSize(700, 100).FloatingSize(700, 100).Caption("Palette"));
	m_mgr.AddPane(m_spriteeditor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();

	FireEvent(EVT_STATUSBAR_UPDATE);
}

SpriteEditorFrame::~SpriteEditorFrame()
{
}

bool SpriteEditorFrame::Open(uint8_t spr, int frame, int anim, int ent)
{
	if (m_gd == nullptr)
	{
		return false;
	}
	uint8_t entity = ent == -1 ? m_gd->GetSpriteData()->GetEntitiesFromSprite(spr)[0] : ent;
	if (frame == -1 && anim == -1)
	{
		m_sprite = m_gd->GetSpriteData()->GetDefaultEntityFrame(entity);
	}
	else if (anim == -1)
	{
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(spr, frame);
	}
	else if (frame == -1)
	{
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(spr, anim, 0);
	}
	else
	{
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(spr, anim, frame);
	}
	m_palette = m_gd->GetSpriteData()->GetSpritePalette(entity);
	m_spriteeditor->Open(m_sprite->GetData(), m_palette);
	m_paledit->SelectPalette(m_palette);
	m_tileedit->SetActivePalette(m_palette);
	m_tileedit->SetTile(Tile(0));
	m_tileedit->SetTileset(m_sprite->GetData()->GetTileset());
	m_spriteeditor->SelectTile(0);
	return true;
}

void SpriteEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_tileedit->SetGameData(gd);
	m_paledit->SetGameData(gd);
}

void SpriteEditorFrame::ClearGameData()
{
	m_gd = nullptr;
	m_tileedit->SetGameData(nullptr);
	m_paledit->SetGameData(nullptr);
}

void SpriteEditorFrame::SetColour(int c)
{
}

void SpriteEditorFrame::SetActivePalette(const std::string& name)
{
	if (m_gd != nullptr)
	{
		auto pal = m_gd->GetPalette(name);
		if (pal != nullptr)
		{
			m_spriteeditor->SetActivePalette(pal->GetData());
			m_paledit->SelectPalette(pal->GetData());
			m_tileedit->SetActivePalette(pal->GetData());
		}
	}
}

void SpriteEditorFrame::Redraw() const
{
	m_spriteeditor->RedrawTiles();
	m_paledit->Refresh(true);
	m_tileedit->Redraw();
}

void SpriteEditorFrame::RedrawTiles(int index) const
{
	m_spriteeditor->RedrawTiles(index);
	m_tileedit->Redraw();
}

bool SpriteEditorFrame::Save()
{
	return m_spriteeditor->Save(m_filename, m_spriteeditor->GetCompressed());
}

bool SpriteEditorFrame::SaveAs(wxString filename, bool compressed)
{
	return m_spriteeditor->Save(filename, compressed);
}

bool SpriteEditorFrame::Open(wxString filename)
{
	return false;
}

bool SpriteEditorFrame::Open(std::vector<uint8_t>& pixels)
{
	return false;
}

bool SpriteEditorFrame::New(int r, int c)
{
	return false;
}

const std::string& SpriteEditorFrame::GetFilename() const
{
	return m_filename;
}

void SpriteEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_FRM, "Export Sprite Frame as Binary...");
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_TILES, "Export Sprite Tileset as Binary...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_VDPMAP, "Export VDP Sprite Map as CSV...");
	AddMenuItem(fileMenu, 3, ID_FILE_EXPORT_PNG, "Export Sprite as PNG...");
	AddMenuItem(fileMenu, 4, ID_FILE_IMPORT_FRM, "Import Sprite Frame from Binary...");
	AddMenuItem(fileMenu, 5, ID_FILE_IMPORT_TILES, "Import Sprite Tileset from Binary...");
	AddMenuItem(fileMenu, 6, ID_FILE_IMPORT_VDPMAP, "Import VDP Sprite Map from CSV...");

	UpdateUI();

	m_mgr.Update();
}

void SpriteEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_FRM:
		OnExportFrm();
		break;
	case ID_FILE_EXPORT_TILES:
		OnExportTiles();
		break;
	case ID_FILE_EXPORT_VDPMAP:
		OnExportVdpSpritemap();
		break;
	case ID_FILE_EXPORT_PNG:
		OnExportPng();
		break;
	case ID_FILE_IMPORT_FRM:
		OnImportFrm();
		break;
	case ID_FILE_IMPORT_TILES:
		OnImportTiles();
		break;
	case ID_FILE_IMPORT_VDPMAP:
		OnImportVdpSpritemap();
		break;
	default:
		break;
	}
	UpdateUI();
	FireEvent(EVT_STATUSBAR_UPDATE);
	FireEvent(EVT_PROPERTIES_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::ExportFrm(const std::string& filename) const
{
	auto bytes = m_sprite->GetData()->GetBits();
	WriteBytes(bytes, filename);
}

void SpriteEditorFrame::ExportTiles(const std::string& filename) const
{
	auto bytes = m_sprite->GetData()->GetTileset()->GetBits(false);
	WriteBytes(bytes, filename);
}

void SpriteEditorFrame::ExportVdpSpritemap(const std::string& filename) const
{
	std::ofstream fs(filename, std::ios::out | std::ios::trunc);
	fs << (m_sprite->GetData()->GetCompressed() ? "1" : "0") << std::endl;
	for (std::size_t i = 0; i < m_sprite->GetData()->GetSubSpriteCount(); ++i)
	{
		auto data = m_sprite->GetData()->GetSubSprite(i);
		fs << StrPrintf("% 03d,% 03d,%01d,%01d\n", data.x, data.y, data.w, data.h);
	}
}

void SpriteEditorFrame::ExportPng(const std::string& filename) const
{
	int width = m_sprite->GetData()->GetWidth();
	int height = m_sprite->GetData()->GetHeight();
	ImageBuffer buf(width, height);
	buf.InsertSprite(-m_sprite->GetData()->GetLeft(), -m_sprite->GetData()->GetTop(), 0, *m_sprite->GetData());
	buf.WritePNG(filename, { m_palette }, true);
}

void SpriteEditorFrame::ImportFrm(const std::string& filename)
{
	auto bytes = ReadBytes(filename);
	m_sprite->GetData()->SetBits(bytes);
	RedrawTiles();
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void SpriteEditorFrame::ImportTiles(const std::string& filename)
{
	auto bytes = ReadBytes(filename);
	m_sprite->GetData()->GetTileset()->SetBits(bytes, false);
	RedrawTiles();
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void SpriteEditorFrame::ImportVdpSpritemap(const std::string& filename)
{
	std::ifstream fs(filename, std::ios::in);
	bool compressed = false;
	std::vector<SpriteFrame::SubSprite> subs;
	std::string row;
	std::string cell;
	std::getline(fs, row);
	if (row.size() > 0)
	{
		compressed = row[0] == '1';
	}
	while (std::getline(fs, row))
	{
		uint32_t x, y, w, h;
		std::istringstream ss(row);
		std::getline(ss, cell, ',');
		StrToInt(cell, x);
		std::getline(ss, cell, ',');
		StrToInt(cell, y);
		std::getline(ss, cell, ',');
		StrToInt(cell, w);
		std::getline(ss, cell, ',');
		StrToInt(cell, h);
		subs.emplace_back(x, y, w, h);
	}
	m_sprite->GetData()->SetCompressed(compressed);
	m_sprite->GetData()->SetSubSprites(subs);

	RedrawTiles();
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void SpriteEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTileHovered(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTileSelected(wxCommandEvent& evt)
{
	auto tile = std::stoi(evt.GetString().ToStdString());
	if (tile != -1)
	{
		m_tileedit->SetTile(tile);
	}
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTileChanged(wxCommandEvent& evt)
{
	auto tile = std::stoi(evt.GetString().ToStdString());
	m_spriteeditor->RedrawTiles(tile);
	evt.Skip();
}

void SpriteEditorFrame::OnTileEditRequested(wxCommandEvent& evt)
{
	evt.Skip();
}

void SpriteEditorFrame::OnButtonClicked(wxCommandEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_TOGGLE_GRIDLINES:
		m_spriteeditor->SetBordersEnabled(!m_spriteeditor->GetBordersEnabled());
		break;
	case ID_TOGGLE_TILE_NOS:
		m_spriteeditor->SetTileNumbersEnabled(!m_spriteeditor->GetTileNumbersEnabled());
		break;
	case ID_TOGGLE_ALPHA:
		m_spriteeditor->SetAlphaEnabled(!m_spriteeditor->GetAlphaEnabled());
		break;
	case ID_ADD_TILE_AFTER_SEL:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->InsertTileAfter(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_ADD_TILE_BEFORE_SEL:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->InsertTileBefore(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_EXTEND_TILESET:
		m_spriteeditor->InsertTileAfter(m_spriteeditor->GetTilemapSize() - 1);
		break;
	case ID_DELETE_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->DeleteTileAt(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_SWAP_TILES:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->SwapTile(m_spriteeditor->GetSelectedTile());
		}
		break;
	case ID_CUT_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->CutTile(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_COPY_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->CopyTile(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_PASTE_TILE:
		if (m_spriteeditor->IsSelectionValid() && !m_spriteeditor->IsClipboardEmpty())
		{
			m_spriteeditor->PasteTile(m_spriteeditor->GetSelectedTile().GetIndex());
		}
		break;
	case ID_EDIT_TILE:
		if (m_spriteeditor->IsSelectionValid())
		{
			m_spriteeditor->EditTile(m_spriteeditor->GetSelectedTile());
		}
		break;
	default:
		break;
	}
	evt.Skip();
}

void SpriteEditorFrame::OnPaletteColourSelect(wxCommandEvent& evt)
{
	m_tileedit->SetPrimaryColour(m_paledit->GetPrimaryColour());
	m_tileedit->SetSecondaryColour(m_paledit->GetSecondaryColour());
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnPaletteColourHover(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnTilePixelHover(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void SpriteEditorFrame::OnExportFrm()
{
	const wxString default_file = m_sprite->GetName() + ".frm";
	wxFileDialog fd(this, _("Export Sprite As Binary"), "", default_file, "Compressed Sprite Frame (*.frm)|*.frm|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportFrm(fd.GetPath().ToStdString());
	}
}

void SpriteEditorFrame::OnExportTiles()
{
	const wxString default_file = m_sprite->GetName() + ".bin";
	wxFileDialog fd(this, _("Export Sprite Tiles As Binary"), "", default_file, "Sprite Tiles (*.bin)|*.bin|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportTiles(fd.GetPath().ToStdString());
	}
}

void SpriteEditorFrame::OnExportVdpSpritemap()
{
	const wxString default_file = m_sprite->GetName() + ".csv";
	wxFileDialog fd(this, _("Export Sprite VDP Spritemap As CSV"), "", default_file, "Comma-Separated Values file (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportVdpSpritemap(fd.GetPath().ToStdString());
	}
}

void SpriteEditorFrame::OnExportPng()
{
	const wxString default_file = m_sprite->GetName() + ".png";
	wxFileDialog fd(this, _("Export Sprite As PNG"), "", default_file, "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportPng(fd.GetPath().ToStdString());
	}
}

void SpriteEditorFrame::OnImportFrm()
{
	wxFileDialog fd(this, _("Import Sprite From FRM"), "", "", "FRM File (*.frm)|*.frm|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportFrm(path);
	}
	m_spriteeditor->SelectTile(0);
	m_tileedit->SetTile(Tile(0));
	m_tileedit->SetTileset(m_sprite->GetData()->GetTileset());
	RedrawTiles();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

void SpriteEditorFrame::OnImportTiles()
{
	wxFileDialog fd(this, _("Import Sprite Tiles From BIN"), "", "", "BIN File (*.bin)|*.bin|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportTiles(path);
	}
	m_spriteeditor->SelectTile(0);
	m_tileedit->SetTile(Tile(0));
	m_tileedit->SetTileset(m_sprite->GetData()->GetTileset());
	RedrawTiles();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

void SpriteEditorFrame::OnImportVdpSpritemap()
{
	wxFileDialog fd(this, _("Import Sprite VDP Spritemap From CSV"), "", "", "Comma-Separated Values file (*.csv)|*.csv|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportVdpSpritemap(path);
	}
	m_spriteeditor->SelectTile(0);
	m_tileedit->SetTile(Tile(0));
	m_tileedit->SetTileset(m_sprite->GetData()->GetTileset());
	RedrawTiles();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

void SpriteEditorFrame::InitStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(3);
	status.SetStatusText("", 0);
	status.SetStatusText("", 1);
	status.SetStatusText("", 2);
}

void SpriteEditorFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const
{
	std::ostringstream ss;
	int colour = m_paledit->GetHoveredColour();
	if (m_spriteeditor->IsSelectionValid())
	{
		ss << "Tile: " << m_spriteeditor->GetSelectedTile().GetIndex();
		if (m_tileedit->IsHoverValid())
		{
			const auto selection = m_tileedit->GetHoveredPixel();
			int idx = m_tileedit->GetColourAtPixel(selection);
			colour = m_tileedit->GetColour(idx);
			ss << ": (" << selection.x << ", " << selection.y << "): " << idx;
		}
	}
	status.SetStatusText(ss.str(), 0);
	ss.str(std::string());
	if (colour != -1)
	{
		const auto& name = m_palette->getOwner(colour);
		ss << StrPrintf("Colour at mouse: Index %d - Genesis 0x%04X, RGB #%06X %s", colour,
			m_palette->getGenesisColour(colour), m_palette->getRGB(colour), m_palette->getA(colour) == 0 ? " [Transparent]" : "");
		if (!name.empty())
		{
			ss << ", Palette: \"" << name << "\"";
		}
	}
	else if (m_spriteeditor->IsHoverValid())
	{
		ss << "Cursor at tile " << m_spriteeditor->GetHoveredTile().GetIndex();
	}
	status.SetStatusText(ss.str(), 1);
}

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

bool SpriteEditorFrame::Open(uint8_t entity, int frame, int anim)
{
	if (m_gd == nullptr)
	{
		return false;
	}
	uint8_t spr = m_gd->GetSpriteData()->GetSpriteFromEntity(entity);
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

void SpriteEditorFrame::InitStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(3);
	status.SetStatusText("", 0);
	status.SetStatusText("", 1);
	status.SetStatusText("", 2);
}

void SpriteEditorFrame::UpdateStatusBar(wxStatusBar& status) const
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

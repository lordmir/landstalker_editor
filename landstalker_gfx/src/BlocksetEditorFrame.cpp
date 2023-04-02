#include "BlocksetEditorFrame.h"

wxBEGIN_EVENT_TABLE(BlocksetEditorFrame, wxWindow)
EVT_SLIDER(wxID_ANY, BlocksetEditorFrame::OnZoomChange)
EVT_TOOL(wxID_ANY, BlocksetEditorFrame::OnButtonClicked)
wxEND_EVENT_TABLE()

enum MENU_IDS
{
	ID_TOGGLE_GRIDLINES = 30000,
	ID_TOGGLE_TILE_NUMBERS,
	ID_TOGGLE_ALPHA,
	ID_INSERT_BLOCK_BEFORE,
	ID_INSERT_BLOCK_AFTER,
	ID_DELETE_BLOCK,
	ID_HFLIP_TILE,
	ID_VFLIP_TILE,
	ID_TOGGLE_TILE_PRIORITY,
	ID_BLOCK_SELECT,
	ID_TILE_SELECT,
	ID_PENCIL
};

BlocksetEditorFrame::BlocksetEditorFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY)
{
	m_mgr.SetManagedWindow(this);

	m_editor = new BlocksetEditorCtrl(this);
	m_tileset = new TilesetEditor(this);

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.4, 0.4);
	m_mgr.AddPane(m_tileset, wxAuiPaneInfo().Right().Layer(1).MinSize(100, 100).BestSize(450, 450).FloatingSize(450, 450).Caption("Tiles"));
	m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();
}

BlocksetEditorFrame::~BlocksetEditorFrame()
{
}

void BlocksetEditorFrame::Open(const std::string& blockset_name)
{
	auto bs = m_gd->GetRoomData()->GetBlockset(blockset_name);
	auto ts = m_gd->GetRoomData()->GetTileset(bs->GetTileset());
	m_editor->Open(blockset_name);
	m_tileset->Open(ts->GetData());
	m_tileset->SetActivePalette(ts->GetDefaultPalette());
}

void BlocksetEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_editor->SetGameData(gd);
	m_tileset->SetGameData(gd);
	m_gd = gd;
}

void BlocksetEditorFrame::ClearGameData()
{
	m_gd = nullptr;
}

void BlocksetEditorFrame::SetActivePalette(const std::string& name)
{
	m_editor->SetActivePalette(name);
	m_tileset->SetActivePalette(name);
}

void BlocksetEditorFrame::SetDrawTile(const Tile& tile)
{
	m_editor->SetDrawTile(tile);
	UpdateStatusBar();
}

void BlocksetEditorFrame::Redraw()
{
	m_editor->RedrawTiles();
}

void BlocksetEditorFrame::RedrawTiles(int index) const
{
	m_editor->RedrawTiles(index);
}

void BlocksetEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	UpdateStatusBar();
	evt.Skip();
}

void BlocksetEditorFrame::OnButtonClicked(wxCommandEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_TOGGLE_GRIDLINES:
		m_editor->SetBordersEnabled(!m_editor->GetBordersEnabled());
		break;
	case ID_TOGGLE_TILE_NUMBERS:
		m_editor->SetTileNumbersEnabled(!m_editor->GetTileNumbersEnabled());
		break;
	case ID_TOGGLE_ALPHA:
		m_editor->SetAlphaEnabled(!m_editor->GetAlphaEnabled());
		break;
	case ID_INSERT_BLOCK_BEFORE:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->InsertBlock(m_editor->GetBlockSelection().x);
		}
		break;
	case ID_INSERT_BLOCK_AFTER:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->InsertBlock(m_editor->GetBlockSelection().x + 1);
		}
		break;
	case ID_DELETE_BLOCK:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->DeleteBlock(m_editor->GetBlockSelection().x);
		}
		break;
	case ID_HFLIP_TILE:
		if (m_editor->IsTileSelectionValid())
		{
			auto tile = m_editor->GetSelectedTile();
			tile.Attributes().toggleAttribute(TileAttributes::ATTR_HFLIP);
			m_editor->SetSelectedTile(tile);
		}
		break;
	case ID_VFLIP_TILE:
		if (m_editor->IsTileSelectionValid())
		{
			auto tile = m_editor->GetSelectedTile();
			tile.Attributes().toggleAttribute(TileAttributes::ATTR_VFLIP);
			m_editor->SetSelectedTile(tile);
		}
		break;
	case ID_TOGGLE_TILE_PRIORITY:
		if (m_editor->IsTileSelectionValid())
		{
			auto tile = m_editor->GetSelectedTile();
			tile.Attributes().toggleAttribute(TileAttributes::ATTR_PRIORITY);
			m_editor->SetSelectedTile(tile);
		}
		break;
	case ID_BLOCK_SELECT:
		m_toolbar->ToggleTool(ID_BLOCK_SELECT, true);
		m_editor->SetMode(BlocksetEditorCtrl::Mode::BLOCK_SELECT);
		break;
	case ID_TILE_SELECT:
		m_toolbar->ToggleTool(ID_TILE_SELECT, true);
		m_editor->SetMode(BlocksetEditorCtrl::Mode::TILE_SELECT);
		break;
	case ID_PENCIL:
		m_toolbar->ToggleTool(ID_PENCIL, true);
		m_editor->SetMode(BlocksetEditorCtrl::Mode::PENCIL);
		break;
	default:
		break;
	}
	UpdateStatusBar();
	evt.Skip();
}

void BlocksetEditorFrame::UpdateStatusBar()
{
	std::ostringstream ss;
	m_statusbar->SetStatusText(wxString::Format("Zoom: %d%s", m_zoomslider->GetValue(), ss.str()), 1);
	m_editor->SetPixelSize(m_zoomslider->GetValue());
}

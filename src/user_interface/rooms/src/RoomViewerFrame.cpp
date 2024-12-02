#include <user_interface/rooms/include/RoomViewerFrame.h>

#include <fstream>
#include <sstream>
#include <wx/busyinfo.h>
#include <wx/dir.h>
#include <user_interface/rooms/include/FlagDialog.h>
#include <user_interface/rooms/include/ChestDialog.h>
#include <user_interface/rooms/include/CharacterDialog.h>
#include <user_interface/rooms/include/RoomErrorDialog.h>
#include <user_interface/rooms/include/TileSwapDialog.h>
#include <landstalker/misc/include/Labels.h>
#include <landstalker/3d_maps/include/MapToTmx.h>
#include <landstalker/3d_maps/include/RoomToTmx.h>
#include <user_interface/rooms/include/RoomViewerCtrl.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_BIN = 20000,
	ID_FILE_EXPORT_CSV,
	ID_FILE_EXPORT_ALL_CSV,
	ID_FILE_EXPORT_TMX,
	ID_FILE_EXPORT_ALL_TMX,
	ID_FILE_EXPORT_PNG,
	ID_FILE_EXPORT_ROOM_TMX,
	ID_FILE_EXPORT_ALL_ROOMS_TMX,
	ID_FILE_SEP1,
	ID_FILE_IMPORT_BIN,
	ID_FILE_IMPORT_CSV,
	ID_FILE_IMPORT_TMX,
	ID_FILE_IMPORT_ALL_TMX,
	ID_EDIT,
	ID_EDIT_ENTITY_PROPERTIES,
	ID_EDIT_FLAGS,
	ID_EDIT_CHESTS,
	ID_EDIT_DIALOGUE,
	ID_EDIT_TILESWAPS,
	ID_TOOLS,
	ID_TOOLS_LAYERS,
	ID_TOOLS_ENTITIES,
	ID_TOOLS_WARPS,
	ID_TOOLS_SWAPS,
	ID_TOOLS_BLOCKS,
	ID_VIEW,
	ID_VIEW_ALPHA,
	ID_VIEW_ENTITIES,
	ID_VIEW_ENTITY_HITBOX,
	ID_VIEW_WARPS,
	ID_VIEW_SWAPS,
	ID_VIEW_SEP1,
	ID_VIEW_ERRORS
};

enum TOOL_IDS
{
	TOOL_TOGGLE_ALPHA = 30000,
	TOOL_TOGGLE_ENTITIES,
	TOOL_TOGGLE_ENTITY_HITBOX,
	TOOL_TOGGLE_WARPS,
	TOOL_TOGGLE_SWAPS,
	TOOL_SHOW_LAYERS_PANE,
	TOOL_SHOW_ENTITIES_PANE,
	TOOL_SHOW_WARPS_PANE,
	TOOL_SHOW_TILESWAP_PANE,
	TOOL_SHOW_BLOCKS_PANE,
	TOOL_SHOW_FLAGS,
	TOOL_SHOW_CHESTS,
	TOOL_SHOW_DIALOGUE,
	TOOL_SHOW_TILESWAPS,
	TOOL_SHOW_SELECTION_PROPERTIES,
	TOOL_SHOW_ERRORS,
	HM_INSERT_ROW_BEFORE,
	HM_INSERT_ROW_AFTER,
	HM_DELETE_ROW,
	HM_INSERT_COLUMN_BEFORE,
	HM_INSERT_COLUMN_AFTER,
	HM_DELETE_COLUMN,
	HM_TYPE_DROPDOWN,
	HM_TOGGLE_PLAYER,
	HM_TOGGLE_NPC,
	HM_TOGGLE_RAFT,
	HM_INCREASE_HEIGHT,
	HM_DECREASE_HEIGHT,
	HM_NUDGE_HM_NE,
	HM_NUDGE_HM_NW,
	HM_NUDGE_HM_SE,
	HM_NUDGE_HM_SW,
	HM_ZOOM,
	TM_CLEAR,
	TM_INSERT_ROW_BEFORE,
	TM_INSERT_ROW_AFTER,
	TM_DELETE_ROW,
	TM_INSERT_COLUMN_BEFORE,
	TM_INSERT_COLUMN_AFTER,
	TM_DELETE_COLUMN
};

wxBEGIN_EVENT_TABLE(RoomViewerFrame, wxWindow)
EVT_KEY_DOWN(RoomViewerFrame::OnKeyDown)
EVT_KEY_UP(RoomViewerFrame::OnKeyUp)
EVT_COMMAND(wxID_ANY, EVT_ZOOM_CHANGE, RoomViewerFrame::OnZoomChange)
EVT_COMMAND(wxID_ANY, EVT_OPACITY_CHANGE, RoomViewerFrame::OnOpacityChange)
EVT_COMMAND(wxID_ANY, EVT_ENTITY_UPDATE, RoomViewerFrame::OnEntityUpdate)
EVT_COMMAND(wxID_ANY, EVT_ENTITY_SELECT, RoomViewerFrame::OnEntitySelect)
EVT_COMMAND(wxID_ANY, EVT_ENTITY_OPEN_PROPERTIES, RoomViewerFrame::OnEntityOpenProperties)
EVT_COMMAND(wxID_ANY, EVT_ENTITY_ADD, RoomViewerFrame::OnEntityAdd)
EVT_COMMAND(wxID_ANY, EVT_ENTITY_DELETE, RoomViewerFrame::OnEntityDelete)
EVT_COMMAND(wxID_ANY, EVT_ENTITY_MOVE_UP, RoomViewerFrame::OnEntityMoveUp)
EVT_COMMAND(wxID_ANY, EVT_ENTITY_MOVE_DOWN, RoomViewerFrame::OnEntityMoveDown)
EVT_COMMAND(wxID_ANY, EVT_WARP_UPDATE, RoomViewerFrame::OnWarpUpdate)
EVT_COMMAND(wxID_ANY, EVT_WARP_SELECT, RoomViewerFrame::OnWarpSelect)
EVT_COMMAND(wxID_ANY, EVT_WARP_OPEN_PROPERTIES, RoomViewerFrame::OnWarpOpenProperties)
EVT_COMMAND(wxID_ANY, EVT_WARP_ADD, RoomViewerFrame::OnWarpAdd)
EVT_COMMAND(wxID_ANY, EVT_WARP_DELETE, RoomViewerFrame::OnWarpDelete)
EVT_COMMAND(wxID_ANY, EVT_HEIGHTMAP_UPDATE, RoomViewerFrame::OnHeightmapUpdate)
EVT_COMMAND(wxID_ANY, EVT_HEIGHTMAP_MOVE, RoomViewerFrame::OnHeightmapMove)
EVT_COMMAND(wxID_ANY, EVT_HEIGHTMAP_CELL_SELECTED, RoomViewerFrame::OnHeightmapSelect)
EVT_COMMAND(wxID_ANY, EVT_BLOCK_SELECT, RoomViewerFrame::OnBlockSelect)
EVT_COMMAND(wxID_ANY, EVT_MAPLAYER_UPDATE, RoomViewerFrame::OnMapUpdate)
EVT_COMMAND(wxID_ANY, EVT_MAPLAYER_CELL_SELECT, RoomViewerFrame::OnMapCellSelect)
EVT_COMMAND(wxID_ANY, EVT_TILESWAP_UPDATE, RoomViewerFrame::OnSwapUpdate)
EVT_COMMAND(wxID_ANY, EVT_TILESWAP_SELECT, RoomViewerFrame::OnSwapSelect)
EVT_COMMAND(wxID_ANY, EVT_TILESWAP_ADD, RoomViewerFrame::OnSwapAdd)
EVT_COMMAND(wxID_ANY, EVT_TILESWAP_DELETE, RoomViewerFrame::OnSwapDelete)
EVT_COMMAND(wxID_ANY, EVT_TILESWAP_MOVE_UP, RoomViewerFrame::OnSwapMoveUp)
EVT_COMMAND(wxID_ANY, EVT_TILESWAP_MOVE_DOWN, RoomViewerFrame::OnSwapMoveDown)
EVT_COMMAND(wxID_ANY, EVT_TILESWAP_OPEN_PROPERTIES, RoomViewerFrame::OnSwapProperties)
EVT_COMMAND(wxID_ANY, EVT_DOOR_UPDATE, RoomViewerFrame::OnDoorUpdate)
EVT_COMMAND(wxID_ANY, EVT_DOOR_SELECT, RoomViewerFrame::OnDoorSelect)
EVT_COMMAND(wxID_ANY, EVT_DOOR_ADD, RoomViewerFrame::OnDoorAdd)
EVT_COMMAND(wxID_ANY, EVT_DOOR_DELETE, RoomViewerFrame::OnDoorDelete)
EVT_COMMAND(wxID_ANY, EVT_DOOR_MOVE_UP, RoomViewerFrame::OnDoorMoveUp)
EVT_COMMAND(wxID_ANY, EVT_DOOR_MOVE_DOWN, RoomViewerFrame::OnDoorMoveDown)
EVT_COMMAND(wxID_ANY, EVT_DOOR_OPEN_PROPERTIES, RoomViewerFrame::OnDoorProperties)
EVT_SLIDER(HM_ZOOM, RoomViewerFrame::OnHMZoom)
EVT_CHOICE(HM_TYPE_DROPDOWN, RoomViewerFrame::OnHMTypeSelect)
EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, RoomViewerFrame::OnTabChange)
EVT_SIZE(RoomViewerFrame::OnSize)
wxEND_EVENT_TABLE()

RoomViewerFrame::RoomViewerFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst),
	  m_mode(RoomEdit::Mode::NORMAL),
	  m_nb(nullptr),
	  m_title(""),
	  m_roomview(nullptr),
	  m_hmedit(nullptr),
	  m_bgedit(nullptr),
	  m_fgedit(nullptr),
	  m_layerctrl(nullptr),
	  m_entityctrl(nullptr),
	  m_warpctrl(nullptr),
	  m_swapctrl(nullptr),
	  m_blkctrl(nullptr),
	  m_g(nullptr),
	  m_roomnum(0),
	  m_zoom(1.0),
	  m_layerctrl_visible(true),
	  m_entityctrl_visible(true),
	  m_warpctrl_visible(true),
	  m_swapctrl_visible(true),
	  m_blkctrl_visible(true),
	  m_sizes_set(false),
	  m_reset_props(false)
{
	m_mgr.SetManagedWindow(this);

	m_layerctrl = new LayerControlFrame(this);
	m_entityctrl = new EntityControlFrame(this, GetImageList());
	m_warpctrl = new WarpControlFrame(this, GetImageList());
	m_swapctrl = new TileSwapControlFrame(this, GetImageList());
	m_blkctrl = new BlocksetEditorCtrl(this);
	m_mgr.AddPane(m_layerctrl, wxAuiPaneInfo().Right().Layer(2).Resizable(false).MinSize(220, 200)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Layers"));
	m_mgr.AddPane(m_entityctrl, wxAuiPaneInfo().Right().Layer(2).Resizable(false).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Entities"));
	m_mgr.AddPane(m_warpctrl, wxAuiPaneInfo().Right().Layer(2).Resizable(false).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Warps"));
	m_mgr.AddPane(m_swapctrl, wxAuiPaneInfo().Right().Layer(2).Resizable(false).MinSize(220, 150)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Swaps"));
	m_mgr.AddPane(m_blkctrl, wxAuiPaneInfo().Bottom().Layer(1).Movable(true).Resizable(true).MinSize(400, 200)
		.BestSize(800, 300).FloatingSize(400, 400).Caption("Blocks"));
	m_nb = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TAB_SPLIT);
	m_nb->Freeze();
	m_roomview = new RoomViewerCtrl(m_nb, this);
	m_hmedit = new HeightmapEditorCtrl(m_nb, this);
	m_bgedit = new Map3DEditor(m_nb, this, Tilemap3D::Layer::BG);
	m_fgedit = new Map3DEditor(m_nb, this, Tilemap3D::Layer::FG);
	m_nb->AddPage(m_roomview, "Room", true);
	m_nb->AddPage(m_hmedit, "Heightmap");
	m_nb->AddPage(m_bgedit, "Background");
	m_nb->AddPage(m_fgedit, "Foreground");
	m_nb->Thaw();
	m_mgr.AddPane(m_nb, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();

	m_roomview->SetZoom(m_layerctrl->GetZoom());
	m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::BACKGROUND1, m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::BG1));
	m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::BACKGROUND2, m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::BG2));
	m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::BG_SPRITES, m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::SPRITES));
	m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::FOREGROUND, m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::FG));
	m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::FG_SPRITES, m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::SPRITES));
	m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::HEIGHTMAP, m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::HM));
	this->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_roomview->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_entityctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_warpctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_swapctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_hmedit->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_bgedit->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_fgedit->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_blkctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
}

RoomViewerFrame::~RoomViewerFrame()
{
	this->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);

	m_roomview->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_entityctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_warpctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_swapctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_hmedit->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_bgedit->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_fgedit->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_blkctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
}

void RoomViewerFrame::SetMode(RoomEdit::Mode mode)
{
	m_mode = mode;
	m_swapctrl->SetMode(m_mode);
	UpdateUI();
	UpdateFrame();
}

void RoomViewerFrame::UpdateFrame()
{
	m_layerctrl->EnableLayers(m_mode == RoomEdit::Mode::NORMAL);
	m_blkctrl->OpenRoom(m_roomnum);
	m_roomview->SetRoomNum(m_roomnum);
	m_hmedit->SetRoomNum(m_roomnum);
	m_bgedit->SetRoomNum(m_roomnum);
	m_fgedit->SetRoomNum(m_roomnum);
	TileSwapRefresh();
	UpdateUI();
	FireEvent(EVT_STATUSBAR_UPDATE);
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void RoomViewerFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_g = gd;
	if (m_roomview)
	{
		m_roomview->SetGameData(gd);
	}
	if (m_hmedit)
	{
		m_hmedit->SetGameData(gd);
	}
	if (m_bgedit)
	{
		m_bgedit->SetGameData(gd);
	}
	if (m_fgedit)
	{
		m_fgedit->SetGameData(gd);
	}
	if (m_blkctrl)
	{
		m_blkctrl->SetGameData(gd);
	}
	if (m_swapctrl)
	{
		m_swapctrl->SetGameData(gd);
	}
	m_mode = RoomEdit::Mode::NORMAL;
	UpdateFrame();
}

void RoomViewerFrame::ClearGameData()
{
	m_g = nullptr;
	if (m_roomview)
	{
		m_roomview->ClearGameData();
	}
	if (m_hmedit)
	{
		m_hmedit->ClearGameData();
	}
	if (m_bgedit)
	{
		m_bgedit->ClearGameData();
	}
	if (m_fgedit)
	{
		m_fgedit->ClearGameData();
	}
	if (m_blkctrl)
	{
		m_blkctrl->ClearGameData();
	}
	if (m_swapctrl)
	{
		m_swapctrl->ClearGameData();
	}
	m_mode = RoomEdit::Mode::NORMAL;
	UpdateFrame();
}

void RoomViewerFrame::SetRoomNum(uint16_t roomnum)
{
	if (m_g != nullptr)
	{
		m_nb->SetPageText(0, m_g->GetRoomData()->GetRoom(roomnum)->name);
		m_nb->SetPageText(1, wxString("Heightmap: ") + m_g->GetRoomData()->GetRoom(roomnum)->map);
		m_nb->SetPageText(2, wxString("Background: ") + m_g->GetRoomData()->GetRoom(roomnum)->map);
		m_nb->SetPageText(3, wxString("Foreground: ") + m_g->GetRoomData()->GetRoom(roomnum)->map);
		m_g->GetRoomData()->CleanupChests(*m_g);
	}
	if (m_roomnum != roomnum)
	{
		m_reset_props = true;
	}
	m_roomnum = roomnum;
	m_blkctrl->SetBlockSelection(0);
	m_bgedit->SetSelectedBlock(0);
	m_fgedit->SetSelectedBlock(0);
	m_bgedit->SetSelectedCell({ -1,-1 });
	m_fgedit->SetSelectedCell({ -1,-1 });
	UpdateUI();
	UpdateFrame();
}

bool RoomViewerFrame::ExportBin(const std::string& path)
{
	auto data = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
	WriteBytes(*data->GetBytes(), path);
	return true;
}

bool RoomViewerFrame::ExportCsv(const std::array<std::string, 3>& paths)
{
	auto data = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
	std::ofstream bg(paths[0], std::ios::out | std::ios::trunc);
	std::ofstream fg(paths[1], std::ios::out | std::ios::trunc);
	std::ofstream hm(paths[2], std::ios::out | std::ios::trunc);


	for (int i = 0; i < data->GetWidth() * data->GetHeight(); ++i)
	{
		fg << StrPrintf("%04X", data->GetBlock(i, Tilemap3D::Layer::FG).value);
		bg << StrPrintf("%04X", data->GetBlock(i, Tilemap3D::Layer::BG).value);
		if ((i + 1) % data->GetWidth() == 0)
		{
			fg << std::endl;
			bg << std::endl;
		}
		else
		{
			fg << ",";
			bg << ",";
		}
	}
	hm << StrPrintf("%02X", data->GetLeft()) << "," << StrPrintf("%02X",data->GetTop()) << std::endl;
	for (int i = 0; i < data->GetHeightmapHeight(); ++i)
		for (int j = 0; j < data->GetHeightmapWidth(); ++j)
		{
			hm << StrPrintf("%X%X%02X", data->GetCellProps({ j, i }), data->GetHeight({ j, i }), data->GetCellType({j, i}));
			if ((j + 1) % data->GetHeightmapWidth() == 0)
			{
				hm << std::endl;
			}
			else
			{
				hm << ",";
			}
		}

	return true;
}

bool RoomViewerFrame::ExportAllCsv(const std::string& dir)
{
	wxSetWorkingDirectory(dir);

	for (std::size_t i = 0; i < m_g->GetRoomData()->GetRoomCount(); ++i)
	{
		m_roomnum = i;
		
		auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);

		const std::string background = rd->map + "_background.csv";
		const std::string heightmap =  rd->map + "_heightmap.csv";
		const std::string foreground = rd->map + "_foreground.csv";
		
		std::array<std::string, 3> paths = {background, foreground, heightmap};
		ExportCsv(paths);
	}

	return true;
}

bool RoomViewerFrame::ExportTmx(const std::string& tmx_path, const std::string& bs_path, uint16_t roomnum)
{
	auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
	auto blocksets = m_g->GetRoomData()->GetCombinedBlocksetForRoom(roomnum);
	auto palette = std::vector<std::shared_ptr<Palette>>{ m_g->GetRoomData()->GetPaletteForRoom(roomnum)->GetData() };
	auto tileset = m_g->GetRoomData()->GetTilesetForRoom(roomnum)->GetData();

	const int width = 16;
	const int height = 64;
	int blockwidth = MapBlock::GetBlockWidth() * tileset->GetTileWidth();
	int blockheight = MapBlock::GetBlockHeight() * tileset->GetTileHeight();
	int pixelwidth = blockwidth * width;
	int pixelheight = blockheight * height;
	ImageBuffer buf(pixelwidth, pixelheight);
	std::size_t i = 0;
	for (int y = 0; y < pixelheight; y += blockheight)
	{
		for (int x = 0; x < pixelwidth; x += blockwidth, ++i)
		{
			if (i < blocksets->size())
			{
				buf.InsertBlock(x, y, 0, blocksets->at(i), *tileset);
			}
			else
			{
				break;
			}
		}
	}
	buf.WritePNG(bs_path, { palette }, true);
	return MapToTmx::ExportToTmx(tmx_path, *m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData(), bs_path);
}

bool RoomViewerFrame::ExportAllTmx(const std::string& dir)
{
	wxBusyInfo wait("Exporting...");
	wxString curdir = wxGetCwd();
	wxSetWorkingDirectory(dir);
	filesystem::path mappath(dir);
	filesystem::path bspath(mappath / "blocksets");
	filesystem::create_directories(bspath);
	for (std::size_t i = 0; i < m_g->GetRoomData()->GetRoomCount(); ++i)
	{
		auto rd = m_g->GetRoomData()->GetRoom(i);
		std::string mapfile = rd->map + ".tmx";
		std::string blkname = StrPrintf("BT%02d_%01d%01d_p%02d.png", rd->tileset + 1, rd->pri_blockset, rd->sec_blockset + 1, rd->room_palette + 1);
		std::string blkpath = "blocksets";
		blkpath += wxFileName::GetPathSeparator() + blkname;
		ExportTmx(mapfile, blkpath, i);
	}
	wxSetWorkingDirectory(curdir);
	return true;
}

bool RoomViewerFrame::ExportRoomTmx(const std::string& tmx_path, const std::string& bs_path, uint16_t roomnum)
{
	auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
	auto blocksets = m_g->GetRoomData()->GetCombinedBlocksetForRoom(roomnum);
	auto palette = std::vector<std::shared_ptr<Palette>>{ m_g->GetRoomData()->GetPaletteForRoom(roomnum)->GetData() };
	auto tileset = m_g->GetRoomData()->GetTilesetForRoom(roomnum)->GetData();

	const int width = 16;
	const int height = 64;
	int blockwidth = MapBlock::GetBlockWidth() * tileset->GetTileWidth();
	int blockheight = MapBlock::GetBlockHeight() * tileset->GetTileHeight();
	int pixelwidth = blockwidth * width;
	int pixelheight = blockheight * height;
	ImageBuffer buf(pixelwidth, pixelheight);
	std::size_t i = 0;
	for (int y = 0; y < pixelheight; y += blockheight)
	{
		for (int x = 0; x < pixelwidth; x += blockwidth, ++i)
		{
			if (i < blocksets->size())
			{
				buf.InsertBlock(x, y, 0, blocksets->at(i), *tileset);
			}
			else
			{
				break;
			}
		}
	}
	buf.WritePNG(bs_path, { palette }, true);

	return RoomToTmx::ExportToTmx(tmx_path, roomnum, m_g, bs_path);
}

bool RoomViewerFrame::ExportAllRoomsTmx(const std::string& dir)
{
	wxBusyInfo wait("Exporting...");
	wxString curdir = wxGetCwd();
	wxSetWorkingDirectory(dir);
	filesystem::path mappath(dir);
	filesystem::path bspath(mappath / "blocksets");
	filesystem::create_directories(bspath);
	for (std::size_t i = 0; i < m_g->GetRoomData()->GetRoomCount(); ++i)
	{
		auto rd = m_g->GetRoomData()->GetRoom(i);
		std::string roomfile = rd->name + ".tmx";
		std::string blkname = StrPrintf("BT%02d_%01d%01d_p%02d.png", rd->tileset + 1, rd->pri_blockset, rd->sec_blockset + 1, rd->room_palette + 1);
		std::string blkpath = "blocksets";
		blkpath += wxFileName::GetPathSeparator() + blkname;
		ExportRoomTmx(roomfile, blkpath, i);
	}
	wxSetWorkingDirectory(curdir);
	return true;
}

bool RoomViewerFrame::ExportPng(const std::string& path)
{
	auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
	auto blocksets = m_g->GetRoomData()->GetBlocksetsForRoom(m_roomnum);
	auto palette = std::vector<std::shared_ptr<Palette>>{ m_g->GetRoomData()->GetPaletteForRoom(m_roomnum)->GetData() };
	auto tileset = m_g->GetRoomData()->GetTilesetForRoom(m_roomnum)->GetData();
	auto blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(m_roomnum);

	int width = map->GetPixelWidth();
	int height = map->GetPixelHeight();
	ImageBuffer buf(width, height);

	buf.Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, map, tileset, blockset);
	buf.Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
	return buf.WritePNG(path, palette, m_roomview->GetAlpha());
}

bool RoomViewerFrame::ImportBin(const std::string& path)
{
	auto bytes = ReadBytes(path);
	auto data = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
	data->GetData()->Decode(bytes.data());
	UpdateFrame();
	return true;
}

bool RoomViewerFrame::ImportTmx(const std::string& paths, uint16_t roomnum)
{
	auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
	auto retval = MapToTmx::ImportFromTmx(paths, *map);
	UpdateFrame();
	return retval;
}

bool RoomViewerFrame::ImportAllTmx(const std::string& dir)
{
	wxBusyInfo wait("Importing...");
	wxDir d;

	wxString curdir = wxGetCwd();
	wxSetWorkingDirectory(dir);
	if (d.Open(dir))
	{
		wxString file;
		bool cont = d.GetFirst(&file, "*.tmx");
		while (cont)
		{
			wxFileName name(file);
			auto map = m_g->GetRoomData()->GetMap(name.GetName().ToStdString());
			if (map)
			{
				MapToTmx::ImportFromTmx(file.ToStdString(), *map->GetData());
			}
			cont = d.GetNext(&file);
		}
	}
	wxSetWorkingDirectory(curdir);
	UpdateFrame();
	return true;
}

bool RoomViewerFrame::ImportCsv(const std::array<std::string, 3>& paths)
{
	auto data = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
	std::ifstream bg(paths[0], std::ios::in);
	std::ifstream fg(paths[1], std::ios::in);
	std::ifstream hm(paths[2], std::ios::in);
	
	std::size_t w, h, t, l, hw, hh;
	std::vector<std::vector<uint16_t>> foreground, background, heightmap;

	auto read_csv = [](auto& iss, auto& data)
	{
		std::string row;
		std::string cell;
		while (std::getline(iss, row))
		{
			data.push_back(std::vector<uint16_t>());
			std::istringstream rss(row);
			while (std::getline(rss, cell, ','))
			{
				data.back().push_back(std::stoi(cell, nullptr, 16));
			}
		}
	};
	
	read_csv(fg, foreground);
	read_csv(bg, background);
	read_csv(hm, heightmap);

	if (heightmap.size() < 2 || heightmap.front().size() != 2)
	{
		return false;
	}
	if (foreground.size() == 0 || foreground.front().size() == 0)
	{
		return false;
	}
	if (background.size() == 0 || background.front().size() == 0)
	{
		return false;
	}
	w = foreground.front().size();
	h = foreground.size();
	hw = heightmap[1].size();
	hh = heightmap.size() - 1;
	l = heightmap[0][0];
	t = heightmap[0][1];

	if (background.size() != h)
	{
		return false;
	}

	for (std::size_t i = 0; i < h; ++i)
	{
		if (background[i].size() != w || foreground[i].size() != w)
		{
			return false;
		}
	}
	for (std::size_t i = 1; i <= hh; ++i)
	{
		if (heightmap[i].size() != hw)
		{
			return false;
		}
	}

	data->Resize(w, h);
	data->ResizeHeightmap(hw, hh);
	data->SetLeft(l);
	data->SetTop(t);
	
	int i = 0;
	for (std::size_t y = 0; y < h; ++y)
	{
		for (std::size_t x = 0; x < w; ++x)
		{
			data->SetBlock(background[y][x], i, Tilemap3D::Layer::BG);
			data->SetBlock(foreground[y][x], i++, Tilemap3D::Layer::FG);
		}
	}
	for (int y = 0; y < static_cast<int>(hh); ++y)
	{
		for (int x = 0; x < static_cast<int>(hw); ++x)
		{
			data->SetCellProps({ x, y }, (heightmap[y + 1][x] >> 12) & 0xF);
			data->SetHeight({ x, y }, (heightmap[y + 1][x] >> 8) & 0xF);
			data->SetCellType({ x, y }, heightmap[y + 1][x] & 0xFF);
		}
	}
	UpdateFrame();
	return true;
}

bool RoomViewerFrame::HandleKeyDown(unsigned int key, unsigned int modifiers)
{
	if (m_mode == RoomEdit::Mode::NORMAL && m_roomview != nullptr)
	{
		return m_roomview->HandleKeyDown(key, modifiers);
	}
	else if (m_mode == RoomEdit::Mode::HEIGHTMAP && m_hmedit != nullptr)
	{
		return m_hmedit->HandleKeyDown(key, modifiers);
	}
	else if (m_mode == RoomEdit::Mode::FOREGROUND && m_fgedit != nullptr)
	{
		return m_fgedit->HandleKeyDown(key, modifiers);
	}
	else if (m_mode == RoomEdit::Mode::BACKGROUND && m_bgedit != nullptr)
	{
		return m_bgedit->HandleKeyDown(key, modifiers);
	}
	return false;
}

bool RoomViewerFrame::HandleKeyUp(unsigned int key, unsigned int modifiers)
{
	if (m_mode == RoomEdit::Mode::NORMAL && m_roomview != nullptr)
	{
		return m_roomview->HandleKeyUp(key, modifiers);
	}
	else if (m_mode == RoomEdit::Mode::HEIGHTMAP && m_hmedit != nullptr)
	{
		return m_hmedit->HandleKeyUp(key, modifiers);
	}
	else if (m_mode == RoomEdit::Mode::FOREGROUND && m_fgedit != nullptr)
	{
		return m_fgedit->HandleKeyUp(key, modifiers);
	}
	else if (m_mode == RoomEdit::Mode::BACKGROUND && m_bgedit != nullptr)
	{
		return m_bgedit->HandleKeyUp(key, modifiers);
	}
	return false;
}

void RoomViewerFrame::ShowFlagDialog()
{
	FlagDialog dlg(this, GetImageList(), m_roomnum, m_g);
	dlg.ShowModal();
	UpdateFrame();
}

void RoomViewerFrame::ShowChestsDialog()
{
	ChestDialog dlg(this, GetImageList(), m_roomnum, m_g);
	dlg.ShowModal();
}

void RoomViewerFrame::ShowCharDialog()
{
	CharacterDialog dlg(this, GetImageList(), m_roomnum, m_g);
	dlg.ShowModal();
}

void RoomViewerFrame::ShowTileswapDialog(bool force, TileSwapDialog::PageType page, int row)
{
	TileSwapDialog dlg(this, GetImageList(), m_roomnum, m_g);
	if (force)
	{
		dlg.SetPage(page);
		dlg.SelectRow(row);
	}

	dlg.ShowModal();
	FireEvent(EVT_TILESWAP_UPDATE);
	if (dlg.GetLastPage() == TileSwapDialog::PageType::SWAPS)
	{
		FireEvent(EVT_TILESWAP_SELECT, dlg.GetLastSelected());
	}
	else if (dlg.GetLastPage() == TileSwapDialog::PageType::DOORS)
	{
		FireEvent(EVT_DOOR_SELECT, dlg.GetLastSelected());
	}
}

void RoomViewerFrame::ShowErrorDialog()
{
	RoomErrorDialog dlg(this, m_roomview->GetErrors());
	dlg.ShowModal();
}

void RoomViewerFrame::InitStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(3);
	status.SetStatusText("", 0);
	status.SetStatusText("", 1);
	status.SetStatusText("", 1);
}

void RoomViewerFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const
{
	if (status.GetFieldsCount() != 3)
	{
		return;
	}
	EditorFrame::UpdateStatusBar(status, evt);
}

void RoomViewerFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_g && ArePropsInitialised() == false)
	{
		RefreshLists();
		props.GetGrid()->Clear();
		const auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
		auto tm = m_g->GetRoomData()->GetMapForRoom(m_roomnum);

		props.Append(new wxPropertyCategory("Main", "Main"));
		props.Append(new wxStringProperty("Name", "Name", rd->name));
		props.Append(new wxIntProperty("Room Number", "RN", rd->index))->Enable(false);
		props.Append(new wxEnumProperty("Tileset", "TS", m_tilesets));
		props.Append(new wxEnumProperty("Room Palette", "RP", m_palettes))->SetChoiceSelection(rd->room_palette);
		props.Append(new wxEnumProperty("Primary Blockset", "PBT", m_pri_blocksets));
		props.Append(new wxEnumProperty("Secondary Blockset", "SBT", m_sec_blocksets));
		props.Append(new wxEnumProperty("BGM", "BGM", m_bgms))->SetChoiceSelection(rd->bgm);
		props.Append(new wxEnumProperty("Map", "M", m_maps));
		props.Append(new wxIntProperty("Unknown Parameter 1", "UP1", rd->unknown_param1));
		props.Append(new wxIntProperty("Unknown Parameter 2", "UP2", rd->unknown_param2));
		props.Append(new wxIntProperty("Z Begin", "ZB", rd->room_z_begin));
		props.Append(new wxIntProperty("Z End", "ZE", rd->room_z_end));
		props.Append(new wxPropertyCategory("Map", "Map"));
		props.Append(new wxIntProperty("Heightmap Left Offset", "TLO", tm->GetData()->GetLeft()));
		props.Append(new wxIntProperty("Heightmap Top Offset", "TTO", tm->GetData()->GetTop()));
		props.Append(new wxIntProperty("Heightmap Width", "HW", tm->GetData()->GetHeightmapWidth()))->Enable(false);
		props.Append(new wxIntProperty("Heightmap Height", "HH", tm->GetData()->GetHeightmapHeight()))->Enable(false);
		props.Append(new wxIntProperty("Tilemap Width", "TW", tm->GetData()->GetWidth()))->Enable(false);
		props.Append(new wxIntProperty("Tilemap Height", "TH", tm->GetData()->GetHeight()))->Enable(false);
		props.Append(new wxPropertyCategory("Warps", "Warps"));
		props.Append(new wxEnumProperty("Fall Destination", "FD", m_rooms));
		props.Append(new wxEnumProperty("Climb Destination", "CD",m_rooms));
		props.Append(new wxPropertyCategory("Flags", "Flags"));
		props.Append(new wxIntProperty("Visit Flag", "VF", m_g->GetStringData()->GetRoomVisitFlag(m_roomnum)));
		int paired_tree = m_g->GetRoomData()->HasTreeWarpFlag(m_roomnum) ? m_g->GetRoomData()->GetTreeWarp(m_roomnum).room2 + 1 : 0;
		props.Append(new wxEnumProperty("Paired Tree", "PairedTree", m_rooms))->SetChoiceSelection(paired_tree);
		props.Append(new wxIntProperty("Tree Active Flag Begin", "TreeFlag", m_g->GetRoomData()->GetTreeWarp(m_roomnum).flag));
		auto lantern_prop = new wxBoolProperty("Lantern Room", "LanternRoom", m_g->GetRoomData()->HasLanternFlag(m_roomnum));
		lantern_prop->SetAttribute("UseCheckbox", 1);
		props.Append(lantern_prop);
		props.Append(new wxIntProperty("Lantern Activate Flag", "LanternFlag", m_g->GetRoomData()->GetLanternFlag(m_roomnum)));
		props.Append(new wxPropertyCategory("Misc", "Misc"));
		auto tree_prop = new wxBoolProperty("Has Tree", "HasTree", m_g->GetRoomData()->IsTree(m_roomnum));
		tree_prop->SetAttribute("UseCheckbox", 1);
		props.Append(tree_prop);
		auto shop_prop = new wxBoolProperty("Is Shop/Church/Inn", "IsShop", m_g->GetRoomData()->IsShop(m_roomnum));
		shop_prop->SetAttribute("UseCheckbox", 1);
		props.Append(shop_prop);
		auto ls_prop = new wxBoolProperty("Lifestock For Sale", "LifestockSale", m_g->GetRoomData()->HasLifestockSaleFlag(m_roomnum));
		ls_prop->SetAttribute("UseCheckbox", 1);
		props.Append(ls_prop);
		props.Append(new wxIntProperty("Lifestock Sale Flag", "LifestockFlag", m_g->GetRoomData()->GetLifestockSaleFlag(m_roomnum)));
		props.Append(new wxEnumProperty("Save Location String", "SLS", m_menustrings));
		props.Append(new wxEnumProperty("Island Location String", "ILS", m_menustrings));
		props.Append(new wxIntProperty("Island Position", "ILP", -1));
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void RoomViewerFrame::RefreshLists() const
{
	const auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);

	m_bgms.Clear();
	for (int i = 0; i < 19; ++i)
	{
		m_bgms.Add(StrWPrintf(L"[%02X] ", i) + Labels::Get(L"bgms", i).value_or(StrWPrintf(L"Track %d", i)));
	}

	m_palettes.Clear();
	for (const auto& p : m_g->GetRoomData()->GetRoomPalettes())
	{
		m_palettes.Add(_(p->GetName()));
	}

	m_tilesets.Clear();
	for (const auto& p : m_g->GetRoomData()->GetTilesets())
	{
		m_tilesets.Add(_(p->GetName()));
	}
	m_pri_blocksets.Clear();
	m_sec_blocksets.Clear();
	for (const auto& p : m_g->GetRoomData()->GetAllBlocksets())
	{
		if (p.second->GetTileset() == rd->tileset)
		{
			if (p.second->GetSecondary() == 0)
			{
				m_pri_blocksets.Add(_(p.first));
			}
			else if (p.second->GetPrimary() == rd->pri_blockset)
			{
				m_sec_blocksets.Add(_(p.first));
			}
		}
	}
	m_maps.Clear();
	for (const auto& map : m_g->GetRoomData()->GetMaps())
	{
		m_maps.Add(map.first);
	}
	m_rooms.Clear();
	m_rooms.Add("<NONE>");
	for (const auto& room : m_g->GetRoomData()->GetRoomlist())
	{
		m_rooms.Add(room->name);
	}
	m_menustrings.Clear();
	m_menustrings.Add("<NONE>");
	for (unsigned int i = 0; i < m_g->GetStringData()->GetItemNameCount(); ++i)
	{
		m_menustrings.Add(m_g->GetStringData()->GetItemName(i));
	}
	for (unsigned int i = 0; i < m_g->GetStringData()->GetMenuStrCount(); ++i)
	{
		m_menustrings.Add(m_g->GetStringData()->GetMenuStr(i));
	}
}

void RoomViewerFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	EditorFrame::UpdateProperties(props);
	if (ArePropsInitialised() == true)
	{
		if (m_reset_props)
		{
			props.GetGrid()->ClearModifiedStatus();
			m_reset_props = false;
		}
		RefreshProperties(props);
	}
}

void RoomViewerFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_g != nullptr)
	{
		RefreshLists();
		props.GetGrid()->Freeze();
		props.GetGrid()->GetProperty("PBT")->SetChoices(m_pri_blocksets);
		props.GetGrid()->GetProperty("SBT")->SetChoices(m_sec_blocksets);

		const auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
		auto tm = m_g->GetRoomData()->GetMapForRoom(m_roomnum);

		props.GetGrid()->SetPropertyValue("Name", _(rd->name));
		props.GetGrid()->SetPropertyValue("RN", rd->index);
		props.GetGrid()->GetProperty("TS")->SetChoiceSelection(rd->tileset);
		props.GetGrid()->GetProperty("RP")->SetChoiceSelection(rd->room_palette);
		props.GetGrid()->GetProperty("BGM")->SetChoiceSelection(rd->bgm);
		props.GetGrid()->GetProperty("PBT")->SetChoiceSelection(rd->pri_blockset);
		props.GetGrid()->GetProperty("SBT")->SetChoiceSelection(rd->sec_blockset);
		props.GetGrid()->GetProperty("M")->SetChoiceSelection(m_maps.Index(rd->map));
		props.GetGrid()->SetPropertyValue("UP1", rd->unknown_param1);
		props.GetGrid()->SetPropertyValue("UP2", rd->unknown_param2);
		props.GetGrid()->SetPropertyValue("ZB", rd->room_z_begin);
		props.GetGrid()->SetPropertyValue("ZE", rd->room_z_end);
		props.GetGrid()->SetPropertyValue("TLO", tm->GetData()->GetLeft());
		props.GetGrid()->SetPropertyValue("TTO", tm->GetData()->GetTop());
		props.GetGrid()->SetPropertyValue("TW", tm->GetData()->GetWidth());
		props.GetGrid()->SetPropertyValue("TH", tm->GetData()->GetHeight());
		props.GetGrid()->SetPropertyValue("HW", tm->GetData()->GetHeightmapWidth());
		props.GetGrid()->SetPropertyValue("HH", tm->GetData()->GetHeightmapHeight());
		int fall = m_g->GetRoomData()->HasFallDestination(m_roomnum) ? (m_g->GetRoomData()->GetFallDestination(m_roomnum) + 1) : 0;
		int climb = m_g->GetRoomData()->HasClimbDestination(m_roomnum) ? (m_g->GetRoomData()->GetClimbDestination(m_roomnum) + 1) : 0;
		props.GetGrid()->GetProperty("FD")->SetChoiceSelection(fall);
		props.GetGrid()->GetProperty("CD")->SetChoiceSelection(climb);
		props.GetGrid()->SetPropertyValue("VF", m_g->GetStringData()->GetRoomVisitFlag(m_roomnum));
		int save_loc = m_g->GetStringData()->GetSaveLocation(m_roomnum);
		save_loc = save_loc == 0xFF ? 0 : save_loc + 1;
		int map_loc = m_g->GetStringData()->GetMapLocation(m_roomnum);
		map_loc = map_loc == 0xFF ? 0 : map_loc + 1;
		props.GetGrid()->GetProperty("SLS")->SetChoiceSelection(save_loc);
		props.GetGrid()->GetProperty("ILS")->SetChoiceSelection(map_loc);
		props.GetGrid()->SetPropertyValue("IsShop", m_g->GetRoomData()->IsShop(m_roomnum));
		props.GetGrid()->SetPropertyValue("HasTree", m_g->GetRoomData()->IsTree(m_roomnum));
		if (map_loc == 0)
		{
			props.GetGrid()->GetProperty("ILP")->Enable(false);
			props.GetGrid()->SetPropertyValue("ILP", 0);
		}
		else
		{
			props.GetGrid()->GetProperty("ILP")->Enable(true);
			props.GetGrid()->SetPropertyValue("ILP", m_g->GetStringData()->GetMapPosition(m_roomnum));
		}
		if (m_g->GetRoomData()->HasTreeWarpFlag(m_roomnum))
		{
			auto tree = m_g->GetRoomData()->GetTreeWarp(m_roomnum);
			props.GetGrid()->GetProperty("TreeFlag")->Enable(true);
			props.GetGrid()->SetPropertyValue("TreeFlag", tree.flag);
			props.GetGrid()->SetPropertyValue("PairedTree", tree.room2 + 1);
		}
		else
		{
			props.GetGrid()->GetProperty("TreeFlag")->Enable(false);
			props.GetGrid()->SetPropertyValue("TreeFlag", 0xFFFF);
			props.GetGrid()->SetPropertyValue("PairedTree", 0);
		}
		if (m_g->GetRoomData()->HasLanternFlag(m_roomnum))
		{
			props.GetGrid()->GetProperty("LanternFlag")->Enable(true);
			props.GetGrid()->SetPropertyValue("LanternFlag", m_g->GetRoomData()->GetLanternFlag(m_roomnum));
			props.GetGrid()->SetPropertyValue("LanternRoom", true);
		}
		else
		{
			props.GetGrid()->GetProperty("LanternFlag")->Enable(false);
			props.GetGrid()->SetPropertyValue("LanternFlag", 0xFFFF);
			props.GetGrid()->SetPropertyValue("LanternRoom", false);
		}
		if (m_g->GetRoomData()->HasLifestockSaleFlag(m_roomnum))
		{
			props.GetGrid()->GetProperty("LifestockFlag")->Enable(true);
			props.GetGrid()->SetPropertyValue("LifestockFlag", m_g->GetRoomData()->GetLifestockSaleFlag(m_roomnum));
			props.GetGrid()->SetPropertyValue("LifestockSale", true);
		}
		else
		{
			props.GetGrid()->GetProperty("LifestockFlag")->Enable(false);
			props.GetGrid()->SetPropertyValue("LifestockFlag", 0xFFFF);
			props.GetGrid()->SetPropertyValue("LifestockSale", false);
		}
		props.GetGrid()->Thaw();
	}
}

void RoomViewerFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
	auto* ctrl = static_cast<wxPropertyGridManager*>(evt.GetEventObject());
	ctrl->GetGrid()->Freeze();
	wxPGProperty* property = evt.GetProperty();
	if (property == nullptr)
	{
		return;
	}
	const auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
	auto tm = m_g->GetRoomData()->GetMapForRoom(m_roomnum);

	const wxString& name = property->GetName();
	if (name == "TS")
	{
		if (property->GetChoiceSelection() != rd->tileset)
		{
			rd->tileset = property->GetChoiceSelection();
			if (m_g->GetRoomData()->GetBlockset(rd->tileset, rd->pri_blockset, 0) == nullptr)
			{
				rd->pri_blockset = 0;
			}
			if (m_g->GetRoomData()->GetBlockset(rd->tileset, rd->pri_blockset, rd->sec_blockset + 1) == nullptr)
			{
				rd->sec_blockset = 0;
			}
			

			UpdateFrame();
		}
	}
	else if (name == "RP")
	{
		if (property->GetChoiceSelection() != rd->room_palette)
		{
			rd->room_palette = property->GetChoiceSelection();
			UpdateFrame();
		}
	}
	else if (name == "PBT")
	{
		if (property->GetChoiceSelection() != rd->pri_blockset)
		{
			rd->pri_blockset = property->GetChoiceSelection();
			if (m_g->GetRoomData()->GetBlockset(rd->tileset, rd->pri_blockset, rd->sec_blockset + 1) == nullptr)
			{
				rd->sec_blockset = 0;
			}
			UpdateFrame();
		}
	}
	else if (name == "SBT")
	{
		if (property->GetChoiceSelection() != rd->sec_blockset)
		{
			rd->sec_blockset = property->GetChoiceSelection();
			UpdateFrame();
		}
	}
	else if (name == "BGM")
	{
		if (property->GetChoiceSelection() != rd->bgm)
		{
			rd->bgm = property->GetChoiceSelection();
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "M")
	{
		const auto& map_name = m_maps.GetLabel(property->GetChoiceSelection());
		if (map_name != rd->map)
		{
			rd->map = map_name;
			UpdateFrame();
		}
	}
	else if (name == "ZB")
	{
		if (property->GetValuePlain().GetLong() != rd->room_z_begin)
		{
			rd->room_z_begin = std::clamp<uint8_t>(property->GetValuePlain().GetLong(), 0, 15);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "ZE")
	{
		if (property->GetValuePlain().GetLong() != rd->room_z_end)
		{
			rd->room_z_end = std::clamp<uint8_t>(property->GetValuePlain().GetLong(), 0, 15);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "UP1")
	{
		if (property->GetValuePlain().GetLong() != rd->unknown_param1)
		{
			rd->unknown_param1 = std::clamp<uint8_t>(property->GetValuePlain().GetLong(), 0, 3);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "UP2")
	{
		if (property->GetValuePlain().GetLong() != rd->unknown_param2)
		{
			rd->unknown_param2 = std::clamp<uint8_t>(property->GetValuePlain().GetLong(), 0, 3);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "TLO")
	{
		if (property->GetValuePlain().GetLong() != tm->GetData()->GetLeft())
		{
			tm->GetData()->SetLeft(static_cast<uint8_t>(property->GetValuePlain().GetLong()));
			UpdateFrame();
		}
	}
	else if (name == "TTO")
	{
		if (property->GetValuePlain().GetLong() != tm->GetData()->GetTop())
		{
			tm->GetData()->SetTop(static_cast<uint8_t>(property->GetValuePlain().GetLong()));
			UpdateFrame();
		}
	}
	else if (name == "VF")
	{
		if (property->GetValuePlain().GetLong() != m_g->GetStringData()->GetRoomVisitFlag(m_roomnum))
		{
			m_g->GetStringData()->SetRoomVisitFlag(m_roomnum, static_cast<uint16_t>(property->GetValuePlain().GetLong()));
			UpdateFrame();
		}
	}
	else if (name == "FD")
	{
		bool enabled = property->GetChoiceSelection() != 0;
		int room = property->GetChoiceSelection() - 1;
		if (m_g->GetRoomData()->HasFallDestination(m_roomnum) != enabled ||
			(enabled && m_g->GetRoomData()->GetFallDestination(m_roomnum) != room))
		{
			m_g->GetRoomData()->SetHasFallDestination(m_roomnum, enabled);
			if (enabled)
			{
				m_g->GetRoomData()->SetFallDestination(m_roomnum, room);
			}
			UpdateFrame();
		}
	}
	else if (name == "CD")
	{
		bool enabled = property->GetChoiceSelection() != 0;
		int room = property->GetChoiceSelection() - 1;
		if (m_g->GetRoomData()->HasClimbDestination(m_roomnum) != enabled ||
			(enabled && m_g->GetRoomData()->GetClimbDestination(m_roomnum) != room))
		{
			m_g->GetRoomData()->SetHasClimbDestination(m_roomnum, enabled);
			if (enabled)
			{
				m_g->GetRoomData()->SetClimbDestination(m_roomnum, room);
			}
			UpdateFrame();
		}
	}
	else if (name == "SLS")
	{
		bool enabled = property->GetChoiceSelection() != 0;
		uint8_t string = enabled ? property->GetChoiceSelection() - 1 : 0xFF;
		if (m_g->GetStringData()->GetSaveLocation(m_roomnum) != string)
		{
			m_g->GetStringData()->SetSaveLocation(m_roomnum, string);
		}
	}
	else if (name == "ILS")
	{
		bool enabled = property->GetChoiceSelection() != 0;
		uint8_t string = enabled ? property->GetChoiceSelection() - 1 : 0xFF;
		auto position = enabled ? m_g->GetStringData()->GetMapPosition(m_roomnum) : 0xFF;
		if (m_g->GetStringData()->GetMapLocation(m_roomnum) != string)
		{
			m_g->GetStringData()->SetMapLocation(m_roomnum, string, position);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "ILP")
	{
		uint8_t position = property->GetValuePlain().GetLong();
		auto string = m_g->GetStringData()->GetMapLocation(m_roomnum);
		if (string != 0xFF && m_g->GetStringData()->GetMapPosition(m_roomnum) != position)
		{
			m_g->GetStringData()->SetMapLocation(m_roomnum, string, position);
		}
	}
	else if (name == "IsShop")
	{
		bool enabled = property->GetValuePlain().GetBool();
		if (enabled != m_g->GetRoomData()->IsShop(m_roomnum))
		{
			m_g->GetRoomData()->SetShop(m_roomnum, enabled);
		}
	}
	else if (name == "HasTree")
	{
		bool enabled = property->GetValuePlain().GetBool();
		if (enabled != m_g->GetRoomData()->IsTree(m_roomnum))
		{
			m_g->GetRoomData()->SetTree(m_roomnum, enabled);
		}
	}
	else if (name == "LifestockSale")
	{
		bool enabled = property->GetValuePlain().GetBool();
		if (enabled != m_g->GetRoomData()->HasLifestockSaleFlag(m_roomnum))
		{
			if (enabled)
			{
				m_g->GetRoomData()->SetLifestockSaleFlag(m_roomnum, 0);
			}
			else
			{
				m_g->GetRoomData()->ClearLifestockSaleFlag(m_roomnum);
			}
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "LifestockFlag")
	{
		uint16_t flag = property->GetValuePlain().GetLong() & 0xFFFF;
		if (flag != 0xFFFF &&
			m_g->GetRoomData()->HasLifestockSaleFlag(m_roomnum) &&
			m_g->GetRoomData()->GetLifestockSaleFlag(m_roomnum) != flag)
		{
			m_g->GetRoomData()->SetLifestockSaleFlag(m_roomnum, flag);
		}
	}
	else if (name == "LanternRoom")
	{
	bool enabled = property->GetValuePlain().GetBool();
	if (enabled != m_g->GetRoomData()->HasLanternFlag(m_roomnum))
	{
		if (enabled)
		{
			m_g->GetRoomData()->SetLanternFlag(m_roomnum, 0);
		}
		else
		{
			m_g->GetRoomData()->ClearLanternFlag(m_roomnum);
		}
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
	}
	else if (name == "LanternFlag")
	{
	uint16_t flag = property->GetValuePlain().GetLong() & 0xFFFF;
	if (flag != 0xFFFF &&
		m_g->GetRoomData()->HasLanternFlag(m_roomnum) &&
		m_g->GetRoomData()->GetLanternFlag(m_roomnum) != flag)
	{
		m_g->GetRoomData()->SetLanternFlag(m_roomnum, flag);
	}
	}
	else if (name == "PairedTree")
	{
		bool updated = false;
		auto dest = property->GetValuePlain().GetLong();
		if (dest > 0 && !m_g->GetRoomData()->HasTreeWarpFlag(m_roomnum))
		{
			m_g->GetRoomData()->SetTreeWarp({ m_roomnum, static_cast<uint16_t>(dest - 1), 0 });
			updated = true;
		}
		else if (dest == 0 && m_g->GetRoomData()->HasTreeWarpFlag(m_roomnum))
		{
			m_g->GetRoomData()->ClearTreeWarp(m_roomnum);
			updated = true;
		}
		else
		{
			auto warp = m_g->GetRoomData()->GetTreeWarp(m_roomnum);
			if (dest != warp.room2)
			{
				m_g->GetRoomData()->SetTreeWarp({ m_roomnum, static_cast<uint16_t>(dest - 1), warp.flag});
				updated = true;
			}
		}
		if (updated)
		{
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "TreeFlag")
	{
		uint16_t flag = property->GetValuePlain().GetLong() & 0x07FE;
		if (m_g->GetRoomData()->HasTreeWarpFlag(m_roomnum) &&
			m_g->GetRoomData()->GetTreeWarp(m_roomnum).flag != flag)
		{
			m_g->GetRoomData()->SetTreeWarp({ m_roomnum, m_g->GetRoomData()->GetTreeWarp(m_roomnum).room2, flag });
		}
	}
	ctrl->GetGrid()->Thaw();
}

void RoomViewerFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_BIN, "Export Map as Binary...");
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_CSV, "Export Map as CSV Set...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_ALL_CSV, "Export All Maps as CSV Set...");
	AddMenuItem(fileMenu, 3, ID_FILE_EXPORT_TMX, "Export Map as Tiled TMX...");
	AddMenuItem(fileMenu, 4, ID_FILE_EXPORT_ALL_TMX, "Export All Maps as Tiled TMX...");
	AddMenuItem(fileMenu, 5, ID_FILE_EXPORT_PNG, "Export Map as PNG...");
	AddMenuItem(fileMenu, 7, ID_FILE_EXPORT_ROOM_TMX, "Export Room as Tiled TMX...");
	AddMenuItem(fileMenu, 8, ID_FILE_EXPORT_ALL_ROOMS_TMX, "Export All Rooms as Tiled TMX...");
	AddMenuItem(fileMenu, 9, ID_FILE_SEP1, "", wxITEM_SEPARATOR);
	AddMenuItem(fileMenu, 10, ID_FILE_IMPORT_BIN, "Import Map from Binary...");
	AddMenuItem(fileMenu, 11, ID_FILE_IMPORT_CSV, "Import Map from CSV...");
	AddMenuItem(fileMenu, 12, ID_FILE_IMPORT_ALL_TMX, "Import All Maps from Tiled TMX...");

	auto& editMenu = AddMenu(menu, 1, ID_EDIT, "Edit");
	AddMenuItem(editMenu, 0, ID_EDIT_ENTITY_PROPERTIES, "Selection Properties...");
	AddMenuItem(editMenu, 1, ID_EDIT_FLAGS, "Flags...");
	AddMenuItem(editMenu, 2, ID_EDIT_CHESTS, "Chests...");
	AddMenuItem(editMenu, 3, ID_EDIT_DIALOGUE, "Dialogue...");
	AddMenuItem(editMenu, 4, ID_EDIT_TILESWAPS, "Tile Swaps...");

	auto& viewMenu = AddMenu(menu, 2, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_ALPHA, "Toggle Alpha", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_ENTITIES, "Show Entities", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_ENTITY_HITBOX, "Show Entity Hitboxes", wxITEM_CHECK);
	AddMenuItem(viewMenu, 3, ID_VIEW_WARPS, "Show Warps", wxITEM_CHECK);
	AddMenuItem(viewMenu, 4, ID_VIEW_SWAPS, "Show Tile Swaps / Doors", wxITEM_CHECK);
	AddMenuItem(viewMenu, 5, ID_VIEW_SEP1, "", wxITEM_SEPARATOR);
	AddMenuItem(viewMenu, 6, ID_VIEW_ERRORS, "Errors...");

	auto& toolsMenu = AddMenu(menu, 3, ID_TOOLS, "Tools");
	AddMenuItem(toolsMenu, 0, ID_TOOLS_LAYERS, "Layers", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 1, ID_TOOLS_ENTITIES, "Entity List", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 2, ID_TOOLS_WARPS, "Warp List", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 3, ID_TOOLS_SWAPS, "Tile Swap / Doors List", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 4, ID_TOOLS_BLOCKS, "Block Selector", wxITEM_CHECK);

	wxAuiToolBar* main_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	main_tb->SetToolBitmapSize(wxSize(16, 16));
	main_tb->AddTool(TOOL_TOGGLE_ALPHA, "Show Transparency", ilist.GetImage("alpha"), "Show Transparency", wxITEM_CHECK);
	main_tb->AddTool(TOOL_TOGGLE_ENTITIES, "Entities Visible", ilist.GetImage("entity"), "Entities Visible", wxITEM_CHECK);
	main_tb->AddTool(TOOL_TOGGLE_ENTITY_HITBOX, "Entity Hitboxes Visible", ilist.GetImage("ehitbox"), "Entity Hitboxes Visible", wxITEM_CHECK);
	main_tb->AddTool(TOOL_TOGGLE_WARPS, "Warps Visible", ilist.GetImage("warp"), "Warps Visible", wxITEM_CHECK);
	main_tb->AddTool(TOOL_TOGGLE_SWAPS, "Tile Swaps / Doors Visible", ilist.GetImage("map_edit_tileswaps"), "Tile Swaps / Doors Visible", wxITEM_CHECK);
	main_tb->AddSeparator();
	main_tb->AddTool(TOOL_SHOW_FLAGS, "Flags", ilist.GetImage("flags"), "Flag Editor");
	main_tb->AddTool(TOOL_SHOW_CHESTS, "Chests", ilist.GetImage("chest16"), "Chest Editor");
	main_tb->AddTool(TOOL_SHOW_DIALOGUE, "Dialogue", ilist.GetImage("dialogue"), "Dialogue Editor");
	main_tb->AddTool(TOOL_SHOW_TILESWAPS, "Tile Swaps", ilist.GetImage("swap"), "Tile Swap Editor");
	main_tb->AddTool(TOOL_SHOW_SELECTION_PROPERTIES, "Selection Properties", ilist.GetImage("properties"), "Selection Properties");
	main_tb->AddSeparator();
	main_tb->AddTool(TOOL_SHOW_LAYERS_PANE, "Layers Pane", ilist.GetImage("layers"), "Layers Pane", wxITEM_CHECK);
	main_tb->AddTool(TOOL_SHOW_ENTITIES_PANE, "Entities Pane", ilist.GetImage("epanel"), "Entities Pane", wxITEM_CHECK);
	main_tb->AddTool(TOOL_SHOW_WARPS_PANE, "Warps Pane", ilist.GetImage("wpanel"), "Warps Pane", wxITEM_CHECK);
	main_tb->AddTool(TOOL_SHOW_TILESWAP_PANE, "Tile Swap / Doors Pane", ilist.GetImage("spanel"), "Tile Swap / Doors Pane", wxITEM_CHECK);
	main_tb->AddTool(TOOL_SHOW_BLOCKS_PANE, "Blocks Pane", ilist.GetImage("big_tiles"), "Blocks Pane", wxITEM_CHECK);
	main_tb->AddSeparator();
	main_tb->AddTool(TOOL_SHOW_ERRORS, "Show Errors", ilist.GetImage("warning"), "Show Errors");
	AddToolbar(m_mgr, *main_tb, "Main", "Main Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1));

	wxArrayString celltypes;
	celltypes.Add("[00] Normal");
	celltypes.Add("[01] Door Nudger NE");
	celltypes.Add("[02] Door Nudger SE");
	celltypes.Add("[03] Door Nudger SW");
	celltypes.Add("[04] Door Nudger NW");
	celltypes.Add("[05] Stairs Warp");
	celltypes.Add("[06] Door Warp");
	celltypes.Add("[07] Pit");
	celltypes.Add("[08] Warp Pad");
	celltypes.Add("[09] ???");
	celltypes.Add("[0A] ???");
	celltypes.Add("[0B] Ladder NW");
	celltypes.Add("[0C] Ladder NE");
	celltypes.Add("[0D] ???");
	celltypes.Add("[0E] Counter");
	celltypes.Add("[0F] Elevator");
	celltypes.Add("[10] Spikes");
	celltypes.Add("[11] NW Sign, Dialogue 0");
	celltypes.Add("[12] NW Sign, Dialogue 1");
	celltypes.Add("[13] NW Sign, Dialogue 2");
	celltypes.Add("[14] NW Sign, Dialogue 3");
	celltypes.Add("[15] NE Sign, Dialogue 0");
	celltypes.Add("[16] NE Sign, Dialogue 1");
	celltypes.Add("[17] NE Sign, Dialogue 2");
	celltypes.Add("[18] NE Sign, Dialogue 3");
	celltypes.Add("[19] Swamp");
	celltypes.Add("[1A] Locked Door");
	celltypes.Add("[1B] ???");
	celltypes.Add("[1C] ???");
	celltypes.Add("[1D] ???");
	celltypes.Add("[1E] NW Sign, Dialogue 4");
	celltypes.Add("[1F] NW Sign, Dialogue 5");
	celltypes.Add("[20] NW Sign, Dialogue 6");
	celltypes.Add("[21] NW Sign, Dialogue 7");
	celltypes.Add("[22] NE Sign, Dialogue 4");
	celltypes.Add("[23] NE Sign, Dialogue 5");
	celltypes.Add("[24] NE Sign, Dialogue 6");
	celltypes.Add("[25] NE Sign, Dialogue 7");
	celltypes.Add("[26] SE Locked Door");
	celltypes.Add("[27] SW Locked Door");
	celltypes.Add("[28] Nole Staircase Transition");
	celltypes.Add("[29] Lava");
	celltypes.Add("[2A] NE Ice");
	celltypes.Add("[2B] SE Ice");
	celltypes.Add("[2C] SW Ice");
	celltypes.Add("[2D] NW Ice");
	celltypes.Add("[2E] Health Restore");
	celltypes.Add("[2F] ???");
	celltypes.Add("???");
	wxAuiToolBar* hm_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	auto hmcell = new wxChoice(hm_tb, HM_TYPE_DROPDOWN, wxDefaultPosition, wxDefaultSize, celltypes);
	hmcell->SetSelection(0);
	auto hmzoom = new wxSlider(hm_tb, HM_ZOOM, 2, 1, 5);
	hm_tb->SetToolBitmapSize(wxSize(16, 16));
	hm_tb->AddTool(HM_INSERT_ROW_BEFORE, "Insert Row Before", ilist.GetImage("hm_insert_se"), "Insert Row Before", wxITEM_NORMAL);
	hm_tb->AddTool(HM_INSERT_ROW_AFTER, "Insert Row After", ilist.GetImage("hm_insert_nw"), "Insert Row After", wxITEM_NORMAL);
	hm_tb->AddTool(HM_DELETE_ROW, "Delete Row", ilist.GetImage("hm_delete_nesw"), "Delete Row", wxITEM_NORMAL);
	hm_tb->AddTool(HM_INSERT_COLUMN_BEFORE, "Insert Column Before", ilist.GetImage("hm_insert_sw"), "Insert Column Before", wxITEM_NORMAL);
	hm_tb->AddTool(HM_INSERT_COLUMN_AFTER, "Insert Column After", ilist.GetImage("hm_insert_ne"), "Insert Column After", wxITEM_NORMAL);
	hm_tb->AddTool(HM_DELETE_COLUMN, "Delete Column", ilist.GetImage("hm_delete_nwse"), "Delete Column", wxITEM_NORMAL);
	hm_tb->AddSeparator();
	hm_tb->AddControl(hmcell, "Cell Type");
	hm_tb->AddTool(HM_TOGGLE_PLAYER, "Toggle Player Passable", ilist.GetImage("hm_player_walkable"), "Toggle Player Passable", wxITEM_CHECK);
	hm_tb->AddTool(HM_TOGGLE_NPC, "Toggle NPC Passable", ilist.GetImage("hm_npc_walkable"), "Toggle NPC Passable", wxITEM_CHECK);
	hm_tb->AddTool(HM_TOGGLE_RAFT, "Toggle Raft Track", ilist.GetImage("hm_raft_track"), "Toggle Raft Track", wxITEM_CHECK);
	hm_tb->AddTool(HM_INCREASE_HEIGHT, "Increase Selected Cell Height", ilist.GetImage("hm_cell_up"), "Increase Selected Cell Height");
	hm_tb->AddTool(HM_DECREASE_HEIGHT, "Decrease Selected Cell Height", ilist.GetImage("hm_cell_down"), "Decrease Selected Cell Height");
	hm_tb->AddSeparator();
	hm_tb->AddTool(HM_NUDGE_HM_NE, "Nudge Heightmap North East", ilist.GetImage("hm_nudge_ne"), "Nudge Heightmap North East");
	hm_tb->AddTool(HM_NUDGE_HM_NW, "Nudge Heightmap North West", ilist.GetImage("hm_nudge_nw"), "Nudge Heightmap North West");
	hm_tb->AddTool(HM_NUDGE_HM_SE, "Nudge Heightmap South East", ilist.GetImage("hm_nudge_se"), "Nudge Heightmap South East");
	hm_tb->AddTool(HM_NUDGE_HM_SW, "Nudge Heightmap South West", ilist.GetImage("hm_nudge_sw"), "Nudge Heightmap South West");
	hm_tb->AddSeparator();
	hm_tb->AddControl(hmzoom, "Zoom");
	AddToolbar(m_mgr, *hm_tb, "Heightmap", "Heightmap Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(2));

	wxAuiToolBar* tm_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	tm_tb->AddTool(TM_CLEAR, "Clear Whole Tilemap", ilist.GetImage("delete"), "Clear Whole Tilemap", wxITEM_NORMAL);
	tm_tb->AddTool(TM_INSERT_ROW_BEFORE, "Insert Row Before", ilist.GetImage("map_insert_se"), "Insert Row Before", wxITEM_NORMAL);
	tm_tb->AddTool(TM_INSERT_ROW_AFTER, "Insert Row After", ilist.GetImage("map_insert_nw"), "Insert Row After", wxITEM_NORMAL);
	tm_tb->AddTool(TM_DELETE_ROW, "Delete Row", ilist.GetImage("map_delete_nesw"), "Delete Row", wxITEM_NORMAL);
	tm_tb->AddTool(TM_INSERT_COLUMN_BEFORE, "Insert Column Before", ilist.GetImage("map_insert_sw"), "Insert Column Before", wxITEM_NORMAL);
	tm_tb->AddTool(TM_INSERT_COLUMN_AFTER, "Insert Column After", ilist.GetImage("map_insert_ne"), "Insert Column After", wxITEM_NORMAL);
	tm_tb->AddTool(TM_DELETE_COLUMN, "Delete Column", ilist.GetImage("map_delete_nwse"), "Delete Column", wxITEM_NORMAL);
	AddToolbar(m_mgr, *tm_tb, "Tilemap", "Tilemap Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(3));

	UpdateUI();

	m_mgr.Update();
}

void RoomViewerFrame::OnMenuClick(wxMenuEvent& evt)
{
	const auto id = evt.GetId();
	if ((id >= 20000) && (id < 31000))
	{
		switch (id)
		{
		case ID_FILE_EXPORT_BIN:
			OnExportBin();
			break;
		case ID_FILE_EXPORT_CSV:
			OnExportCsv();
			break;
		case ID_FILE_EXPORT_ALL_CSV:
			OnExportAllCsv();
			break;
		case ID_FILE_EXPORT_TMX:
			OnExportTmx();
			break;
		case ID_FILE_EXPORT_ALL_TMX:
			OnExportAllTmx();
			break;
		case ID_FILE_EXPORT_PNG:
			OnExportPng();
			break;
		case ID_FILE_EXPORT_ROOM_TMX:
			OnExportRoomTmx();
			break;
		case ID_FILE_EXPORT_ALL_ROOMS_TMX:
			OnExportAllRoomsTmx();
			break;
		case ID_FILE_IMPORT_BIN:
			OnImportBin();
			break;
		case ID_FILE_IMPORT_CSV:
			OnImportCsv();
			break;
		case ID_FILE_IMPORT_TMX:
			OnImportTmx();
			break;
		case ID_FILE_IMPORT_ALL_TMX:
			OnImportAllTmx();
			break;
		case ID_VIEW_ALPHA:
		case TOOL_TOGGLE_ALPHA:
			m_roomview->SetAlpha(!m_roomview->GetAlpha());
			break;
		case ID_VIEW_ENTITIES:
		case TOOL_TOGGLE_ENTITIES:
			m_roomview->SetEntitiesVisible(!m_roomview->GetEntitiesVisible());
			break;
		case ID_VIEW_ENTITY_HITBOX:
		case TOOL_TOGGLE_ENTITY_HITBOX:
			m_roomview->SetEntitiesHitboxVisible(!m_roomview->GetEntitiesHitboxVisible());
			break;
		case ID_VIEW_WARPS:
		case TOOL_TOGGLE_WARPS:
			m_roomview->SetWarpsVisible(!m_roomview->GetWarpsVisible());
			break;
		case ID_VIEW_SWAPS:
		case TOOL_TOGGLE_SWAPS:
			m_roomview->SetTileSwapsVisible(!m_roomview->GetTileSwapsVisible());
			break;
		case ID_TOOLS_LAYERS:
		case TOOL_SHOW_LAYERS_PANE:
			SetPaneVisibility(m_layerctrl, !IsPaneVisible(m_layerctrl));
			break;
		case ID_TOOLS_ENTITIES:
		case TOOL_SHOW_ENTITIES_PANE:
			SetPaneVisibility(m_entityctrl, !IsPaneVisible(m_entityctrl));
			break;
		case ID_TOOLS_WARPS:
		case TOOL_SHOW_WARPS_PANE:
			SetPaneVisibility(m_warpctrl, !IsPaneVisible(m_warpctrl));
			break;
		case ID_TOOLS_SWAPS:
		case TOOL_SHOW_TILESWAP_PANE:
			SetPaneVisibility(m_swapctrl, !IsPaneVisible(m_swapctrl));
			break;
		case ID_TOOLS_BLOCKS:
		case TOOL_SHOW_BLOCKS_PANE:
			SetPaneVisibility(m_blkctrl, !IsPaneVisible(m_blkctrl));
			break;
		case ID_EDIT_ENTITY_PROPERTIES:
		case TOOL_SHOW_SELECTION_PROPERTIES:
			if (m_roomview->IsEntitySelected())
			{
				m_roomview->UpdateEntityProperties(m_roomview->GetSelectedEntityIndex());
			}
			else if (m_roomview->IsWarpSelected())
			{
				m_roomview->UpdateWarpProperties(m_roomview->GetSelectedWarpIndex());
			}
			else if (m_roomview->IsTileSwapSelected())
			{
				ShowTileswapDialog(true, TileSwapDialog::PageType::SWAPS, m_roomview->GetSelectedTileSwapIndex());
			}
			else if (m_roomview->IsDoorSelected())
			{
				ShowTileswapDialog(true, TileSwapDialog::PageType::DOORS, m_roomview->GetSelectedDoorIndex());
			}
			break;
		case ID_EDIT_FLAGS:
		case TOOL_SHOW_FLAGS:
			ShowFlagDialog();
			break;
		case ID_EDIT_CHESTS:
		case TOOL_SHOW_CHESTS:
			ShowChestsDialog();
			break;
		case ID_EDIT_DIALOGUE:
		case TOOL_SHOW_DIALOGUE:
			ShowCharDialog();
			break;
		case ID_EDIT_TILESWAPS:
		case TOOL_SHOW_TILESWAPS:
			ShowTileswapDialog();
			break;
		case ID_VIEW_ERRORS:
		case TOOL_SHOW_ERRORS:
			ShowErrorDialog();
			break;
		case HM_INSERT_ROW_BEFORE:
			m_hmedit->InsertRowBelow();
			break;
		case HM_INSERT_ROW_AFTER:
			m_hmedit->InsertRowAbove();
			break;
		case HM_DELETE_ROW:
			m_hmedit->DeleteRow();
			break;
		case HM_INSERT_COLUMN_BEFORE:
			m_hmedit->InsertColumnLeft();
			break;
		case HM_INSERT_COLUMN_AFTER:
			m_hmedit->InsertColumnRight();
			break;
		case HM_DELETE_COLUMN:
			m_hmedit->DeleteColumn();
			break;
		case HM_TOGGLE_PLAYER:
			m_hmedit->ToggleSelectedPlayerPassable();
			break;
		case HM_TOGGLE_NPC:
			m_hmedit->ToggleSelectedNPCPassable();
			break;
		case HM_TOGGLE_RAFT:
			m_hmedit->ToggleSelectedRaftTrack();
			break;
		case HM_INCREASE_HEIGHT:
			m_hmedit->IncreaseHeight();
			break;
		case HM_DECREASE_HEIGHT:
			m_hmedit->DecreaseSelectedHeight();
			break;
		case HM_NUDGE_HM_NE:
			m_hmedit->NudgeHeightmapDown();
			break;
		case HM_NUDGE_HM_NW:
			m_hmedit->NudgeHeightmapRight();
			break;
		case HM_NUDGE_HM_SE:
			m_hmedit->NudgeHeightmapLeft();
			break;
		case HM_NUDGE_HM_SW:
			m_hmedit->NudgeHeightmapUp();
			break;
		case TM_CLEAR:
			OnTmClear();
			break;
		case TM_DELETE_COLUMN:
			OnTmDeleteColumn();
			break;
		case TM_DELETE_ROW:
			OnTmDeleteRow();
			break;
		case TM_INSERT_COLUMN_BEFORE:
			OnTmInsertColumnBefore();
			break;
		case TM_INSERT_COLUMN_AFTER:
			OnTmInsertColumnAfter();
			break;
		case TM_INSERT_ROW_BEFORE:
			OnTmInsertRowBefore();
			break;
		case TM_INSERT_ROW_AFTER:
			OnTmInsertRowAfter();
			break;
		case HM_TYPE_DROPDOWN:
		case HM_ZOOM:
			break;
		default:
			wxMessageBox(wxString::Format("Unrecognised Event %d", evt.GetId()));
		}
		UpdateUI();
	}
}

void RoomViewerFrame::OnTmClear()
{
	auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
	map->GetData()->ClearTilemap();
	UpdateFrame();
}

void RoomViewerFrame::OnTmDeleteRow()
{
	if (m_bgedit->IsCellSelected())
	{
		auto sel = m_bgedit->GetSelectedCell();
		auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
		map->GetData()->DeleteTilemapRow(sel.first);
		UpdateFrame();
	}
}

void RoomViewerFrame::OnTmDeleteColumn()
{
	if (m_bgedit->IsCellSelected())
	{
		auto sel = m_bgedit->GetSelectedCell();
		auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
		map->GetData()->DeleteTilemapColumn(sel.second);
		UpdateFrame();
	}
}

void RoomViewerFrame::OnTmInsertRowBefore()
{
	if (m_bgedit->IsCellSelected())
	{
		auto sel = m_bgedit->GetSelectedCell();
		auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
		map->GetData()->InsertTilemapRow(sel.first);
		++sel.first;
		m_bgedit->SetSelectedCell(sel);
		m_fgedit->SetSelectedCell(sel);
		UpdateFrame();
	}
}

void RoomViewerFrame::OnTmInsertRowAfter()
{
	if (m_bgedit->IsCellSelected())
	{
		auto sel = m_bgedit->GetSelectedCell();
		auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
		map->GetData()->InsertTilemapRow(sel.first + 1);
		UpdateFrame();
	}
}

void RoomViewerFrame::OnTmInsertColumnBefore()
{
	if (m_bgedit->IsCellSelected())
	{
		auto sel = m_bgedit->GetSelectedCell();
		auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
		map->GetData()->InsertTilemapColumn(sel.second);
		++sel.second;
		m_bgedit->SetSelectedCell(sel);
		m_fgedit->SetSelectedCell(sel);
		UpdateFrame();
	}
}

void RoomViewerFrame::OnTmInsertColumnAfter()
{
	if (m_bgedit->IsCellSelected())
	{
		auto sel = m_bgedit->GetSelectedCell();
		auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
		map->GetData()->InsertTilemapColumn(sel.second + 1);
		UpdateFrame();
	}
}

void RoomViewerFrame::OnExportBin()
{
	auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
	const wxString default_file = rd->map + ".cmp";
	wxFileDialog fd(this, _("Export Map As Binary"), "", default_file, "Room Map (*.cmp)|*.cmp|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportBin(fd.GetPath().ToStdString());
	}
}

void RoomViewerFrame::OnExportCsv()
{
	auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
	std::array<std::string, 3> fnames;
	wxString default_file = rd->map + "_background.csv";
	wxFileDialog fd(this, _("Export Background Layer as CSV"), "", default_file, "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		fnames[0] = fd.GetPath().ToStdString();
	}
	default_file = rd->map + "_foreground.csv";
	fd.Create(this, _("Export Foreground Layer as CSV"), "", default_file, "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		fnames[1] = fd.GetPath().ToStdString();
	}
	default_file = rd->map + "_heightmap.csv";
	fd.Create(this, _("Export Heightmap Layer as CSV"), "", default_file, "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		fnames[2] = fd.GetPath().ToStdString();
	}
	ExportCsv(fnames);
}

void RoomViewerFrame::OnExportAllCsv()
{
	wxDirDialog dd(this, "Select CSV Output Directory");
	if (dd.ShowModal() != wxID_CANCEL)
	{
		ExportAllCsv(dd.GetPath().ToStdString());
	}
}

void RoomViewerFrame::OnExportTmx()
{
	auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
	wxString default_file = rd->map + ".tmx";
	wxFileDialog fd(this, _("Export Map As TMX"), "", default_file, "Tiled TMX Tilemap (*.tmx)|*.tmx|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		wxString tmx_file = fd.GetPath();
		default_file = StrPrintf("BT%02d_%01d%01d_p%02d.png", rd->tileset + 1, rd->pri_blockset, rd->sec_blockset + 1, rd->room_palette + 1);
		wxFileDialog bfd(this, _("Export Blockset As PNG"), "", default_file, "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (bfd.ShowModal() != wxID_CANCEL)
		{
			ExportTmx(tmx_file.ToStdString(), bfd.GetPath().ToStdString(), m_roomnum);
		}
	}
}

void RoomViewerFrame::OnExportAllTmx()
{
	wxDirDialog dd(this, "Select TMX Output Directory");
	if (dd.ShowModal() != wxID_CANCEL)
	{
		ExportAllTmx(dd.GetPath().ToStdString());
	}
}

void RoomViewerFrame::OnExportRoomTmx()
{
	auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
	wxString default_file = wxString::Format("Room%03d.tmx", m_roomnum);
	wxFileDialog fd(this, _("Export Room As TMX"), "", default_file, "Tiled TMX Tilemap (*.tmx)|*.tmx|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		wxString tmx_file = fd.GetPath();
		default_file = StrPrintf("BT%02d_%01d%01d_p%02d.png", rd->tileset + 1, rd->pri_blockset, rd->sec_blockset + 1, rd->room_palette + 1);
		wxFileDialog bfd(this, _("Export Blockset As PNG"), "", default_file, "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (bfd.ShowModal() != wxID_CANCEL)
		{
			ExportRoomTmx(tmx_file.ToStdString(), bfd.GetPath().ToStdString(), m_roomnum);
		}
	}
}

void RoomViewerFrame::OnExportAllRoomsTmx()
{
	wxDirDialog dd(this, "Select TMX Output Directory");
	if (dd.ShowModal() != wxID_CANCEL)
	{
		ExportAllRoomsTmx(dd.GetPath().ToStdString());
	}
}

void RoomViewerFrame::OnExportPng()
{
	auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
	const wxString default_file = rd->name + ".png";
	wxFileDialog fd(this, _("Export Map As PNG"), "", default_file, "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportPng(fd.GetPath().ToStdString());
	}
}

void RoomViewerFrame::OnImportBin()
{
	wxFileDialog fd(this, _("Import Map From Binary"), "", "", "Room Map (*.cmp)|*.cmp|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportBin(path);
	}
	UpdateFrame();
}

void RoomViewerFrame::OnImportCsv()
{
	std::array<std::string, 3> filenames;
	wxFileDialog fd(this, _("Import Background Layer"), "", "", "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		filenames[0] = fd.GetPath().ToStdString();
	}
	fd.Create(this, _("Import Foreground Layer"), "", "", "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		filenames[1] = fd.GetPath().ToStdString();
	}
	fd.Create(this, _("Import Heightmap Data"), "", "", "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		filenames[2] = fd.GetPath().ToStdString();
	}
	ImportCsv(filenames);
	UpdateFrame();
}

void RoomViewerFrame::OnImportTmx()
{
	wxFileDialog fd(this, _("Import Map From Tiled TMX"), "", "", "Tiled TMX Tilemap (*.tmx)|*.tmx|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportTmx(path, m_roomnum);
	}
	UpdateFrame();
}

void RoomViewerFrame::OnImportAllTmx()
{
	wxDirDialog dd(this, "Select TMX Input Directory");
	if (dd.ShowModal() != wxID_CANCEL)
	{
		ImportAllTmx(dd.GetPath().ToStdString());
	}
}


void RoomViewerFrame::UpdateUI() const
{
	wxChoice* hmcell = nullptr;
	wxSlider * hmzoom = nullptr;
	auto tb = GetToolbar("Heightmap");
	if (tb != nullptr)
	{
		hmcell = static_cast<wxChoice*>(tb->FindControl(HM_TYPE_DROPDOWN));
		hmzoom = static_cast<wxSlider*>(tb->FindControl(HM_ZOOM));
	}

	EnableMenuItem(ID_TOOLS_LAYERS, true);
	EnableToolbarItem("Main", TOOL_SHOW_LAYERS_PANE, true);
	CheckMenuItem(ID_TOOLS_LAYERS, IsPaneVisible(m_layerctrl));
	CheckToolbarItem("Main", TOOL_SHOW_LAYERS_PANE, IsPaneVisible(m_layerctrl));

	EnableMenuItem(ID_TOOLS_ENTITIES, true);
	EnableToolbarItem("Main", TOOL_SHOW_ENTITIES_PANE, true);
	CheckMenuItem(ID_TOOLS_ENTITIES, IsPaneVisible(m_entityctrl));
	CheckToolbarItem("Main", TOOL_SHOW_ENTITIES_PANE, IsPaneVisible(m_entityctrl));

	EnableMenuItem(ID_TOOLS_WARPS, true);
	EnableToolbarItem("Main", TOOL_SHOW_WARPS_PANE, true);
	CheckMenuItem(ID_TOOLS_WARPS, IsPaneVisible(m_warpctrl));
	CheckToolbarItem("Main", TOOL_SHOW_WARPS_PANE, IsPaneVisible(m_warpctrl));

	EnableMenuItem(ID_TOOLS_SWAPS, true);
	EnableToolbarItem("Main", TOOL_SHOW_TILESWAP_PANE, true);
	CheckMenuItem(ID_TOOLS_SWAPS, IsPaneVisible(m_swapctrl));
	CheckToolbarItem("Main", TOOL_SHOW_TILESWAP_PANE, IsPaneVisible(m_swapctrl));

	EnableMenuItem(ID_TOOLS_BLOCKS, true);
	EnableToolbarItem("Main", TOOL_SHOW_BLOCKS_PANE, true);
	CheckMenuItem(ID_TOOLS_BLOCKS, IsPaneVisible(m_blkctrl));
	CheckToolbarItem("Main", TOOL_SHOW_BLOCKS_PANE, IsPaneVisible(m_blkctrl));

	if (m_mode == RoomEdit::Mode::NORMAL)
	{
		CheckMenuItem(ID_VIEW_ALPHA, m_roomview->GetAlpha());
		CheckToolbarItem("Main", TOOL_TOGGLE_ALPHA, m_roomview->GetAlpha());
		EnableMenuItem(ID_VIEW_ALPHA, true);
		EnableToolbarItem("Main", TOOL_TOGGLE_ALPHA, true);

		EnableMenuItem(ID_VIEW_ENTITIES, true);
		CheckMenuItem(ID_VIEW_ENTITIES, m_roomview->GetEntitiesVisible());
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITIES, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITIES, m_roomview->GetEntitiesVisible());

		EnableMenuItem(ID_VIEW_ENTITY_HITBOX, true);
		CheckMenuItem(ID_VIEW_ENTITY_HITBOX, m_roomview->GetEntitiesHitboxVisible());
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, m_roomview->GetEntitiesHitboxVisible());

		EnableMenuItem(ID_VIEW_WARPS, true);
		CheckMenuItem(ID_VIEW_WARPS, m_roomview->GetWarpsVisible());
		EnableToolbarItem("Main", TOOL_TOGGLE_WARPS, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_WARPS, m_roomview->GetWarpsVisible());

		EnableMenuItem(ID_VIEW_SWAPS, true);
		CheckMenuItem(ID_VIEW_SWAPS, m_roomview->GetTileSwapsVisible());
		EnableToolbarItem("Main", TOOL_TOGGLE_SWAPS, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_SWAPS, m_roomview->GetTileSwapsVisible());
		
		bool object_selected = m_roomview->IsEntitySelected() || m_roomview->IsWarpSelected() || m_roomview->IsDoorSelected() || m_roomview->IsTileSwapSelected();
		EnableMenuItem(ID_EDIT_ENTITY_PROPERTIES, object_selected);
		EnableToolbarItem("Main", TOOL_SHOW_SELECTION_PROPERTIES, object_selected);

		CheckToolbarItem("Heightmap", HM_TOGGLE_PLAYER, false);
		CheckToolbarItem("Heightmap", HM_TOGGLE_NPC, false);
		CheckToolbarItem("Heightmap", HM_TOGGLE_RAFT, false);

		EnableToolbarItem("Heightmap", HM_INSERT_ROW_BEFORE, false);
		EnableToolbarItem("Heightmap", HM_INSERT_ROW_AFTER, false);
		EnableToolbarItem("Heightmap", HM_DELETE_ROW, false);
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_BEFORE, false);
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_AFTER, false);
		EnableToolbarItem("Heightmap", HM_DELETE_COLUMN, false);
		EnableToolbarItem("Heightmap", HM_TOGGLE_PLAYER, false);
		EnableToolbarItem("Heightmap", HM_TOGGLE_NPC, false);
		EnableToolbarItem("Heightmap", HM_TOGGLE_RAFT, false);
		EnableToolbarItem("Heightmap", HM_INCREASE_HEIGHT, false);
		EnableToolbarItem("Heightmap", HM_DECREASE_HEIGHT, false);
		if (hmcell != nullptr && hmzoom != nullptr)
		{
			hmcell->SetSelection(0);
			hmcell->Enable(false);
			hmzoom->Enable(false);
		}

		EnableToolbarItem("Tilemap", TM_CLEAR, false);
		EnableToolbarItem("Tilemap", TM_DELETE_COLUMN, false);
		EnableToolbarItem("Tilemap", TM_DELETE_ROW, false);
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_AFTER, false);
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_BEFORE, false);
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_AFTER, false);
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_BEFORE, false);
	}
	else if (m_mode == RoomEdit::Mode::HEIGHTMAP)
	{
		CheckMenuItem(ID_VIEW_ALPHA, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_ALPHA, false);
		EnableMenuItem(ID_VIEW_ALPHA, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_ALPHA, false);

		CheckMenuItem(ID_VIEW_ENTITIES, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITIES, false);
		EnableMenuItem(ID_VIEW_ENTITIES, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITIES, false);

		CheckMenuItem(ID_VIEW_ENTITY_HITBOX, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, false);
		EnableMenuItem(ID_VIEW_ENTITY_HITBOX, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, false);

		CheckMenuItem(ID_VIEW_WARPS, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_WARPS, false);
		EnableMenuItem(ID_VIEW_WARPS, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_WARPS, false);

		CheckMenuItem(ID_VIEW_SWAPS, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_SWAPS, false);
		EnableMenuItem(ID_VIEW_SWAPS, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_SWAPS, false);

		EnableMenuItem(ID_EDIT_ENTITY_PROPERTIES, false);
		EnableToolbarItem("Main", ID_EDIT_ENTITY_PROPERTIES, false);

		EnableToolbarItem("Heightmap", HM_INSERT_ROW_BEFORE, m_hmedit->IsSingleSelection());
		EnableToolbarItem("Heightmap", HM_INSERT_ROW_AFTER, m_hmedit->IsSingleSelection());
		EnableToolbarItem("Heightmap", HM_DELETE_ROW, m_hmedit->IsSingleSelection());
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_BEFORE, m_hmedit->IsSingleSelection());
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_AFTER, m_hmedit->IsSingleSelection());
		EnableToolbarItem("Heightmap", HM_DELETE_COLUMN, m_hmedit->IsSingleSelection());
		EnableToolbarItem("Heightmap", HM_TOGGLE_PLAYER, !m_hmedit->IsSelectionEmpty());
		EnableToolbarItem("Heightmap", HM_TOGGLE_NPC, !m_hmedit->IsSelectionEmpty());
		EnableToolbarItem("Heightmap", HM_TOGGLE_RAFT, !m_hmedit->IsSelectionEmpty());
		EnableToolbarItem("Heightmap", HM_INCREASE_HEIGHT, !m_hmedit->IsSelectionEmpty() && m_hmedit->AnySelectedMaxHeight());
		EnableToolbarItem("Heightmap", HM_DECREASE_HEIGHT, !m_hmedit->IsSelectionEmpty() && m_hmedit->AnySelectedMinHeight());
		if (hmcell != nullptr && hmzoom != nullptr)
		{
			hmcell->Enable(m_hmedit->IsSelectionEmpty());
			hmzoom->Enable(true);
		}

		CheckToolbarItem("Heightmap", HM_TOGGLE_PLAYER, !m_hmedit->IsSelectionEmpty() && m_hmedit->IsSelectedPlayerPassable());
		CheckToolbarItem("Heightmap", HM_TOGGLE_NPC, !m_hmedit->IsSelectionEmpty() && !m_hmedit->IsSelectedNPCPassable());
		CheckToolbarItem("Heightmap", HM_TOGGLE_RAFT, !m_hmedit->IsSelectionEmpty() && m_hmedit->IsSelectedRaftTrack());
		if (hmcell != nullptr && hmzoom != nullptr)
		{
			hmzoom->Enable(true);
			if (!m_hmedit->IsSelectionEmpty())
			{
				if(m_hmedit->IsMultipleSelection())
				{
					hmcell->Enable(false);
					hmcell->SetSelection(0);
				}
				else
				{
					hmcell->Enable(true);
					hmcell->SetSelection(m_hmedit->GetSelectedType(0) > 0x2F ? 0x30 : m_hmedit->GetSelectedType(0));
				}
			}
			else
			{
				hmcell->Enable(true);
				hmcell->SetSelection(0);
			}
		}
		EnableToolbarItem("Tilemap", TM_CLEAR, false);
		EnableToolbarItem("Tilemap", TM_DELETE_COLUMN, false);
		EnableToolbarItem("Tilemap", TM_DELETE_ROW, false);
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_AFTER, false);
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_BEFORE, false);
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_AFTER, false);
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_BEFORE, false);
	}
	else if (m_mode == RoomEdit::Mode::FOREGROUND || m_mode == RoomEdit::Mode::BACKGROUND)
	{
		CheckMenuItem(ID_VIEW_ALPHA, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_ALPHA, false);
		EnableMenuItem(ID_VIEW_ALPHA, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_ALPHA, false);

		CheckMenuItem(ID_VIEW_ENTITIES, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITIES, false);
		EnableMenuItem(ID_VIEW_ENTITIES, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITIES, false);

		CheckMenuItem(ID_VIEW_ENTITY_HITBOX, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, false);
		EnableMenuItem(ID_VIEW_ENTITY_HITBOX, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, false);

		CheckMenuItem(ID_VIEW_WARPS, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_WARPS, false);
		EnableMenuItem(ID_VIEW_WARPS, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_WARPS, false);

		CheckMenuItem(ID_VIEW_SWAPS, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_SWAPS, true);
		EnableMenuItem(ID_VIEW_SWAPS, true);
		EnableToolbarItem("Main", TOOL_TOGGLE_SWAPS, true);

		EnableMenuItem(ID_EDIT_ENTITY_PROPERTIES, false);
		EnableToolbarItem("Main", ID_EDIT_ENTITY_PROPERTIES, false);

		CheckToolbarItem("Heightmap", HM_TOGGLE_PLAYER, false);
		CheckToolbarItem("Heightmap", HM_TOGGLE_NPC, false);
		CheckToolbarItem("Heightmap", HM_TOGGLE_RAFT, false);

		EnableToolbarItem("Heightmap", HM_INSERT_ROW_BEFORE, false);
		EnableToolbarItem("Heightmap", HM_INSERT_ROW_AFTER, false);
		EnableToolbarItem("Heightmap", HM_DELETE_ROW, false);
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_BEFORE, false);
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_AFTER, false);
		EnableToolbarItem("Heightmap", HM_DELETE_COLUMN, false);
		EnableToolbarItem("Heightmap", HM_TOGGLE_PLAYER, false);
		EnableToolbarItem("Heightmap", HM_TOGGLE_NPC, false);
		EnableToolbarItem("Heightmap", HM_TOGGLE_RAFT, false);
		EnableToolbarItem("Heightmap", HM_INCREASE_HEIGHT, false);
		EnableToolbarItem("Heightmap", HM_DECREASE_HEIGHT, false);
		if (hmcell != nullptr && hmzoom != nullptr)
		{
			hmcell->SetSelection(0);
			hmcell->Enable(false);
			hmzoom->Enable(false);
		}
		EnableToolbarItem("Tilemap", TM_CLEAR, true);
		EnableToolbarItem("Tilemap", TM_DELETE_COLUMN, m_bgedit->IsCellSelected());
		EnableToolbarItem("Tilemap", TM_DELETE_ROW, m_bgedit->IsCellSelected());
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_AFTER, m_bgedit->IsCellSelected());
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_BEFORE, m_bgedit->IsCellSelected());
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_AFTER, m_bgedit->IsCellSelected());
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_BEFORE, m_bgedit->IsCellSelected());
	}
}

void RoomViewerFrame::OnKeyDown(wxKeyEvent& evt)
{
	evt.Skip(!HandleKeyDown(evt.GetKeyCode(), evt.GetModifiers()));
}

void RoomViewerFrame::OnKeyUp(wxKeyEvent& evt)
{
	evt.Skip(!HandleKeyUp(evt.GetKeyCode(), evt.GetModifiers()));
}

void RoomViewerFrame::OnZoomChange(wxCommandEvent& /*evt*/)
{
	m_roomview->SetZoom(m_layerctrl->GetZoom());
}

void RoomViewerFrame::OnOpacityChange(wxCommandEvent& evt)
{
	LayerControlFrame::Layer layer = static_cast<LayerControlFrame::Layer>(reinterpret_cast<intptr_t>(evt.GetClientData()));
	switch (layer)
	{
	case LayerControlFrame::Layer::BG1:
		m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::BACKGROUND1, m_layerctrl->GetLayerOpacity(layer));
		break;
	case LayerControlFrame::Layer::BG2:
		m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::BACKGROUND2, m_layerctrl->GetLayerOpacity(layer));
		break;
	case LayerControlFrame::Layer::FG:
		m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::FOREGROUND, m_layerctrl->GetLayerOpacity(layer));
		break;
	case LayerControlFrame::Layer::SPRITES:
		m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::BG_SPRITES, m_layerctrl->GetLayerOpacity(layer));
		m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::FG_SPRITES, m_layerctrl->GetLayerOpacity(layer));
		break;
	case LayerControlFrame::Layer::HM:
		m_roomview->SetLayerOpacity(RoomViewerCtrl::Layer::HEIGHTMAP, m_layerctrl->GetLayerOpacity(layer));
		break;
	}
	m_roomview->RefreshGraphics();
}

void RoomViewerFrame::OnEntityUpdate(wxCommandEvent& /*evt*/)
{
	m_hmedit->UpdateEntities(m_roomview->GetEntities());
	m_entityctrl->SetEntities(m_roomview->GetEntities());
	m_entityctrl->SetSelected(m_roomview->GetSelectedEntityIndex());
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	UpdateUI();
}

void RoomViewerFrame::OnEntitySelect(wxCommandEvent& /*evt*/)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	UpdateUI();
}

void RoomViewerFrame::OnEntityOpenProperties(wxCommandEvent& /*evt*/)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	m_roomview->UpdateEntityProperties(m_entityctrl->GetSelected());
}

void RoomViewerFrame::OnEntityAdd(wxCommandEvent& /*evt*/)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->AddEntity();
}

void RoomViewerFrame::OnEntityDelete(wxCommandEvent& /*evt*/)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	m_roomview->DeleteSelectedEntity();
}

void RoomViewerFrame::OnEntityMoveUp(wxCommandEvent& /*evt*/)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	m_roomview->MoveSelectedEntityUp();
}

void RoomViewerFrame::OnEntityMoveDown(wxCommandEvent& /*evt*/ )
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	m_roomview->MoveSelectedEntityDown();
}

void RoomViewerFrame::OnWarpUpdate(wxCommandEvent& /*evt*/ )
{
	m_warpctrl->SetWarps(m_roomview->GetWarps());
	m_hmedit->UpdateWarps(m_roomview->GetWarps());
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_entityctrl->SetSelected(m_roomview->GetSelectedEntityIndex());
	UpdateUI();
}

void RoomViewerFrame::OnWarpSelect(wxCommandEvent& /*evt*/)
{
	m_roomview->SelectWarp(m_warpctrl->GetSelected());
	m_entityctrl->SetSelected(m_roomview->GetSelectedEntityIndex());
	UpdateUI();
}

void RoomViewerFrame::OnWarpOpenProperties(wxCommandEvent& /*evt*/)
{
	m_roomview->SelectWarp(m_warpctrl->GetSelected());
	m_roomview->UpdateWarpProperties(m_roomview->GetSelectedWarpIndex());
	UpdateUI();
}

void RoomViewerFrame::OnWarpAdd(wxCommandEvent& /*evt*/)
{
	m_roomview->AddWarp();
	UpdateUI();
}

void RoomViewerFrame::OnWarpDelete(wxCommandEvent& /*evt*/)
{
	m_roomview->SelectWarp(m_warpctrl->GetSelected());
	m_roomview->DeleteSelectedWarp();
	UpdateUI();
}

void RoomViewerFrame::TileSwapRefresh()
{
	if (m_g != nullptr)
	{
		m_swapctrl->SetSwaps(m_g->GetRoomData()->GetTileSwaps(m_roomnum), m_g->GetRoomData()->GetDoors(m_roomnum), m_roomnum);
		m_roomview->UpdateSwaps();
		m_fgedit->UpdateSwaps();
		m_bgedit->UpdateSwaps();
		m_hmedit->UpdateSwaps();
		m_roomview->UpdateDoors();
		m_hmedit->UpdateDoors();
		m_fgedit->UpdateDoors();
		m_bgedit->UpdateDoors();
		UpdateUI();
	}
}

void RoomViewerFrame::OnSwapUpdate(wxCommandEvent& /*evt*/)
{
	//OnSwapSelect(evt);
	TileSwapRefresh();
	m_roomview->Refresh();
	m_fgedit->Refresh();
	m_bgedit->Refresh();
	m_hmedit->Refresh();
	UpdateUI();
}

void RoomViewerFrame::OnSwapSelect(wxCommandEvent& evt)
{
	if (evt.GetInt() > 0 && m_swapctrl->GetPage() != TileSwapControlFrame::ID::TILESWAP)
	{
		m_swapctrl->SetPage(TileSwapControlFrame::ID::TILESWAP);
	}
	if (m_swapctrl->GetSelected() != evt.GetInt() ||
		m_roomview->GetSelectedTileSwapIndex() != evt.GetInt() ||
		m_fgedit->GetSelectedSwap() != evt.GetInt() ||
		m_bgedit->GetSelectedSwap() != evt.GetInt() ||
		m_hmedit->GetSelectedSwap() != evt.GetInt())
	{
		wxLogDebug("Event %d", evt.GetInt());
		m_swapctrl->SetSelected(evt.GetInt());
		m_roomview->SelectTileSwap(evt.GetInt());
		m_fgedit->SetSelectedSwap(evt.GetInt());
		m_bgedit->SetSelectedSwap(evt.GetInt());
		m_hmedit->SetSelectedSwap(evt.GetInt());
		UpdateUI();
	}
}

void RoomViewerFrame::OnSwapAdd(wxCommandEvent& /*evt*/)
{
	if (m_g)
	{
		auto swaps = m_g->GetRoomData()->GetTileSwaps(m_roomnum);
		if (swaps.size() < 64UL)
		{
			swaps.push_back(TileSwap({20,20,22,22,1,1}, {10,10,12,12,1,1}, TileSwap::Mode::FLOOR));
			m_g->GetRoomData()->SetTileSwaps(m_roomnum, swaps);
			TileSwapRefresh();
			FireEvent(EVT_TILESWAP_SELECT, swaps.size());
		}
	}
}

void RoomViewerFrame::OnSwapDelete(wxCommandEvent& evt)
{
	if (m_g)
	{
		auto swaps = m_g->GetRoomData()->GetTileSwaps(m_roomnum);
		if (!swaps.empty() && evt.GetInt() > 0 && evt.GetInt() <= static_cast<int>(swaps.size()))
		{
			swaps.erase(swaps.begin() + evt.GetInt() - 1);
			m_g->GetRoomData()->SetTileSwaps(m_roomnum, swaps);
			TileSwapRefresh();
			FireEvent(EVT_TILESWAP_SELECT, evt.GetInt() - 1);
		}
	}
}

void RoomViewerFrame::OnSwapMoveUp(wxCommandEvent& evt)
{
	if (m_g)
	{
		auto swaps = m_g->GetRoomData()->GetTileSwaps(m_roomnum);
		if (swaps.size() > 1 && evt.GetInt() > 1 && evt.GetInt() <= static_cast<int>(swaps.size()))
		{
			std::iter_swap(swaps.begin() + evt.GetInt() - 2, swaps.begin() + evt.GetInt() - 1);
			m_g->GetRoomData()->SetTileSwaps(m_roomnum, swaps);
			TileSwapRefresh();
			FireEvent(EVT_TILESWAP_SELECT, evt.GetInt() - 1);
		}
	}
}

void RoomViewerFrame::OnSwapMoveDown(wxCommandEvent& evt)
{
	if (m_g)
	{
		auto swaps = m_g->GetRoomData()->GetTileSwaps(m_roomnum);
		if (swaps.size() > 1 && evt.GetInt() > 0 && evt.GetInt() < static_cast<int>(swaps.size()))
		{
			std::iter_swap(swaps.begin() + evt.GetInt() - 1, swaps.begin() + evt.GetInt());
			m_g->GetRoomData()->SetTileSwaps(m_roomnum, swaps);
			TileSwapRefresh();
			FireEvent(EVT_TILESWAP_SELECT, evt.GetInt() + 1);
		}
	}
}

void RoomViewerFrame::OnSwapProperties(wxCommandEvent& evt)
{
	ShowTileswapDialog(true, TileSwapDialog::PageType::SWAPS, evt.GetInt());
}

void RoomViewerFrame::OnDoorUpdate(wxCommandEvent& /*evt*/)
{
	//OnDoorSelect(evt);
	TileSwapRefresh();
	m_roomview->Refresh();
	m_fgedit->Refresh();
	m_bgedit->Refresh();
}

void RoomViewerFrame::OnDoorSelect(wxCommandEvent& evt)
{
	if (evt.GetInt() > 0 && m_swapctrl->GetPage() != TileSwapControlFrame::ID::DOOR)
	{
		m_swapctrl->SetPage(TileSwapControlFrame::ID::DOOR);
	}
	if (m_swapctrl->GetSelected() != evt.GetInt() ||
		m_roomview->GetSelectedDoorIndex() != evt.GetInt() ||
		m_fgedit->GetSelectedDoor() != evt.GetInt() ||
		m_bgedit->GetSelectedDoor() != evt.GetInt() ||
		m_hmedit->GetSelectedDoor() != evt.GetInt())
	{
		wxLogDebug("Event %d", evt.GetInt());
		m_swapctrl->SetSelected(evt.GetInt());
		m_roomview->SelectDoor(evt.GetInt());
		m_fgedit->SetSelectedDoor(evt.GetInt());
		m_bgedit->SetSelectedDoor(evt.GetInt());
		m_hmedit->SetSelectedDoor(evt.GetInt());
	}
}

void RoomViewerFrame::OnDoorAdd(wxCommandEvent& /*evt*/)
{
	if (m_g)
	{
		auto doors = m_g->GetRoomData()->GetDoors(m_roomnum);
		if (doors.size() < 64UL)
		{
			doors.push_back(Door(20_u8, 20_u8, Door::Size::DOOR_1X4));
			m_g->GetRoomData()->SetDoors(m_roomnum, doors);
			TileSwapRefresh();
			FireEvent(EVT_DOOR_SELECT, doors.size());
		}
	}
}

void RoomViewerFrame::OnDoorDelete(wxCommandEvent& evt)
{
	if (m_g)
	{
		auto doors = m_g->GetRoomData()->GetDoors(m_roomnum);
		if (!doors.empty() && evt.GetInt() > 0 && evt.GetInt() <= static_cast<int>(doors.size()))
		{
			doors.erase(doors.begin() + evt.GetInt() - 1);
			m_g->GetRoomData()->SetDoors(m_roomnum, doors);
			TileSwapRefresh();
			FireEvent(EVT_DOOR_SELECT, evt.GetInt() - 1);
		}
	}
}

void RoomViewerFrame::OnDoorMoveUp(wxCommandEvent& evt)
{
	if (m_g)
	{
		auto doors = m_g->GetRoomData()->GetDoors(m_roomnum);
		if (doors.size() > 1 && evt.GetInt() > 1 && evt.GetInt() <= static_cast<int>(doors.size()))
		{
			std::iter_swap(doors.begin() + evt.GetInt() - 2, doors.begin() + evt.GetInt() - 1);
			m_g->GetRoomData()->SetDoors(m_roomnum, doors);
			TileSwapRefresh();
			FireEvent(EVT_DOOR_SELECT, evt.GetInt() - 1);
		}
	}
}

void RoomViewerFrame::OnDoorMoveDown(wxCommandEvent& evt)
{
	if (m_g)
	{
		auto doors = m_g->GetRoomData()->GetDoors(m_roomnum);
		if (doors.size() > 1 && evt.GetInt() > 0 && evt.GetInt() < static_cast<int>(doors.size()))
		{
			std::iter_swap(doors.begin() + evt.GetInt() - 1, doors.begin() + evt.GetInt());
			m_g->GetRoomData()->SetDoors(m_roomnum, doors);
			TileSwapRefresh();
			FireEvent(EVT_DOOR_SELECT, evt.GetInt() + 1);
		}
	}
}

void RoomViewerFrame::OnDoorProperties(wxCommandEvent& evt)
{
	ShowTileswapDialog(true, TileSwapDialog::PageType::DOORS, evt.GetInt());
}

void RoomViewerFrame::OnHeightmapUpdate(wxCommandEvent& /*evt*/)
{
	m_roomview->RefreshHeightmap();
}

void RoomViewerFrame::OnHeightmapMove(wxCommandEvent& evt)
{
	m_roomview->RefreshGraphics();
	evt.Skip();
}

void RoomViewerFrame::OnHeightmapSelect(wxCommandEvent& /*evt*/)
{
	UpdateUI();
}

void RoomViewerFrame::OnHMTypeSelect(wxCommandEvent& evt)
{
	wxChoice* ctrl = static_cast<wxChoice*>(evt.GetEventObject());
	if (ctrl != nullptr)
	{
		m_hmedit->SetSelectedType(0, ctrl->GetSelection());
	}
	UpdateUI();
	evt.Skip();
}

void RoomViewerFrame::OnHMZoom(wxCommandEvent& evt)
{
	wxSlider* ctrl = static_cast<wxSlider*>(evt.GetEventObject());
	if (ctrl != nullptr)
	{
		m_hmedit->SetZoom(static_cast<double>(ctrl->GetValue()) * 0.5);
	}
	evt.Skip();
}

void RoomViewerFrame::OnBlockSelect(wxCommandEvent& evt)
{
	auto block = evt.GetInt();
	if (m_bgedit != nullptr)
	{
		m_bgedit->SetSelectedBlock(block);
	}
	if (m_fgedit != nullptr)
	{
		m_fgedit->SetSelectedBlock(block);
	}
	if (m_blkctrl != nullptr)
	{
		m_blkctrl->SetBlockSelection(block);
	}
	evt.Skip();
}

void RoomViewerFrame::OnMapUpdate(wxCommandEvent& evt)
{
	if (m_roomview)
	{
		m_roomview->RefreshLayers();
	}
	if (m_fgedit && static_cast<Tilemap3D::Layer>(evt.GetInt()) == Tilemap3D::Layer::BG)
	{
		m_fgedit->RefreshGraphics();
	}
	evt.Skip();
}

void RoomViewerFrame::OnMapCellSelect(wxCommandEvent& evt)
{
	if (m_bgedit)
	{
		m_bgedit->SetSelectedCell({ evt.GetInt(), evt.GetExtraLong() });
	}
	if (m_fgedit)
	{
		m_fgedit->SetSelectedCell({ evt.GetInt(), evt.GetExtraLong() });
	}
	UpdateUI();
	evt.Skip();
}

void RoomViewerFrame::OnSize(wxSizeEvent& evt)
{
	evt.Skip();
}

void RoomViewerFrame::OnTabChange(wxAuiNotebookEvent& evt)
{
	SetMode(static_cast<RoomEdit::Mode>(evt.GetSelection()));
	evt.Skip();
}

void RoomViewerFrame::FireUpdateStatusEvent(const std::string& data, int pane)
{
	wxCommandEvent evt(EVT_STATUSBAR_UPDATE);
	evt.SetString(data); 
	evt.SetInt(pane);
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void RoomViewerFrame::FireEvent(const wxEventType& e)
{
	wxCommandEvent evt(e);
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void RoomViewerFrame::FireEvent(const wxEventType& e, const std::string& userdata)
{
	wxCommandEvent evt(e);
	evt.SetString(userdata);
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void RoomViewerFrame::FireEvent(const wxEventType& e, int userdata)
{
	wxCommandEvent evt(e);
	evt.SetInt(userdata);
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void RoomViewerFrame::SetPaneSizes()
{
	int w, h;
	if (!m_sizes_set)
	{
		w = GetClientSize().GetWidth() * 7 / 10;
		h = GetClientSize().GetHeight();
		wxMessageBox(std::to_string(w));

		//wxAUI hack: set minimum height to desired value, then call wxAuiPaneInfo::Fixed() to apply it
		m_mgr.GetPane(m_hmedit).BestSize(w, h);
		m_mgr.GetPane(m_hmedit).Fixed();
		m_mgr.Update();

		//now make resizable again
		m_mgr.GetPane(m_hmedit).Resizable();
		m_mgr.Update();

		m_sizes_set = true;
	}
}

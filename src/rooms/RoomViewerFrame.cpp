#include <rooms/RoomViewerFrame.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <wx/busyinfo.h>
#include <wx/dir.h>
#include <wx/progdlg.h>
#include <main/ImageBufferWx.h>
#include <rooms/FlagDialog.h>
#include <rooms/ChestDialog.h>
#include <rooms/CharacterDialog.h>
#include <rooms/EntityPropertiesWindow.h>
#include <rooms/MapFileIo.h>
#include <rooms/MapManagerDialog.h>
#include <rooms/RoomErrorDialog.h>
#include <rooms/RoomManagerDialog.h>
#include <rooms/BlocksetManagerDialog.h>
#include <rooms/TilesetManagerDialog.h>
#include <rooms/TileSwapDialog.h>
#include <rooms/WarpPropertyWindow.h>
#include <landstalker/misc/Labels.h>
#include <landstalker/3d_maps/MapToTmx.h>
#include <landstalker/3d_maps/RoomToTmx.h>
#include <landstalker/3d_maps/RoomToYaml.h>
#include <rooms/gpu/GLCanvas.h>

wxDEFINE_EVENT(EVT_ENTITY_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_WARP_UPDATE, wxCommandEvent);

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
	ID_FILE_EXPORT_ROOM_METADATA,
	ID_FILE_EXPORT_ALL_ROOM_METADATA,
	ID_FILE_SEP1,
	ID_FILE_IMPORT_ROOM_METADATA,
	ID_FILE_IMPORT_BIN,
	ID_FILE_IMPORT_CSV,
	ID_FILE_IMPORT_TMX,
	ID_FILE_IMPORT_ALL_TMX,
	ID_EDIT,
	ID_EDIT_ROOMS,
	ID_EDIT_MAPS,
	ID_EDIT_TILESETS,
	ID_EDIT_BLOCKSETS,
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
	ID_VIEW_ROOM,
	ID_VIEW_HEIGHTMAP,
	ID_VIEW_BACKGROUND,
	ID_VIEW_FOREGROUND,
	ID_VIEW_SEP1,
	ID_VIEW_ALPHA,
	ID_VIEW_ENTITIES,
	ID_VIEW_ENTITY_HITBOX,
	ID_VIEW_WARPS,
	ID_VIEW_SWAPS,
	ID_VIEW_SEP2,
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
	TOOL_UNDO,
	TOOL_REDO,
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
	TM_DELETE_COLUMN,
	TM_TOGGLE_PRIORITY_HIGHLIGHT,
	MODE_ROOM,
	MODE_HEIGHTMAP,
	MODE_BACKGROUND,
	MODE_FOREGROUND,
	TOOL_SELECT,
	TOOL_DRAW,
	TOOL_LINE,
	TOOL_FILLED_RECT,
	TOOL_OUTLINE_RECT,
	TOOL_FILLED_CIRCLE,
	TOOL_OUTLINE_CIRCLE,
	TOOL_FLOODFILL,
	TOOL_STAMP,
	TOOL_CLEAR
};

wxBEGIN_EVENT_TABLE(RoomViewerFrame, wxWindow)
EVT_KEY_DOWN(RoomViewerFrame::OnKeyDown)
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
EVT_COMMAND(wxID_ANY, EVT_BLOCK_SELECT, RoomViewerFrame::OnBlockSelect)
EVT_COMMAND(wxID_ANY, EVT_GPU_EDITOR_MODE_CHANGE, RoomViewerFrame::OnGpuEditorModeChange)
EVT_COMMAND(wxID_ANY, EVT_GPU_LAYER_OPACITY_CHANGE, RoomViewerFrame::OnGpuLayerOpacityChange)
EVT_COMMAND(wxID_ANY, EVT_GPU_LAYER_BLOCK_SELECT, RoomViewerFrame::OnGpuLayerBlockSelect)
EVT_COMMAND(wxID_ANY, EVT_GPU_HEIGHTMAP_TARGET_CHANGE, RoomViewerFrame::OnGpuHeightmapTargetChange)
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
EVT_SIZE(RoomViewerFrame::OnSize)
wxEND_EVENT_TABLE()

using namespace Landstalker;

namespace
{
float LayerOpacityToFloat(uint8_t opacity)
{
	return static_cast<float>(opacity) / 255.0f;
}

MyGLCanvas::EditorMode ToGpuEditorMode(RoomEdit::Mode mode)
{
	switch (mode)
	{
	case RoomEdit::Mode::HEIGHTMAP:
		return MyGLCanvas::EditorMode::Heightmap;
	case RoomEdit::Mode::BACKGROUND:
		return MyGLCanvas::EditorMode::BackgroundLayer;
	case RoomEdit::Mode::FOREGROUND:
		return MyGLCanvas::EditorMode::ForegroundLayer;
	case RoomEdit::Mode::NORMAL:
	default:
		return MyGLCanvas::EditorMode::Room;
	}
}

RoomEdit::Mode ToRoomEditMode(MyGLCanvas::EditorMode mode)
{
	switch (mode)
	{
	case MyGLCanvas::EditorMode::Heightmap:
		return RoomEdit::Mode::HEIGHTMAP;
	case MyGLCanvas::EditorMode::BackgroundLayer:
		return RoomEdit::Mode::BACKGROUND;
	case MyGLCanvas::EditorMode::ForegroundLayer:
		return RoomEdit::Mode::FOREGROUND;
	case MyGLCanvas::EditorMode::Room:
	default:
		return RoomEdit::Mode::NORMAL;
	}
}
}

RoomViewerFrame::RoomViewerFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst),
	  m_mode(RoomEdit::Mode::NORMAL),
	  m_title(""),
	  m_gpuview(nullptr),
	  m_direction_input_mode(GLCanvasDirectionInputMode::UpIsNorthEast),
	  m_layerctrl(nullptr),
	  m_entityctrl(nullptr),
	  m_warpctrl(nullptr),
	  m_swapctrl(nullptr),
	  m_blkctrl(nullptr),
	  m_g(nullptr),
	  m_roomnum(0),
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

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();

	this->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_entityctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_warpctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_swapctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_blkctrl->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
}

RoomViewerFrame::~RoomViewerFrame()
{
	this->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);

	m_entityctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_warpctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_swapctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_blkctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
}

void RoomViewerFrame::SetMode(RoomEdit::Mode mode)
{
	if (m_mode == mode)
	{
		return;
	}
	m_mode = mode;
	m_swapctrl->SetMode(m_mode);
	UpdateUI();
	UpdateFrame();
}

void RoomViewerFrame::SetDirectionInputMode(GLCanvasDirectionInputMode mode)
{
	m_direction_input_mode = mode;
	if (m_gpuview != nullptr)
	{
		m_gpuview->SetDirectionInputMode(mode);
	}
}

void RoomViewerFrame::UpdateFrame()
{
	m_layerctrl->EnableLayers(m_mode == RoomEdit::Mode::NORMAL);
	m_blkctrl->OpenRoom(m_roomnum);
	if (m_gpuview)
	{
		m_gpuview->SetRoomNum(m_roomnum);
	}
	RefreshObjectLists();
	UpdateUI();
	FireEvent(EVT_STATUSBAR_UPDATE);
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void RoomViewerFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	if (m_entity_dialog != nullptr && gd != m_g)
	{
		// The cached dialog holds combo lists and script trees built from the old game data.
		m_entity_dialog->Destroy();
		m_entity_dialog = nullptr;
	}
	m_g = gd;
	if (m_gpuview == nullptr && gd != nullptr)
	{
		m_gpuview = new MyGLCanvas(this, gd);
		m_gpuview->SetDirectionInputMode(m_direction_input_mode);
		m_mgr.AddPane(m_gpuview, wxAuiPaneInfo().CenterPane().PaneBorder(false));
		m_mgr.Update();
		// The room page can be logically disabled while its contents are being
		// created. On MSW, wxGLCanvas then inherits WS_DISABLED from the parent,
		// but its wx-level enabled flag remains true and is not resynchronized
		// when the page becomes active. Cycle the local state after attachment so
		// the native canvas can receive mouse input.
		m_gpuview->Disable();
		m_gpuview->Enable();
		m_gpuview->SetFocus();
		SyncGpuViewControls();
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
	if (m_entity_dialog != nullptr)
	{
		m_entity_dialog->Destroy();
		m_entity_dialog = nullptr;
	}
	if (m_gpuview != nullptr)
	{
		m_mgr.DetachPane(m_gpuview);
		m_gpuview->Destroy();
		m_gpuview = nullptr;
		m_mgr.Update();
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
		m_g->GetRoomData()->CleanupChests(*m_g);
	}
	if (m_roomnum != roomnum)
	{
		m_reset_props = true;
	}
	m_roomnum = roomnum;
	m_blkctrl->SetBlockSelection(0);
	if (m_gpuview)
	{
		m_gpuview->SetSelectedBlockId(0);
	}
	UpdateUI();
	UpdateFrame();
}

bool RoomViewerFrame::ExportBin(const std::string& path)
{
	return MapFileIo::ExportCmp(path, m_g->GetRoomData()->GetMapForRoom(m_roomnum));
}

bool RoomViewerFrame::ExportCsv(const std::array<std::string, 3>& paths)
{
	return MapFileIo::ExportCsv(paths, *m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData());
}

bool RoomViewerFrame::ExportAllCsv(const std::string& dir)
{
	wxSetWorkingDirectory(dir);

	auto dialog = wxProgressDialog("Export", "Exporting Maps", m_g->GetRoomData()->GetRoomCount(), this);
	std::set<std::string> exported;

	for (std::size_t i = 0; i < m_g->GetRoomData()->GetRoomCount(); ++i)
	{
		m_roomnum = i;
		
		auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
		if (exported.find(rd->map) != exported.cend())
		{
			continue;
		}
		dialog.Update(i, "Exporting " + rd->map + "...");
		wxYield();

		const std::string background = rd->map + "_background.csv";
		const std::string heightmap =  rd->map + "_heightmap.csv";
		const std::string foreground = rd->map + "_foreground.csv";
		
		std::array<std::string, 3> paths = {background, foreground, heightmap};
		ExportCsv(paths);
		exported.insert(rd->map);
	}

	return true;
}

bool RoomViewerFrame::ExportTmx(const std::string& tmx_path, const std::string& bs_path, uint16_t roomnum)
{
	MapFileIo::RenderBlocksetPng(bs_path, m_g, roomnum);
	return MapFileIo::ExportTmx(tmx_path, *m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData(), bs_path);
}

bool RoomViewerFrame::ExportAllTmx(const std::string& dir)
{
	wxString curdir = wxGetCwd();
	wxSetWorkingDirectory(dir);
	std::filesystem::path mappath(dir);
	std::filesystem::path bspath(mappath / "blocksets");
	std::filesystem::create_directories(bspath);
	auto dialog = wxProgressDialog("Export", "Exporting Maps", m_g->GetRoomData()->GetRoomCount(), this);
	std::set<std::string> exported;
	for (std::size_t i = 0; i < m_g->GetRoomData()->GetRoomCount(); ++i)
	{
		auto rd = m_g->GetRoomData()->GetRoom(i);
		std::string mapfile = rd->map + ".tmx";
		if (exported.find(rd->map) != exported.cend())
		{
			continue;
		}
		std::string blkname = StrPrintf("BT%02d_%01d%01d_p%02d.png", rd->tileset + 1, rd->pri_blockset, rd->sec_blockset + 1, rd->room_palette + 1);
		std::string blkpath = "blocksets";
		blkpath += wxString(wxFileName::GetPathSeparator()).ToStdString() + blkname;
		dialog.Update(i, "Exporting " + rd->map + "...");
		wxYield();
		ExportTmx(mapfile, blkpath, i);
		exported.insert(rd->map);
	}
	wxSetWorkingDirectory(curdir);
	return true;
}

bool RoomViewerFrame::ExportRoomTmx(const std::string& tmx_path, const std::string& bs_path, uint16_t roomnum)
{
	MapFileIo::RenderBlocksetPng(bs_path, m_g, roomnum);
	return RoomToTmx::ExportToTmx(tmx_path, roomnum, m_g, bs_path);
}

bool RoomViewerFrame::ExportAllRoomsTmx(const std::string& dir)
{
	wxBusyInfo wait("Exporting...");
	wxString curdir = wxGetCwd();
	wxSetWorkingDirectory(dir);
	std::filesystem::path mappath(dir);
	std::filesystem::path bspath(mappath / "blocksets");
	std::filesystem::create_directories(bspath);
	auto dialog = wxProgressDialog("Export", "Exporting Rooms", m_g->GetRoomData()->GetRoomCount(), this);
	for (std::size_t i = 0; i < m_g->GetRoomData()->GetRoomCount(); ++i)
	{
		auto rd = m_g->GetRoomData()->GetRoom(i);
		std::string roomfile = rd->name + ".tmx";
		std::string blkname = StrPrintf("BT%02d_%01d%01d_p%02d.png", rd->tileset + 1, rd->pri_blockset, rd->sec_blockset + 1, rd->room_palette + 1);
		std::string blkpath = "blocksets";
		blkpath += wxString(wxFileName::GetPathSeparator()).ToStdString() + blkname;
		dialog.Update(i, Landstalker::StrWPrintf("Exporting %s...", m_g->GetRoomData()->GetRoomDisplayName(i).c_str()));
		wxYield();
		ExportRoomTmx(roomfile, blkpath, i);
	}
	wxSetWorkingDirectory(curdir);
	return true;
}

bool RoomViewerFrame::ExportRoomMetadata(const std::string& path, uint16_t roomnum)
{
	if (!m_g || roomnum >= m_g->GetRoomData()->GetRoomCount())
	{
		return false;
	}

	return RoomToYaml::ExportToYaml(path, roomnum, m_g);
}

bool RoomViewerFrame::ExportAllRoomMetadata(const std::string& dir)
{
	if (!m_g)
	{
		return false;
	}

	const std::filesystem::path output_dir(dir);
	std::error_code error;
	std::filesystem::create_directories(output_dir, error);
	if (error)
	{
		return false;
	}

	auto dialog = wxProgressDialog("Export", "Exporting Room Metadata", m_g->GetRoomData()->GetRoomCount(), this);
	for (std::size_t i = 0; i < m_g->GetRoomData()->GetRoomCount(); ++i)
	{
		const auto room = m_g->GetRoomData()->GetRoom(i);
		dialog.Update(i, Landstalker::StrWPrintf("Exporting %s...", room->GetDisplayName().c_str()));
		wxYield();
		if (!ExportRoomMetadata((output_dir / (room->name + ".yaml")).string(), static_cast<uint16_t>(i)))
		{
			return false;
		}
	}
	return true;
}

bool RoomViewerFrame::ImportRoomMetadata(const std::string& path, uint16_t roomnum)
{
	if (!m_g || roomnum >= m_g->GetRoomData()->GetRoomCount())
	{
		return false;
	}
	return RoomToYaml::ImportFromYaml(path, RoomToYaml::RoomKey{ roomnum }, m_g);
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
	ImageBufferWx buf(width, height);

	buf.Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, map, tileset, blockset);
	buf.Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
	return buf.WritePNG(path, palette, m_gpuview != nullptr && m_gpuview->GetAlpha());
}

bool RoomViewerFrame::ImportBin(const std::string& path)
{
	auto data = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
	if (!MapFileIo::ImportCmp(path, *data))
	{
		return false;
	}
	UpdateFrame();
	return true;
}

bool RoomViewerFrame::ImportTmx(const std::string& paths, uint16_t roomnum)
{
	auto current_map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
	auto validation_map = *current_map;
	if (!MapToTmx::ImportFromTmx(paths, validation_map))
	{
		return false;
	}
	if (!RoomToTmx::ImportFromTmx(paths, RoomToYaml::RoomKey{ roomnum }, m_g))
	{
		return false;
	}
	auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
	return MapToTmx::ImportFromTmx(paths, *map);
}

bool RoomViewerFrame::ImportAllTmx(const std::string& dir)
{
	wxDir d;

	wxString curdir = wxGetCwd();
	wxSetWorkingDirectory(dir);
	if (d.Open(dir))
	{
		wxString file;
		bool cont = d.GetFirst(&file, "*.tmx");
		auto dialog = wxProgressDialog("Import", "Importing Rooms", 1, this);
		while (cont)
		{
			wxFileName name(file);
			dialog.Pulse("Importing " + file);
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
	if (!MapFileIo::ImportCsv(paths, *data))
	{
		return false;
	}
	UpdateFrame();
	return true;
}

bool RoomViewerFrame::HandleKeyDown(unsigned int key, unsigned int modifiers)
{
	if (m_gpuview && (modifiers & wxMOD_CONTROL) != 0 && (modifiers & wxMOD_ALT) == 0)
	{
		if (key == 'Z')
		{
			m_gpuview->Undo();
			UpdateUI();
			return true;
		}
		if (key == 'Y')
		{
			m_gpuview->Redo();
			UpdateUI();
			return true;
		}
	}
	// The entity, warp and tile swap panes route their key presses here, and clicking a row
	// in one of them takes focus off the canvas. Insert adds an object to the room, which is
	// what the user is asking for whichever pane they happen to be in, so pass it on. Only
	// this key: everything else - arrows, Tab, Delete, Home/End - belongs to the list, and
	// forwarding it would move the camera or edit the room while the user navigates.
	if (m_gpuview && key == WXK_INSERT)
	{
		wxKeyEvent forwarded(wxEVT_KEY_DOWN);
		forwarded.m_keyCode = static_cast<int>(key);
		forwarded.SetControlDown((modifiers & wxMOD_CONTROL) != 0);
		forwarded.SetShiftDown((modifiers & wxMOD_SHIFT) != 0);
		forwarded.SetAltDown((modifiers & wxMOD_ALT) != 0);
		return m_gpuview->HandleKeyDown(forwarded);
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
	RoomErrorDialog dlg(this, {});
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
		props.Append(new wxStringProperty("Name", "Name", rd->GetDisplayName()));
		props.Append(new wxStringProperty("Label", "Label", rd->name))->Enable(false);
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
		m_bgms.Add(Labels::Get(Labels::C_BGMS, i).value_or(StrWPrintf(L"[%02X] Track %d", i, i)));
	}

	m_palettes.Clear();
	for (std::size_t i = 0; i < m_g->GetRoomData()->GetRoomPalettes().size(); ++i)
	{
		m_palettes.Add(wxString(m_g->GetRoomData()->GetRoomPaletteDisplayName(i)));
	}

	m_tilesets.Clear();
	for (const auto& p : m_g->GetRoomData()->GetTilesets())
	{
		m_tilesets.Add(wxString(p->GetName()));
	}
	m_pri_blocksets.Clear();
	m_sec_blocksets.Clear();
	for (const auto& p : m_g->GetRoomData()->GetAllBlocksets())
	{
		if (p.second->GetTileset() == rd->tileset)
		{
			if (p.second->GetSecondary() == 0)
			{
				m_pri_blocksets.Add(wxString(p.first));
			}
			else if (p.second->GetPrimary() == rd->pri_blockset)
			{
				m_sec_blocksets.Add(wxString(p.first));
			}
		}
	}
	m_maps.Clear();
	for (const auto& map : m_g->GetRoomData()->GetMapOrder())
	{
		m_maps.Add(map);
	}
	m_rooms.Clear();
	m_rooms.Add("<NONE>");
	for (const auto& room : m_g->GetRoomData()->GetRoomlist())
	{
		m_rooms.Add(wxString(room->GetDisplayName()));
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
		props.GetGrid()->GetProperty("M")->SetChoices(m_maps);

		const auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
		auto tm = m_g->GetRoomData()->GetMapForRoom(m_roomnum);

		props.GetGrid()->SetPropertyValue("Name", wxString(rd->GetDisplayName()));
		props.GetGrid()->SetPropertyValue("Label", wxString(rd->name));
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
		ctrl->GetGrid()->Thaw();
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->CommitPendingEdits();
	}
	const auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
	auto tm = m_g->GetRoomData()->GetMapForRoom(m_roomnum);

	const wxString& name = property->GetName();
	if (name == "Name")
	{
		const auto new_name = Labels::NormalizePath(property->GetValueAsString().ToStdWstring());
		if (new_name && *new_name == rd->GetDisplayName())
		{
			property->SetValueFromString(*new_name);
		}
		else if (new_name && Labels::IsValid(*new_name, Labels::C_ROOMS, m_roomnum))
		{
			FireRenameNavItemEvent(*new_name, rd->GetDisplayName());
			Labels::Update(Labels::C_ROOMS, m_roomnum, *new_name);
			property->SetValueFromString(*new_name);
		}
		else
		{
			property->SetValueFromString(rd->GetDisplayName());
		}
	}
	else if (name == "TS")
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
	if (m_gpuview)
	{
		m_gpuview->ReloadCurrentRoomFromGameData();
	}
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
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
	AddMenuItem(fileMenu, 9, ID_FILE_EXPORT_ROOM_METADATA, "Export Room Metadata...");
	AddMenuItem(fileMenu, 10, ID_FILE_EXPORT_ALL_ROOM_METADATA, "Export All Room Metadata...");
	AddMenuItem(fileMenu, 11, ID_FILE_SEP1, "", wxITEM_SEPARATOR);
	AddMenuItem(fileMenu, 12, ID_FILE_IMPORT_ROOM_METADATA, "Import Room Metadata...");
	AddMenuItem(fileMenu, 13, ID_FILE_IMPORT_BIN, "Import Map from Binary...");
	AddMenuItem(fileMenu, 14, ID_FILE_IMPORT_CSV, "Import Map from CSV...");
	AddMenuItem(fileMenu, 15, ID_FILE_IMPORT_TMX, "Import Room from Tiled TMX...");
	AddMenuItem(fileMenu, 16, ID_FILE_IMPORT_ALL_TMX, "Import All Maps from Tiled TMX...");

	auto& editMenu = AddMenu(menu, 1, ID_EDIT, "Edit");
	AddMenuItem(editMenu, 0, ID_EDIT_ROOMS, "Rooms...\tF10");
	AddMenuItem(editMenu, 1, ID_EDIT_MAPS, "Maps...\tF9");
	AddMenuItem(editMenu, 2, ID_EDIT_TILESETS, "Tilesets...");
	AddMenuItem(editMenu, 3, ID_EDIT_BLOCKSETS, "Blocksets...");
	AddMenuItem(editMenu, 4, ID_EDIT_ENTITY_PROPERTIES, "Selection Properties...");
	AddMenuItem(editMenu, 5, ID_EDIT_FLAGS, "Flags...");
	AddMenuItem(editMenu, 6, ID_EDIT_CHESTS, "Chests...");
	AddMenuItem(editMenu, 7, ID_EDIT_DIALOGUE, "Dialogue...");
	AddMenuItem(editMenu, 8, ID_EDIT_TILESWAPS, "Tile Swaps...");

	auto& viewMenu = AddMenu(menu, 2, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_ROOM, "Room Edit Mode", wxITEM_RADIO);
	AddMenuItem(viewMenu, 1, ID_VIEW_HEIGHTMAP, "Heightmap Edit Mode", wxITEM_RADIO);
	AddMenuItem(viewMenu, 2, ID_VIEW_BACKGROUND, "Background Edit Mode", wxITEM_RADIO);
	AddMenuItem(viewMenu, 3, ID_VIEW_FOREGROUND, "Foreground Edit Mode", wxITEM_RADIO);
	AddMenuItem(viewMenu, 4, ID_VIEW_SEP1, "", wxITEM_SEPARATOR);
	AddMenuItem(viewMenu, 5, ID_VIEW_ALPHA, "Toggle Alpha", wxITEM_CHECK);
	AddMenuItem(viewMenu, 6, ID_VIEW_ENTITIES, "Show Entities", wxITEM_CHECK);
	AddMenuItem(viewMenu, 7, ID_VIEW_ENTITY_HITBOX, "Show Entity Hitboxes", wxITEM_CHECK);
	AddMenuItem(viewMenu, 8, ID_VIEW_WARPS, "Show Warps", wxITEM_CHECK);
	AddMenuItem(viewMenu, 9, ID_VIEW_SWAPS, "Show Tile Swaps / Doors", wxITEM_CHECK);
	AddMenuItem(viewMenu, 10, ID_VIEW_SEP2, "", wxITEM_SEPARATOR);
	AddMenuItem(viewMenu, 11, ID_VIEW_ERRORS, "Errors...");

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
	main_tb->AddTool(MODE_ROOM, "Room Edit Mode", ilist.GetImage("room"), "Room Edit Mode", wxITEM_CHECK);
	main_tb->AddTool(MODE_HEIGHTMAP, "Heightmap Edit Mode", ilist.GetImage("heightmap"), "Heightmap Edit Mode", wxITEM_CHECK);
	main_tb->AddTool(MODE_BACKGROUND, "Background Edit Mode", ilist.GetImage("map_bg_active"), "Background Edit Mode", wxITEM_CHECK);
	main_tb->AddTool(MODE_FOREGROUND, "Foreground Edit Mode", ilist.GetImage("map_fg_active"), "Foreground Edit Mode", wxITEM_CHECK);
	main_tb->AddSeparator();
	main_tb->AddTool(TOOL_UNDO, "Undo", ilist.GetImage("undo"), "Undo", wxITEM_NORMAL);
	main_tb->AddTool(TOOL_REDO, "Redo", ilist.GetImage("redo"), "Redo", wxITEM_NORMAL);
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
	auto hmzoom = new wxSlider(hm_tb, HM_ZOOM, 0, 0, 4);
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
	hm_tb->AddControl(hmzoom, "Z Height");
	AddToolbar(m_mgr, *hm_tb, "Heightmap", "Heightmap Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(2));

	wxAuiToolBar* tm_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	tm_tb->AddTool(TM_CLEAR, "Clear Whole Tilemap", ilist.GetImage("delete"), "Clear Whole Tilemap", wxITEM_NORMAL);
	tm_tb->AddTool(TM_INSERT_ROW_BEFORE, "Insert Row Before", ilist.GetImage("map_insert_se"), "Insert Row Before", wxITEM_NORMAL);
	tm_tb->AddTool(TM_INSERT_ROW_AFTER, "Insert Row After", ilist.GetImage("map_insert_nw"), "Insert Row After", wxITEM_NORMAL);
	tm_tb->AddTool(TM_DELETE_ROW, "Delete Row", ilist.GetImage("map_delete_nesw"), "Delete Row", wxITEM_NORMAL);
	tm_tb->AddTool(TM_INSERT_COLUMN_BEFORE, "Insert Column Before", ilist.GetImage("map_insert_sw"), "Insert Column Before", wxITEM_NORMAL);
	tm_tb->AddTool(TM_INSERT_COLUMN_AFTER, "Insert Column After", ilist.GetImage("map_insert_ne"), "Insert Column After", wxITEM_NORMAL);
	tm_tb->AddTool(TM_DELETE_COLUMN, "Delete Column", ilist.GetImage("map_delete_nwse"), "Delete Column", wxITEM_NORMAL);
	tm_tb->AddSeparator();
	tm_tb->AddTool(TM_TOGGLE_PRIORITY_HIGHLIGHT, "Highlight Priority Tiles", ilist.GetImage("priority"), "Highlight Priority Tiles", wxITEM_CHECK);
	AddToolbar(m_mgr, *tm_tb, "Tilemap", "Tilemap Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(3));

	wxAuiToolBar* tools_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL);
	tools_tb->AddTool(TOOL_SELECT, "Select Cells", ilist.GetImage("selblock"), "Select Cells", wxITEM_CHECK);
	tools_tb->AddTool(TOOL_DRAW, "Draw Cell", ilist.GetImage("pencil"), "Draw Cell", wxITEM_CHECK);
	tools_tb->AddTool(TOOL_LINE, "Draw Line", ilist.GetImage("line"), "Draw Line", wxITEM_CHECK);
	tools_tb->AddTool(TOOL_FILLED_RECT, "Draw Filled Rectangle", ilist.GetImage("rect_filled"), "Draw Filled Rectangle", wxITEM_CHECK);
	tools_tb->AddTool(TOOL_OUTLINE_RECT, "Draw Outlined Rectangle", ilist.GetImage("rect_outline"), "Draw Outlined Rectangle", wxITEM_CHECK);
	tools_tb->AddTool(TOOL_FILLED_CIRCLE, "Draw Filled Circle", ilist.GetImage("circle_filled"), "Draw Filled Circle", wxITEM_CHECK);
	tools_tb->AddTool(TOOL_OUTLINE_CIRCLE, "Draw Outlined Circle", ilist.GetImage("circle_outline"), "Draw Outlined Circle", wxITEM_CHECK);
	tools_tb->AddTool(TOOL_FLOODFILL, "Fill", ilist.GetImage("fill"), "Fill", wxITEM_CHECK);
	tools_tb->AddTool(TOOL_STAMP, "Stamp", ilist.GetImage("stamp"), "Stamp", wxITEM_CHECK);
	tools_tb->AddTool(TOOL_CLEAR, "Clear", ilist.GetImage("delete"), "Clear", wxITEM_NORMAL);
	AddToolbar(m_mgr, *tools_tb, "Drawing Tools", "Drawing Tools", wxAuiPaneInfo().ToolbarPane().Left().Row(1).Position(1));

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
		case ID_FILE_EXPORT_ROOM_METADATA:
			OnExportRoomMetadata();
			break;
		case ID_FILE_EXPORT_ALL_ROOM_METADATA:
			OnExportAllRoomMetadata();
			break;
		case ID_FILE_IMPORT_ROOM_METADATA:
			OnImportRoomMetadata();
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
		case ID_VIEW_ROOM:
		case MODE_ROOM:
			SetGpuEditorMode(RoomEdit::Mode::NORMAL);
			break;
		case ID_VIEW_HEIGHTMAP:
		case MODE_HEIGHTMAP:
			SetGpuEditorMode(RoomEdit::Mode::HEIGHTMAP);
			break;
		case ID_VIEW_BACKGROUND:
		case MODE_BACKGROUND:
			SetGpuEditorMode(RoomEdit::Mode::BACKGROUND);
			break;
		case ID_VIEW_FOREGROUND:
		case MODE_FOREGROUND:
			SetGpuEditorMode(RoomEdit::Mode::FOREGROUND);
			break;
		case ID_VIEW_ALPHA:
		case TOOL_TOGGLE_ALPHA:
			if (m_gpuview)
			{
				m_gpuview->SetAlpha(!m_gpuview->GetAlpha());
			}
			break;
		case ID_VIEW_ENTITIES:
		case TOOL_TOGGLE_ENTITIES:
			if (m_gpuview)
			{
				m_gpuview->SetEntitiesVisible(!m_gpuview->GetEntitiesVisible());
			}
			break;
		case ID_VIEW_ENTITY_HITBOX:
		case TOOL_TOGGLE_ENTITY_HITBOX:
			if (m_gpuview)
			{
				m_gpuview->SetEntitiesHitboxVisible(!m_gpuview->GetEntitiesHitboxVisible());
			}
			break;
		case ID_VIEW_WARPS:
		case TOOL_TOGGLE_WARPS:
			if (m_gpuview)
			{
				m_gpuview->SetWarpsVisible(!m_gpuview->GetWarpsVisible());
			}
			break;
		case ID_VIEW_SWAPS:
		case TOOL_TOGGLE_SWAPS:
			if (m_gpuview)
			{
				m_gpuview->SetTileSwapsVisible(!m_gpuview->GetTileSwapsVisible());
			}
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
		case ID_EDIT_ROOMS:
			ShowRoomManagerDialog();
			break;
		case ID_EDIT_MAPS:
			ShowMapManagerDialog();
			break;
		case ID_EDIT_BLOCKSETS:
			ShowBlocksetManagerDialog();
			break;
		case ID_EDIT_TILESETS:
			ShowTilesetManagerDialog();
			break;
		case ID_EDIT_ENTITY_PROPERTIES:
		case TOOL_SHOW_SELECTION_PROPERTIES:
		{
			const int selected_entity = m_gpuview ? m_gpuview->GetSelectedEntityIndex() : -1;
			const int selected_warp = m_gpuview ? m_gpuview->GetSelectedWarpIndex() : -1;
			const int selected_swap = m_gpuview ? m_gpuview->GetSelectedTileSwapIndex() : -1;
			const int selected_door = m_gpuview ? m_gpuview->GetSelectedDoorIndex() : -1;

			if (selected_entity > 0)
			{
				UpdateEntityProperties(selected_entity);
			}
			else if (selected_warp > 0)
			{
				UpdateWarpProperties(selected_warp);
			}
			else if (selected_swap > 0)
			{
				ShowTileswapDialog(true, TileSwapDialog::PageType::SWAPS, selected_swap);
				if (m_gpuview)
				{
					m_gpuview->SelectTileSwapByIndex(selected_swap);
				}
			}
			else if (selected_door > 0)
			{
				ShowTileswapDialog(true, TileSwapDialog::PageType::DOORS, selected_door);
				if (m_gpuview)
				{
					m_gpuview->SelectDoorByIndex(selected_door);
				}
			}
			break;
		}
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
		case TOOL_UNDO:
			if (m_gpuview) m_gpuview->Undo();
			break;
		case TOOL_REDO:
			if (m_gpuview) m_gpuview->Redo();
			break;
		case HM_INSERT_ROW_BEFORE:
			if (IsGpuViewSelected()) m_gpuview->InsertSelectedHeightmapRowBefore();
			break;
		case HM_INSERT_ROW_AFTER:
			if (IsGpuViewSelected()) m_gpuview->InsertSelectedHeightmapRowAfter();
			break;
		case HM_DELETE_ROW:
			if (IsGpuViewSelected()) m_gpuview->DeleteSelectedHeightmapRow();
			break;
		case HM_INSERT_COLUMN_BEFORE:
			if (IsGpuViewSelected()) m_gpuview->InsertSelectedHeightmapColumnBefore();
			break;
		case HM_INSERT_COLUMN_AFTER:
			if (IsGpuViewSelected()) m_gpuview->InsertSelectedHeightmapColumnAfter();
			break;
		case HM_DELETE_COLUMN:
			if (IsGpuViewSelected()) m_gpuview->DeleteSelectedHeightmapColumn();
			break;
		case HM_TOGGLE_PLAYER:
			if (IsGpuViewSelected()) m_gpuview->ToggleSelectedHeightmapPlayerPassable();
			break;
		case HM_TOGGLE_NPC:
			if (IsGpuViewSelected()) m_gpuview->ToggleSelectedHeightmapNpcPassable();
			break;
		case HM_TOGGLE_RAFT:
			if (IsGpuViewSelected()) m_gpuview->ToggleSelectedHeightmapRaftTrack();
			break;
		case HM_INCREASE_HEIGHT:
			if (IsGpuViewSelected()) m_gpuview->AdjustSelectedHeightmapHeight(1);
			break;
		case HM_DECREASE_HEIGHT:
			if (IsGpuViewSelected()) m_gpuview->AdjustSelectedHeightmapHeight(-1);
			break;
		case HM_NUDGE_HM_NE:
			if (IsGpuViewSelected()) m_gpuview->NudgeHeightmap(0, 1);
			break;
		case HM_NUDGE_HM_NW:
			if (IsGpuViewSelected()) m_gpuview->NudgeHeightmap(1, 0);
			break;
		case HM_NUDGE_HM_SE:
			if (IsGpuViewSelected()) m_gpuview->NudgeHeightmap(-1, 0);
			break;
		case HM_NUDGE_HM_SW:
			if (IsGpuViewSelected()) m_gpuview->NudgeHeightmap(0, -1);
			break;
		case TM_CLEAR:
			if (IsGpuViewSelected()) m_gpuview->ClearCurrentTilemap();
			break;
		case TM_DELETE_COLUMN:
			if (IsGpuViewSelected()) m_gpuview->DeleteSelectedTilemapColumn();
			break;
		case TM_DELETE_ROW:
			if (IsGpuViewSelected()) m_gpuview->DeleteSelectedTilemapRow();
			break;
		case TM_INSERT_COLUMN_BEFORE:
			if (IsGpuViewSelected()) m_gpuview->InsertSelectedTilemapColumnBefore();
			break;
		case TM_INSERT_COLUMN_AFTER:
			if (IsGpuViewSelected()) m_gpuview->InsertSelectedTilemapColumnAfter();
			break;
		case TM_INSERT_ROW_BEFORE:
			if (IsGpuViewSelected()) m_gpuview->InsertSelectedTilemapRowBefore();
			break;
		case TM_INSERT_ROW_AFTER:
			if (IsGpuViewSelected()) m_gpuview->InsertSelectedTilemapRowAfter();
			break;
		case TM_TOGGLE_PRIORITY_HIGHLIGHT:
			if (IsGpuViewSelected()) m_gpuview->ToggleLayerPriorityHighlight();
			break;
		case TOOL_SELECT:
			if (m_gpuview) m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::Select);
			break;
		case TOOL_DRAW:
			if (m_gpuview) m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::Draw);
			break;
		case TOOL_LINE:
			if (m_gpuview) m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::Line);
			break;
		case TOOL_FILLED_RECT:
			if (m_gpuview) m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::FilledRect);
			break;
		case TOOL_OUTLINE_RECT:
			if (m_gpuview) m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::OutlineRect);
			break;
		case TOOL_FILLED_CIRCLE:
			if (m_gpuview) m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::FilledCircle);
			break;
		case TOOL_OUTLINE_CIRCLE:
			if (m_gpuview) m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::OutlineCircle);
			break;
		case TOOL_FLOODFILL:
			if (m_gpuview) m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::FloodFill);
			break;
		case TOOL_STAMP:
			if (m_gpuview) m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::Stamp);
			break;
		case TOOL_CLEAR:
			if (m_gpuview && (m_gpuview->GetEditorMode() == MyGLCanvas::EditorMode::BackgroundLayer ||
			                  m_gpuview->GetEditorMode() == MyGLCanvas::EditorMode::ForegroundLayer))
			{
				m_gpuview->ClearSelectedLayerCells();
			}
			else if (m_gpuview)
			{
				m_gpuview->ClearSelectedHeightmapCells();
			}
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

void RoomViewerFrame::OnExportRoomMetadata()
{
	const auto room = m_g->GetRoomData()->GetRoom(m_roomnum);
	const wxString default_file = room->name + ".yaml";
	wxFileDialog fd(this, _("Export Room Metadata"), "", default_file,
		"YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		try
		{
			if (!ExportRoomMetadata(fd.GetPath().ToStdString(), m_roomnum))
			{
				wxMessageBox("Unable to write room metadata.", "Export Error", wxOK | wxICON_ERROR, this);
			}
		}
		catch (const std::exception& e)
		{
			wxMessageBox("Unable to export room metadata:\n" + wxString::FromUTF8(e.what()),
				"Export Error", wxOK | wxICON_ERROR, this);
		}
	}
}

void RoomViewerFrame::OnExportAllRoomMetadata()
{
	wxDirDialog dd(this, "Select Room Metadata Output Directory");
	if (dd.ShowModal() != wxID_CANCEL)
	{
		try
		{
			if (!ExportAllRoomMetadata(dd.GetPath().ToStdString()))
			{
				wxMessageBox("Unable to write all room metadata.", "Export Error", wxOK | wxICON_ERROR, this);
			}
		}
		catch (const std::exception& e)
		{
			wxMessageBox("Unable to export all room metadata:\n" + wxString::FromUTF8(e.what()),
				"Export Error", wxOK | wxICON_ERROR, this);
		}
	}
}

void RoomViewerFrame::OnImportRoomMetadata()
{
	const auto room = m_g->GetRoomData()->GetRoom(m_roomnum);
	const auto old_display_name = room->GetDisplayName();
	wxFileDialog fd(this, _("Import Room Metadata"), "", room->name + ".yaml",
		"YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	try
	{
		if (m_gpuview)
		{
			m_gpuview->CommitPendingEdits();
		}
		if (!ImportRoomMetadata(fd.GetPath().ToStdString(), m_roomnum))
		{
			wxMessageBox("Unable to read room metadata.", "Import Error", wxOK | wxICON_ERROR, this);
			return;
		}

		const auto new_display_name = m_g->GetRoomData()->GetRoom(m_roomnum)->GetDisplayName();
		if (new_display_name != old_display_name)
		{
			FireRenameNavItemEvent(new_display_name, old_display_name);
		}
		if (m_gpuview)
		{
			m_gpuview->ReloadCurrentRoomFromGameData();
		}
		m_reset_props = true;
		UpdateFrame();
	}
	catch (const std::exception& e)
	{
		wxMessageBox("Unable to import room metadata:\n" + wxString::FromUTF8(e.what()),
			"Import Error", wxOK | wxICON_ERROR, this);
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
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	if (!ImportBin(fd.GetPath().ToStdString()))
	{
		wxMessageBox("Unable to read map data from the selected file.", "Import Error", wxOK | wxICON_ERROR, this);
	}
	UpdateFrame();
}

void RoomViewerFrame::OnImportCsv()
{
	std::array<std::string, 3> filenames;
	const std::array<wxString, 3> titles = { _("Import Background Layer"), _("Import Foreground Layer"), _("Import Heightmap Data") };
	for (std::size_t i = 0; i < filenames.size(); ++i)
	{
		wxFileDialog fd(this, titles[i], "", "", "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fd.ShowModal() == wxID_CANCEL)
		{
			return;
		}
		filenames[i] = fd.GetPath().ToStdString();
	}
	if (!ImportCsv(filenames))
	{
		wxMessageBox("Unable to read map data from the selected files.", "Import Error", wxOK | wxICON_ERROR, this);
	}
	UpdateFrame();
}

void RoomViewerFrame::OnImportTmx()
{
	const auto room = m_g->GetRoomData()->GetRoom(m_roomnum);
	const auto old_display_name = room->GetDisplayName();
	wxFileDialog fd(this, _("Import Room From Tiled TMX"), "", room->name + ".tmx",
		"Tiled TMX Tilemap (*.tmx)|*.tmx|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	try
	{
		if (m_gpuview)
		{
			m_gpuview->CommitPendingEdits();
		}
		if (!ImportTmx(fd.GetPath().ToStdString(), m_roomnum))
		{
			wxMessageBox("Unable to read room TMX.", "Import Error", wxOK | wxICON_ERROR, this);
			return;
		}

		const auto new_display_name = m_g->GetRoomData()->GetRoom(m_roomnum)->GetDisplayName();
		if (new_display_name != old_display_name)
		{
			FireRenameNavItemEvent(new_display_name, old_display_name);
		}
		if (m_gpuview)
		{
			m_gpuview->ReloadCurrentRoomFromGameData();
		}
		m_reset_props = true;
		UpdateFrame();
	}
	catch (const std::exception& e)
	{
		wxMessageBox("Unable to import room TMX:\n" + wxString::FromUTF8(e.what()),
			"Import Error", wxOK | wxICON_ERROR, this);
	}
}

void RoomViewerFrame::OnImportAllTmx()
{
	wxDirDialog dd(this, "Select TMX Input Directory");
	if (dd.ShowModal() != wxID_CANCEL)
	{
		ImportAllTmx(dd.GetPath().ToStdString());
	}
}

void RoomViewerFrame::ShowMapManagerDialog()
{
	if (!m_g)
	{
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->CommitPendingEdits();
	}
	MapManagerDialog dlg(this, m_g, m_roomnum);
	dlg.ShowModal();
	ApplyManagerDialogResult(dlg.HasChanges(), dlg.GetRoomToOpen());
}

void RoomViewerFrame::ShowTilesetManagerDialog()
{
	if (!m_g)
	{
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->CommitPendingEdits();
	}
	// Open on the tileset the current room draws with, so the dialog lands somewhere
	// relevant rather than on slot zero.
	const auto tileset = m_roomnum < m_g->GetRoomData()->GetRoomCount()
		? m_g->GetRoomData()->GetTilesetForRoom(m_roomnum) : nullptr;
	TilesetManagerDialog dlg(this, m_g, tileset ? tileset->GetName() : std::string());
	dlg.ShowModal();

	const auto to_open = dlg.GetTilesetToOpen();
	if (!dlg.HasChanges() && to_open.empty())
	{
		return;
	}
	std::wstring path;
	if (!to_open.empty())
	{
		const std::wstring name(to_open.cbegin(), to_open.cend());
		if (dlg.IsTilesetToOpenAnimated())
		{
			// Animations are nested under the tileset they belong to.
			const auto anim = m_g->GetRoomData()->GetAnimatedTileset(to_open);
			const auto parent = anim
				? m_g->GetRoomData()->GetTileset(static_cast<uint8_t>(anim->GetIndex().first))
				: nullptr;
			if (parent)
			{
				const auto parent_name = parent->GetName();
				path = L"Tilesets/" + std::wstring(parent_name.cbegin(), parent_name.cend()) + L"/" + name;
			}
		}
		else
		{
			path = L"Tilesets/" + name;
		}
	}

	if (!dlg.HasChanges())
	{
		// Nothing moved, so the tree is still correct - just follow the double-click.
		wxCommandEvent evt(EVT_GO_TO_NAV_ITEM);
		evt.SetString(wxString(path));
		evt.SetInt(m_roomnum);
		evt.SetClientData(this);
		wxPostEvent(this, evt);
		return;
	}

	// Adding, deleting, renaming or moving a tileset changes the name and number of entries
	// under both Tilesets and Blocksets, which the editor cannot patch one at a time, so the
	// tree is rebuilt from the game data.
	if (m_gpuview)
	{
		m_gpuview->ReloadCurrentRoomFromGameData();
	}
	wxCommandEvent evt(EVT_REBUILD_NAV_TREE);
	evt.SetInt(m_roomnum);
	evt.SetString(wxString(path));
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void RoomViewerFrame::ShowBlocksetManagerDialog()
{
	if (!m_g)
	{
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->CommitPendingEdits();
	}
	// Open on the alternate the current room selects, so the dialog lands on what is being
	// drawn rather than on the first tileset's base.
	std::string select;
	if (m_roomnum < m_g->GetRoomData()->GetRoomCount())
	{
		const auto room = m_g->GetRoomData()->GetRoom(m_roomnum);
		const auto entry = m_g->GetRoomData()->GetBlockset(room->tileset, room->pri_blockset,
			static_cast<uint8_t>(room->sec_blockset + 1));
		if (entry)
		{
			select = entry->GetName();
		}
	}

	BlocksetManagerDialog dlg(this, m_g, select);
	dlg.ShowModal();
	if (!dlg.HasChanges())
	{
		return;
	}
	// Adding, deleting, renaming or moving a blockset changes the entries under Blocksets,
	// which the editor cannot patch one at a time, so the tree is rebuilt from the game data.
	if (m_gpuview)
	{
		m_gpuview->ReloadCurrentRoomFromGameData();
	}
	wxCommandEvent evt(EVT_REBUILD_NAV_TREE);
	evt.SetInt(m_roomnum);
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void RoomViewerFrame::ShowRoomManagerDialog()
{
	if (!m_g)
	{
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->CommitPendingEdits();
	}
	RoomManagerDialog dlg(this, m_g, m_roomnum);
	dlg.ShowModal();
	if (!dlg.HasChanges())
	{
		// The room list is untouched, so the navigation tree is still correct; there may
		// still be a double-clicked room to open.
		ApplyManagerDialogResult(false, dlg.GetRoomToOpen());
		return;
	}

	// Adding, deleting, renaming or moving a room changes the name and number of tree
	// entries the editor cannot patch one at a time - deleting room 5 renumbers every
	// room above it - so the tree is rebuilt from the game data instead. Pick the room to
	// land on first: the one the user double-clicked, else the one already open, clamped
	// in case it was the room that just went.
	const auto room_count = static_cast<int>(m_g->GetRoomData()->GetRoomCount());
	int room = dlg.GetRoomToOpen();
	if (room < 0)
	{
		room = m_roomnum;
	}
	room = room_count == 0 ? -1 : std::min(room, room_count - 1);

	wxCommandEvent evt(EVT_REBUILD_NAV_TREE);
	evt.SetInt(room);
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void RoomViewerFrame::ApplyManagerDialogResult(bool changed, int room_to_open)
{
	if (changed)
	{
		if (m_gpuview)
		{
			m_gpuview->ReloadCurrentRoomFromGameData();
		}
		m_reset_props = true;
		UpdateFrame();
	}

	if (room_to_open >= 0 && room_to_open < static_cast<int>(m_g->GetRoomData()->GetRoomCount()))
	{
		SetRoomNum(static_cast<uint16_t>(room_to_open));
		// Keep the navigation tree's selection in step with the room we just opened.
		wxCommandEvent evt(EVT_GO_TO_NAV_ITEM);
		evt.SetString(wxString(L"Rooms/") + m_g->GetRoomData()->GetRoom(room_to_open)->GetDisplayName());
		evt.SetInt(room_to_open);
		evt.SetClientData(this);
		wxPostEvent(this, evt);
	}
}

bool RoomViewerFrame::IsGpuViewSelected() const
{
	return m_gpuview != nullptr;
}

void RoomViewerFrame::SetGpuEditorMode(RoomEdit::Mode mode, bool select_gpu_page)
{
	(void)select_gpu_page;
	if (m_gpuview == nullptr)
	{
		SetMode(mode);
		return;
	}

	m_gpuview->SetEditorMode(ToGpuEditorMode(mode));
	m_gpuview->SetFocus();
	m_mode = mode;
	m_swapctrl->SetMode(m_mode);
	m_layerctrl->EnableLayers(m_mode == RoomEdit::Mode::NORMAL);
	UpdateUI();
	FireEvent(EVT_STATUSBAR_UPDATE);
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void RoomViewerFrame::SyncFrameModeFromGpuView()
{
	if (m_gpuview == nullptr)
	{
		return;
	}

	m_mode = ToRoomEditMode(m_gpuview->GetEditorMode());
	m_gpuview->SetFocus();
	m_swapctrl->SetMode(m_mode);
	m_layerctrl->EnableLayers(m_mode == RoomEdit::Mode::NORMAL);
	UpdateUI();
	FireEvent(EVT_STATUSBAR_UPDATE);
	FireEvent(EVT_PROPERTIES_UPDATE);
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

	EnableMenuItem(ID_EDIT_MAPS, m_g != nullptr);
	EnableMenuItem(ID_EDIT_TILESETS, m_g != nullptr);
	EnableMenuItem(ID_EDIT_BLOCKSETS, m_g != nullptr);

	EnableMenuItem(ID_TOOLS_LAYERS, true);
	EnableToolbarItem("Main", TOOL_UNDO, m_gpuview != nullptr && m_gpuview->CanUndo());
	EnableToolbarItem("Main", TOOL_REDO, m_gpuview != nullptr && m_gpuview->CanRedo());
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

	const bool gpu_selected = IsGpuViewSelected();
	const RoomEdit::Mode active_mode = gpu_selected ? ToRoomEditMode(m_gpuview->GetEditorMode()) : m_mode;
	const bool drawing_tools_enabled = gpu_selected && active_mode != RoomEdit::Mode::NORMAL;
	CheckMenuItem(ID_VIEW_ROOM, active_mode == RoomEdit::Mode::NORMAL);
	CheckToolbarItem("Main", MODE_ROOM, active_mode == RoomEdit::Mode::NORMAL);
	CheckMenuItem(ID_VIEW_HEIGHTMAP, active_mode == RoomEdit::Mode::HEIGHTMAP);
	CheckToolbarItem("Main", MODE_HEIGHTMAP, active_mode == RoomEdit::Mode::HEIGHTMAP);
	CheckMenuItem(ID_VIEW_BACKGROUND, active_mode == RoomEdit::Mode::BACKGROUND);
	CheckToolbarItem("Main", MODE_BACKGROUND, active_mode == RoomEdit::Mode::BACKGROUND);
	CheckMenuItem(ID_VIEW_FOREGROUND, active_mode == RoomEdit::Mode::FOREGROUND);
	CheckToolbarItem("Main", MODE_FOREGROUND, active_mode == RoomEdit::Mode::FOREGROUND);
	CheckToolbarItem("Drawing Tools", TOOL_SELECT, m_gpuview == nullptr || m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::Select);
	CheckToolbarItem("Drawing Tools", TOOL_DRAW, m_gpuview != nullptr && m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::Draw);
	CheckToolbarItem("Drawing Tools", TOOL_LINE, m_gpuview != nullptr && m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::Line);
	CheckToolbarItem("Drawing Tools", TOOL_FILLED_RECT, m_gpuview != nullptr && m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::FilledRect);
	CheckToolbarItem("Drawing Tools", TOOL_OUTLINE_RECT, m_gpuview != nullptr && m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::OutlineRect);
	CheckToolbarItem("Drawing Tools", TOOL_FILLED_CIRCLE, m_gpuview != nullptr && m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::FilledCircle);
	CheckToolbarItem("Drawing Tools", TOOL_OUTLINE_CIRCLE, m_gpuview != nullptr && m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::OutlineCircle);
	CheckToolbarItem("Drawing Tools", TOOL_FLOODFILL, m_gpuview != nullptr && m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::FloodFill);
	CheckToolbarItem("Drawing Tools", TOOL_STAMP, m_gpuview != nullptr && m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::Stamp);
	CheckToolbarItem("Drawing Tools", TOOL_CLEAR, m_gpuview != nullptr && m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::Clear);
	EnableToolbarItem("Drawing Tools", TOOL_SELECT, drawing_tools_enabled);
	EnableToolbarItem("Drawing Tools", TOOL_DRAW, drawing_tools_enabled);
	EnableToolbarItem("Drawing Tools", TOOL_LINE, drawing_tools_enabled);
	EnableToolbarItem("Drawing Tools", TOOL_FILLED_RECT, drawing_tools_enabled);
	EnableToolbarItem("Drawing Tools", TOOL_OUTLINE_RECT, drawing_tools_enabled);
	EnableToolbarItem("Drawing Tools", TOOL_FILLED_CIRCLE, drawing_tools_enabled);
	EnableToolbarItem("Drawing Tools", TOOL_OUTLINE_CIRCLE, drawing_tools_enabled);
	EnableToolbarItem("Drawing Tools", TOOL_FLOODFILL, drawing_tools_enabled);
	EnableToolbarItem("Drawing Tools", TOOL_STAMP, drawing_tools_enabled);
	EnableToolbarItem("Drawing Tools", TOOL_CLEAR, drawing_tools_enabled);

	if (m_mode == RoomEdit::Mode::NORMAL)
	{
		const bool alpha = m_gpuview != nullptr && m_gpuview->GetAlpha();
		const bool entities_visible = m_gpuview == nullptr || m_gpuview->GetEntitiesVisible();
		const bool hitboxes_visible = m_gpuview == nullptr || m_gpuview->GetEntitiesHitboxVisible();
		const bool warps_visible = m_gpuview == nullptr || m_gpuview->GetWarpsVisible();
		const bool swaps_visible = m_gpuview == nullptr || m_gpuview->GetTileSwapsVisible();

		CheckMenuItem(ID_VIEW_ALPHA, alpha);
		CheckToolbarItem("Main", TOOL_TOGGLE_ALPHA, alpha);
		EnableMenuItem(ID_VIEW_ALPHA, true);
		EnableToolbarItem("Main", TOOL_TOGGLE_ALPHA, true);

		EnableMenuItem(ID_VIEW_ENTITIES, true);
		CheckMenuItem(ID_VIEW_ENTITIES, entities_visible);
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITIES, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITIES, entities_visible);

		EnableMenuItem(ID_VIEW_ENTITY_HITBOX, true);
		CheckMenuItem(ID_VIEW_ENTITY_HITBOX, hitboxes_visible);
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, hitboxes_visible);

		EnableMenuItem(ID_VIEW_WARPS, true);
		CheckMenuItem(ID_VIEW_WARPS, warps_visible);
		EnableToolbarItem("Main", TOOL_TOGGLE_WARPS, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_WARPS, warps_visible);

		EnableMenuItem(ID_VIEW_SWAPS, true);
		CheckMenuItem(ID_VIEW_SWAPS, swaps_visible);
		EnableToolbarItem("Main", TOOL_TOGGLE_SWAPS, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_SWAPS, swaps_visible);
		
		bool object_selected = m_gpuview != nullptr && m_gpuview->HasObjectSelection();
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
		EnableToolbarItem("Tilemap", TM_TOGGLE_PRIORITY_HIGHLIGHT, false);
		CheckToolbarItem("Tilemap", TM_TOGGLE_PRIORITY_HIGHLIGHT, m_gpuview != nullptr && m_gpuview->GetLayerPriorityHighlight());
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

		const bool has_hm_edit_target = gpu_selected && m_gpuview->HasHeightmapEditTarget();
		EnableToolbarItem("Heightmap", HM_INSERT_ROW_BEFORE, gpu_selected && m_gpuview->CanInsertSelectedHeightmapRow());
		EnableToolbarItem("Heightmap", HM_INSERT_ROW_AFTER, gpu_selected && m_gpuview->CanInsertSelectedHeightmapRow());
		EnableToolbarItem("Heightmap", HM_DELETE_ROW, gpu_selected && m_gpuview->CanDeleteSelectedHeightmapRow());
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_BEFORE, gpu_selected && m_gpuview->CanInsertSelectedHeightmapColumn());
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_AFTER, gpu_selected && m_gpuview->CanInsertSelectedHeightmapColumn());
		EnableToolbarItem("Heightmap", HM_DELETE_COLUMN, gpu_selected && m_gpuview->CanDeleteSelectedHeightmapColumn());
		EnableToolbarItem("Heightmap", HM_TOGGLE_PLAYER, has_hm_edit_target);
		EnableToolbarItem("Heightmap", HM_TOGGLE_NPC, has_hm_edit_target);
		EnableToolbarItem("Heightmap", HM_TOGGLE_RAFT, has_hm_edit_target);
		EnableToolbarItem("Heightmap", HM_INCREASE_HEIGHT, gpu_selected && m_gpuview->CanIncreaseSelectedHeightmapHeight());
		EnableToolbarItem("Heightmap", HM_DECREASE_HEIGHT, gpu_selected && m_gpuview->CanDecreaseSelectedHeightmapHeight());
		EnableToolbarItem("Heightmap", HM_NUDGE_HM_NE, gpu_selected && m_gpuview->CanNudgeHeightmap(0, 1));
		EnableToolbarItem("Heightmap", HM_NUDGE_HM_NW, gpu_selected && m_gpuview->CanNudgeHeightmap(1, 0));
		EnableToolbarItem("Heightmap", HM_NUDGE_HM_SE, gpu_selected && m_gpuview->CanNudgeHeightmap(-1, 0));
		EnableToolbarItem("Heightmap", HM_NUDGE_HM_SW, gpu_selected && m_gpuview->CanNudgeHeightmap(0, -1));

		CheckToolbarItem("Heightmap", HM_TOGGLE_PLAYER, has_hm_edit_target && m_gpuview->IsSelectedHeightmapPlayerPassable());
		CheckToolbarItem("Heightmap", HM_TOGGLE_NPC, has_hm_edit_target && !m_gpuview->IsSelectedHeightmapNpcPassable());
		CheckToolbarItem("Heightmap", HM_TOGGLE_RAFT, has_hm_edit_target && m_gpuview->IsSelectedHeightmapRaftTrack());
		if (hmcell != nullptr && hmzoom != nullptr)
		{
			hmzoom->Enable(gpu_selected);
			hmzoom->SetValue(gpu_selected ? std::clamp<int>(static_cast<int>(std::round(m_gpuview->GetHeightmapZScale() * 4.0f)), 0, 4) : 0);
			hmcell->Enable(has_hm_edit_target);
			hmcell->SetSelection(has_hm_edit_target ? std::min<int>(m_gpuview->GetSelectedHeightmapType(), 0x30) : 0);
		}
		EnableToolbarItem("Tilemap", TM_CLEAR, false);
		EnableToolbarItem("Tilemap", TM_DELETE_COLUMN, false);
		EnableToolbarItem("Tilemap", TM_DELETE_ROW, false);
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_AFTER, false);
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_BEFORE, false);
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_AFTER, false);
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_BEFORE, false);
		EnableToolbarItem("Tilemap", TM_TOGGLE_PRIORITY_HIGHLIGHT, false);
		CheckToolbarItem("Tilemap", TM_TOGGLE_PRIORITY_HIGHLIGHT, m_gpuview != nullptr && m_gpuview->GetLayerPriorityHighlight());
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
		const bool has_layer_selection = gpu_selected && m_gpuview->HasSelectedLayerCell();
		EnableToolbarItem("Tilemap", TM_DELETE_COLUMN, gpu_selected && m_gpuview->CanDeleteSelectedTilemapColumn());
		EnableToolbarItem("Tilemap", TM_DELETE_ROW, gpu_selected && m_gpuview->CanDeleteSelectedTilemapRow());
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_AFTER, has_layer_selection);
		EnableToolbarItem("Tilemap", TM_INSERT_COLUMN_BEFORE, has_layer_selection);
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_AFTER, has_layer_selection);
		EnableToolbarItem("Tilemap", TM_INSERT_ROW_BEFORE, has_layer_selection);
		EnableToolbarItem("Tilemap", TM_TOGGLE_PRIORITY_HIGHLIGHT, gpu_selected);
		CheckToolbarItem("Tilemap", TM_TOGGLE_PRIORITY_HIGHLIGHT, gpu_selected && m_gpuview->GetLayerPriorityHighlight());
	}
}

void RoomViewerFrame::OnKeyDown(wxKeyEvent& evt)
{
	if (IsGpuViewSelected())
	{
		evt.Skip(!m_gpuview->HandleKeyDown(evt));
		return;
	}
	evt.Skip(!HandleKeyDown(evt.GetKeyCode(), evt.GetModifiers()));
}

void RoomViewerFrame::SyncGpuViewControls()
{
	SyncGpuViewLayerControls();
}

void RoomViewerFrame::SyncGpuViewLayerControls()
{
	if (!m_gpuview || !m_layerctrl)
	{
		return;
	}

	const uint8_t bg_opacity = m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::BG);
	const uint8_t fg_opacity = m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::FG);
	const uint8_t sprite_opacity = m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::SPRITES);
	const uint8_t heightmap_opacity = m_layerctrl->GetLayerOpacity(LayerControlFrame::Layer::HM);

	m_gpuview->SetZoom(m_layerctrl->GetZoom());
	m_gpuview->SetBackgroundOpacity(LayerOpacityToFloat(bg_opacity));
	m_gpuview->SetForegroundOpacity(LayerOpacityToFloat(fg_opacity));
	m_gpuview->SetSpriteOpacity(LayerOpacityToFloat(sprite_opacity));
	m_gpuview->SetHeightmapVisible(heightmap_opacity > 0);
}

void RoomViewerFrame::OnZoomChange(wxCommandEvent& /*evt*/)
{
	if (m_gpuview)
	{
		m_gpuview->SetZoom(m_layerctrl->GetZoom());
	}
}

void RoomViewerFrame::OnOpacityChange(wxCommandEvent& evt)
{
	LayerControlFrame::Layer layer = static_cast<LayerControlFrame::Layer>(reinterpret_cast<intptr_t>(evt.GetClientData()));
	const uint8_t opacity = m_layerctrl->GetLayerOpacity(layer);
	switch (layer)
	{
	case LayerControlFrame::Layer::BG:
		if (m_gpuview)
		{
			m_gpuview->SetBackgroundOpacity(LayerOpacityToFloat(opacity));
		}
		break;
	case LayerControlFrame::Layer::FG:
		if (m_gpuview)
		{
			m_gpuview->SetForegroundOpacity(LayerOpacityToFloat(opacity));
		}
		break;
	case LayerControlFrame::Layer::SPRITES:
		if (m_gpuview)
		{
			m_gpuview->SetSpriteOpacity(LayerOpacityToFloat(opacity));
		}
		break;
	case LayerControlFrame::Layer::HM:
		if (m_gpuview)
		{
			m_gpuview->SetHeightmapVisible(opacity > 0);
		}
		break;
	}
}

void RoomViewerFrame::OnGpuLayerOpacityChange(wxCommandEvent& /*evt*/)
{
	if (!m_gpuview || !m_layerctrl)
	{
		return;
	}

	m_layerctrl->SetLayerOpacity(LayerControlFrame::Layer::BG, m_gpuview->GetBackgroundOpacityByte());
	m_layerctrl->SetLayerOpacity(LayerControlFrame::Layer::FG, m_gpuview->GetForegroundOpacityByte());
	m_layerctrl->SetLayerOpacity(LayerControlFrame::Layer::SPRITES, m_gpuview->GetSpriteOpacityByte());
}

void RoomViewerFrame::OnGpuLayerBlockSelect(wxCommandEvent& evt)
{
	if (evt.GetClientData() == m_gpuview && m_blkctrl != nullptr)
	{
		m_blkctrl->SetBlockSelection(evt.GetInt());
	}
	evt.Skip();
}

void RoomViewerFrame::OnGpuHeightmapTargetChange(wxCommandEvent& evt)
{
	if (evt.GetClientData() == m_gpuview)
	{
		UpdateUI();
	}
	evt.Skip();
}

std::vector<Entity> RoomViewerFrame::GetRoomEntities() const
{
	if (!m_g)
	{
		return {};
	}
	return m_g->GetSpriteData()->GetRoomEntities(m_roomnum);
}

std::vector<WarpList::Warp> RoomViewerFrame::GetRoomWarps() const
{
	if (!m_g)
	{
		return {};
	}
	return m_g->GetRoomData()->GetWarpsForRoom(m_roomnum);
}

void RoomViewerFrame::UpdateEntityProperties(int entity)
{
	if (!m_g)
	{
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->CommitPendingEdits();
	}
	auto entities = GetRoomEntities();
	if (entity <= 0 || entity > static_cast<int>(entities.size()))
	{
		return;
	}

	// The dialog is expensive to construct (thousands of combo entries), so keep one cached
	// instance alive and repoint it at the entity being edited on each open.
	if (m_entity_dialog == nullptr)
	{
		m_entity_dialog = new EntityPropertiesWindow(this, m_g);
	}
	m_entity_dialog->SetEntity(entity, m_roomnum, entities);
	if (m_entity_dialog->ShowModal() == wxID_OK)
	{
		if (m_gpuview)
		{
			m_gpuview->CaptureObjectUndoState();
		}
		m_g->GetSpriteData()->SetRoomEntities(m_roomnum, entities);
		if (m_gpuview)
		{
			m_gpuview->ReloadCurrentRoomFromGameData();
			m_gpuview->SelectEntityByIndex(entity);
		}
		FireEvent(EVT_ENTITY_UPDATE, entity);
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
}

void RoomViewerFrame::UpdateWarpProperties(int warp)
{
	if (!m_g)
	{
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->CommitPendingEdits();
	}
	auto warps = GetRoomWarps();
	if (warp <= 0 || warp > static_cast<int>(warps.size()))
	{
		return;
	}

	WarpPropertyWindow dlg(this, m_roomnum, warp, &warps[warp - 1], *m_g);
	if (dlg.ShowModal() == wxID_OK)
	{
		if (Landstalker::WarpList::HasDuplicateWarps(warps))
		{
			wxMessageBox("That connection already exists. Warp direction does not create a distinct warp.",
				"Duplicate Warp", wxOK | wxICON_ERROR, this);
			return;
		}
		if (m_gpuview)
		{
			m_gpuview->CaptureObjectUndoState();
		}
		m_g->GetRoomData()->SetWarpsForRoom(m_roomnum, warps);
		if (m_gpuview)
		{
			m_gpuview->ReloadCurrentRoomFromGameData();
			m_gpuview->SelectWarpByIndex(warp);
		}
		FireEvent(EVT_WARP_UPDATE, warp);
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
}

void RoomViewerFrame::RefreshObjectLists()
{
	if (!m_g)
	{
		m_entityctrl->ResetEntities();
		m_warpctrl->ResetWarps();
		m_swapctrl->ResetSwaps();
		UpdateUI();
		return;
	}

	m_entityctrl->SetEntities(GetRoomEntities());
	m_warpctrl->SetWarps(GetRoomWarps());
	TileSwapRefresh();

	if (m_gpuview)
	{
		m_entityctrl->SetSelected(m_gpuview->GetSelectedEntityIndex());
		m_warpctrl->SetSelected(m_gpuview->GetSelectedWarpIndex());
		m_swapctrl->SetSelected(std::max(m_gpuview->GetSelectedTileSwapIndex(), m_gpuview->GetSelectedDoorIndex()));
	}
	else
	{
		m_entityctrl->SetSelected(-1);
		m_warpctrl->SetSelected(-1);
		m_swapctrl->SetSelected(-1);
	}
	UpdateUI();
}

void RoomViewerFrame::OnEntityUpdate(wxCommandEvent& evt)
{
	m_entityctrl->SetEntities(GetRoomEntities());
	if (evt.GetClientData() == m_gpuview)
	{
		m_entityctrl->SetSelected(evt.GetInt());
		m_warpctrl->SetSelected(-1);
	}
	else if (m_gpuview)
	{
		m_entityctrl->SetSelected(m_gpuview->GetSelectedEntityIndex());
		m_warpctrl->SetSelected(m_gpuview->GetSelectedWarpIndex());
	}
	UpdateUI();
}

void RoomViewerFrame::OnEntitySelect(wxCommandEvent& evt)
{
	if (evt.GetClientData() == m_gpuview)
	{
		m_entityctrl->SetSelected(evt.GetInt());
		m_warpctrl->SetSelected(-1);
		m_swapctrl->SetSelected(-1);
		UpdateUI();
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->SelectEntityByIndex(m_entityctrl->GetSelected());
		m_warpctrl->SetSelected(m_gpuview->GetSelectedWarpIndex());
	}
	UpdateUI();
}

void RoomViewerFrame::OnEntityOpenProperties(wxCommandEvent& evt)
{
	if (evt.GetClientData() == m_gpuview)
	{
		m_warpctrl->SetSelected(-1);
		m_entityctrl->SetSelected(evt.GetInt());
		UpdateEntityProperties(evt.GetInt());
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->SelectEntityByIndex(m_entityctrl->GetSelected());
		m_warpctrl->SetSelected(m_gpuview->GetSelectedWarpIndex());
	}
	UpdateEntityProperties(m_entityctrl->GetSelected());
}

void RoomViewerFrame::OnEntityAdd(wxCommandEvent& /*evt*/)
{
	if (!m_gpuview)
	{
		return;
	}
	m_warpctrl->SetSelected(m_gpuview->GetSelectedWarpIndex());
	m_gpuview->AddEntity();
}

void RoomViewerFrame::OnEntityDelete(wxCommandEvent& /*evt*/)
{
	if (!m_gpuview)
	{
		return;
	}
	m_gpuview->SelectEntityByIndex(m_entityctrl->GetSelected());
	m_gpuview->DeleteSelectedObject();
}

void RoomViewerFrame::OnEntityMoveUp(wxCommandEvent& /*evt*/)
{
	if (!m_gpuview)
	{
		return;
	}
	m_gpuview->SelectEntityByIndex(m_entityctrl->GetSelected());
	m_gpuview->ReorderSelectedObject(-1);
}

void RoomViewerFrame::OnEntityMoveDown(wxCommandEvent& /*evt*/ )
{
	if (!m_gpuview)
	{
		return;
	}
	m_gpuview->SelectEntityByIndex(m_entityctrl->GetSelected());
	m_gpuview->ReorderSelectedObject(1);
}

void RoomViewerFrame::OnWarpUpdate(wxCommandEvent& evt)
{
	m_warpctrl->SetWarps(GetRoomWarps());
	if (evt.GetClientData() == m_gpuview)
	{
		m_warpctrl->SetSelected(evt.GetInt());
		m_entityctrl->SetSelected(-1);
	}
	else if (m_gpuview)
	{
		m_warpctrl->SetSelected(m_gpuview->GetSelectedWarpIndex());
		m_entityctrl->SetSelected(m_gpuview->GetSelectedEntityIndex());
	}
	UpdateUI();
}

void RoomViewerFrame::OnWarpSelect(wxCommandEvent& evt)
{
	if (evt.GetClientData() == m_gpuview)
	{
		m_warpctrl->SetSelected(evt.GetInt());
		m_entityctrl->SetSelected(-1);
		m_swapctrl->SetSelected(-1);
		UpdateUI();
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->SelectWarpByIndex(m_warpctrl->GetSelected());
		m_entityctrl->SetSelected(m_gpuview->GetSelectedEntityIndex());
	}
	UpdateUI();
}

void RoomViewerFrame::OnWarpOpenProperties(wxCommandEvent& evt)
{
	if (evt.GetClientData() == m_gpuview)
	{
		m_entityctrl->SetSelected(-1);
		m_warpctrl->SetSelected(evt.GetInt());
		UpdateWarpProperties(evt.GetInt());
		UpdateUI();
		return;
	}
	if (m_gpuview)
	{
		m_gpuview->SelectWarpByIndex(m_warpctrl->GetSelected());
	}
	UpdateWarpProperties(m_warpctrl->GetSelected());
	UpdateUI();
}

void RoomViewerFrame::OnWarpAdd(wxCommandEvent& /*evt*/)
{
	if (!m_gpuview)
	{
		return;
	}
	m_gpuview->AddWarpHalf();
	UpdateUI();
}

void RoomViewerFrame::OnWarpDelete(wxCommandEvent& /*evt*/)
{
	if (!m_gpuview)
	{
		return;
	}
	m_gpuview->SelectWarpByIndex(m_warpctrl->GetSelected());
	m_gpuview->DeleteSelectedObject();
	UpdateUI();
}

void RoomViewerFrame::TileSwapRefresh()
{
	if (m_g != nullptr)
	{
		m_swapctrl->SetSwaps(m_g->GetRoomData()->GetTileSwaps(m_roomnum), m_g->GetRoomData()->GetDoors(m_roomnum), m_roomnum);
		UpdateUI();
		return;
	}
	m_swapctrl->ResetSwaps();
	UpdateUI();
}

void RoomViewerFrame::OnSwapUpdate(wxCommandEvent& evt)
{
	//OnSwapSelect(evt);
	TileSwapRefresh();
	if (m_gpuview && evt.GetClientData() != m_gpuview)
	{
		m_gpuview->ReloadCurrentRoomFromGameData();
	}
	UpdateUI();
}

void RoomViewerFrame::OnSwapSelect(wxCommandEvent& evt)
{
	const bool gpu_selection = evt.GetClientData() == m_gpuview;
	if (evt.GetInt() > 0 && m_swapctrl->GetPage() != TileSwapControlFrame::ID::TILESWAP)
	{
		m_swapctrl->SetPage(TileSwapControlFrame::ID::TILESWAP);
	}
	if (m_swapctrl->GetSelected() != evt.GetInt() ||
		(m_gpuview && m_gpuview->GetSelectedTileSwapIndex() != evt.GetInt()))
	{
		m_swapctrl->SetSelected(evt.GetInt());
		if (m_gpuview && !gpu_selection)
		{
			m_gpuview->SelectTileSwapByIndex(evt.GetInt());
		}
		if (gpu_selection)
		{
			m_entityctrl->SetSelected(-1);
			m_warpctrl->SetSelected(-1);
		}
		UpdateUI();
	}
}

void RoomViewerFrame::OnSwapAdd(wxCommandEvent& /*evt*/)
{
	if (m_gpuview)
	{
		m_gpuview->AddTileSwap();
	}
}

void RoomViewerFrame::OnSwapDelete(wxCommandEvent& evt)
{
	if (m_gpuview)
	{
		m_gpuview->SelectTileSwapByIndex(evt.GetInt());
		m_gpuview->DeleteSelectedObject();
	}
}

void RoomViewerFrame::OnSwapMoveUp(wxCommandEvent& evt)
{
	if (m_gpuview)
	{
		m_gpuview->SelectTileSwapByIndex(evt.GetInt());
		m_gpuview->ReorderSelectedObject(-1);
	}
}

void RoomViewerFrame::OnSwapMoveDown(wxCommandEvent& evt)
{
	if (m_gpuview)
	{
		m_gpuview->SelectTileSwapByIndex(evt.GetInt());
		m_gpuview->ReorderSelectedObject(1);
	}
}

void RoomViewerFrame::OnSwapProperties(wxCommandEvent& evt)
{
	ShowTileswapDialog(true, TileSwapDialog::PageType::SWAPS, evt.GetInt());
}

void RoomViewerFrame::OnDoorUpdate(wxCommandEvent& evt)
{
	//OnDoorSelect(evt);
	TileSwapRefresh();
	if (m_gpuview && evt.GetClientData() != m_gpuview)
	{
		m_gpuview->ReloadCurrentRoomFromGameData();
	}
}

void RoomViewerFrame::OnDoorSelect(wxCommandEvent& evt)
{
	const bool gpu_selection = evt.GetClientData() == m_gpuview;
	if (evt.GetInt() > 0 && m_swapctrl->GetPage() != TileSwapControlFrame::ID::DOOR)
	{
		m_swapctrl->SetPage(TileSwapControlFrame::ID::DOOR);
	}
	if (m_swapctrl->GetSelected() != evt.GetInt() ||
		(m_gpuview && m_gpuview->GetSelectedDoorIndex() != evt.GetInt()))
	{
		m_swapctrl->SetSelected(evt.GetInt());
		if (m_gpuview && !gpu_selection)
		{
			m_gpuview->SelectDoorByIndex(evt.GetInt());
		}
		if (gpu_selection)
		{
			m_entityctrl->SetSelected(-1);
			m_warpctrl->SetSelected(-1);
		}
		UpdateUI();
	}
}

void RoomViewerFrame::OnDoorAdd(wxCommandEvent& /*evt*/)
{
	if (m_gpuview)
	{
		m_gpuview->AddDoor();
	}
}

void RoomViewerFrame::OnDoorDelete(wxCommandEvent& evt)
{
	if (m_gpuview)
	{
		m_gpuview->SelectDoorByIndex(evt.GetInt());
		m_gpuview->DeleteSelectedObject();
	}
}

void RoomViewerFrame::OnDoorMoveUp(wxCommandEvent& evt)
{
	if (m_gpuview)
	{
		m_gpuview->SelectDoorByIndex(evt.GetInt());
		m_gpuview->ReorderSelectedObject(-1);
	}
}

void RoomViewerFrame::OnDoorMoveDown(wxCommandEvent& evt)
{
	if (m_gpuview)
	{
		m_gpuview->SelectDoorByIndex(evt.GetInt());
		m_gpuview->ReorderSelectedObject(1);
	}
}

void RoomViewerFrame::OnDoorProperties(wxCommandEvent& evt)
{
	ShowTileswapDialog(true, TileSwapDialog::PageType::DOORS, evt.GetInt());
}

void RoomViewerFrame::OnHMTypeSelect(wxCommandEvent& evt)
{
	wxChoice* ctrl = static_cast<wxChoice*>(evt.GetEventObject());
	if (ctrl != nullptr && IsGpuViewSelected())
	{
		m_gpuview->SetSelectedHeightmapType(static_cast<uint8_t>(ctrl->GetSelection()));
	}
	UpdateUI();
	evt.Skip();
}

void RoomViewerFrame::OnHMZoom(wxCommandEvent& evt)
{
	wxSlider* ctrl = static_cast<wxSlider*>(evt.GetEventObject());
	if (ctrl != nullptr && IsGpuViewSelected())
	{
		m_gpuview->SetHeightmapZScale(static_cast<float>(std::clamp(ctrl->GetValue(), 0, 4)) * 0.25f);
	}
	UpdateUI();
	evt.Skip();
}

void RoomViewerFrame::OnBlockSelect(wxCommandEvent& evt)
{
	auto block = evt.GetInt();
	if (m_gpuview != nullptr)
	{
		m_gpuview->SetSelectedBlockId(block);
		// Picking a block out of the block selector says the user wants to paint with it,
		// so drop out of select mode rather than making them reach for the tool button.
		// The heightmap editor already behaves this way when a cell type is picked.
		const auto editor_mode = m_gpuview->GetEditorMode();
		if ((editor_mode == MyGLCanvas::EditorMode::BackgroundLayer ||
			 editor_mode == MyGLCanvas::EditorMode::ForegroundLayer) &&
			m_gpuview->GetDrawingTool() == MyGLCanvas::DrawingTool::Select)
		{
			m_gpuview->SetDrawingTool(MyGLCanvas::DrawingTool::Draw);
			SyncGpuViewControls();
			UpdateUI();
		}
	}
	if (m_blkctrl != nullptr)
	{
		m_blkctrl->SetBlockSelection(block);
	}
	evt.Skip();
}

void RoomViewerFrame::OnGpuEditorModeChange(wxCommandEvent& evt)
{
	if (evt.GetClientData() == m_gpuview)
	{
		SyncFrameModeFromGpuView();
	}
	evt.Skip();
}

void RoomViewerFrame::OnSize(wxSizeEvent& evt)
{
	evt.Skip();
}

void RoomViewerFrame::FireRenameNavItemEvent(const std::wstring& old_name, const std::wstring& new_name)
{
	wxCommandEvent evt(EVT_RENAME_NAV_ITEM);
	std::wstring lbl(L"Rooms/" + old_name + L"\1Rooms/" + new_name);
	evt.SetString(lbl);
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

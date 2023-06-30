#include "RoomViewerFrame.h"

#include <fstream>
#include <sstream>
#include <FlagDialog.h>
#include <ChestDialog.h>
#include <CharacterDialog.h>
#include "RoomViewerCtrl.h"

enum MENU_IDS
{
	ID_FILE_EXPORT_BIN = 20000,
	ID_FILE_EXPORT_CSV,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_BIN,
	ID_FILE_IMPORT_CSV,
	ID_EDIT,
	ID_EDIT_ENTITY_PROPERTIES,
	ID_EDIT_FLAGS,
	ID_EDIT_CHESTS,
	ID_EDIT_DIALOGUE,
	ID_TOOLS,
	ID_TOOLS_LAYERS,
	ID_TOOLS_ENTITIES,
	ID_TOOLS_WARPS,
	ID_VIEW,
	ID_VIEW_ENTITIES,
	ID_VIEW_ENTITY_HITBOX,
	ID_VIEW_WARPS,
	ID_VIEW_ERRORS
};

enum TOOL_IDS
{
	TOOL_TOGGLE_ENTITIES = 30000,
	TOOL_TOGGLE_ENTITY_HITBOX,
	TOOL_TOGGLE_WARPS,
	TOOL_SHOW_LAYERS_PANE,
	TOOL_SHOW_ENTITIES_PANE,
	TOOL_SHOW_WARPS_PANE,
	TOOL_SHOW_FLAGS,
	TOOL_SHOW_CHESTS,
	TOOL_SHOW_DIALOGUE,
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
	HM_ZOOM
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
EVT_COMMAND(wxID_ANY, EVT_HEIGHTMAP_UPDATE, RoomViewerFrame::OnHeightmapUpdate)
EVT_COMMAND(wxID_ANY, EVT_HEIGHTMAP_MOVE, RoomViewerFrame::OnHeightmapMove)
EVT_COMMAND(wxID_ANY, EVT_HEIGHTMAP_CELL_SELECTED, RoomViewerFrame::OnHeightmapSelect)
EVT_SLIDER(HM_ZOOM, RoomViewerFrame::OnHMZoom)
EVT_CHOICE(HM_TYPE_DROPDOWN, RoomViewerFrame::OnHMTypeSelect)
EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, RoomViewerFrame::OnTabChange)
EVT_SIZE(RoomViewerFrame::OnSize)
wxEND_EVENT_TABLE()

RoomViewerFrame::RoomViewerFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst),
	  m_title(""),
	  m_mode(Mode::NORMAL),
	  m_reset_props(false),
	  m_layerctrl_visible(true),
	  m_entityctrl_visible(true),
	  m_warpctrl_visible(true),
	  m_sizes_set(false),
	  m_tb_hmcell(nullptr),
	  m_tb_hmzoom(nullptr)
{
	m_mgr.SetManagedWindow(this);

	m_layerctrl = new LayerControlFrame(this);
	m_entityctrl = new EntityControlFrame(this, GetImageList());
	m_warpctrl = new WarpControlFrame(this, GetImageList());
	m_mgr.AddPane(m_layerctrl, wxAuiPaneInfo().Right().Layer(1).Resizable(false).MinSize(220, 200)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Layers"));
	m_mgr.AddPane(m_entityctrl, wxAuiPaneInfo().Right().Layer(1).Resizable(false).MinSize(220, 200)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Entities"));
	m_mgr.AddPane(m_warpctrl, wxAuiPaneInfo().Right().Layer(1).Resizable(false).MinSize(220, 200)
		.BestSize(220, 200).FloatingSize(220, 200).Caption("Warps"));
	m_nb = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TAB_SPLIT);
	m_nb->Freeze();
	m_roomview = new RoomViewerCtrl(m_nb, this);
	m_hmedit = new HeightmapEditorCtrl(m_nb, this);
	m_nb->AddPage(m_roomview, "Room", true);
	m_nb->AddPage(m_hmedit, "Heightmap");
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
	m_hmedit->Connect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
}

RoomViewerFrame::~RoomViewerFrame()
{
	this->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);

	m_roomview->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_entityctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
	m_warpctrl->Disconnect(wxEVT_CHAR, wxKeyEventHandler(RoomViewerFrame::OnKeyDown), nullptr, this);
}

void RoomViewerFrame::SetMode(RoomViewerFrame::Mode mode)
{
	m_mode = mode;
	if (mode == Mode::NORMAL)
	{
		SetPaneVisibility(m_layerctrl, m_layerctrl_visible);
		SetPaneVisibility(m_entityctrl, m_entityctrl_visible);
		SetPaneVisibility(m_warpctrl, m_warpctrl_visible);
	}
	else
	{
		m_layerctrl_visible = IsPaneVisible(m_layerctrl);
		m_entityctrl_visible = IsPaneVisible(m_entityctrl);
		m_warpctrl_visible = IsPaneVisible(m_warpctrl);
		SetPaneVisibility(m_layerctrl, false);
		SetPaneVisibility(m_entityctrl, false);
		SetPaneVisibility(m_warpctrl, false);
	}
	UpdateFrame();
	UpdateUI();
}

void RoomViewerFrame::UpdateFrame()
{
	m_layerctrl->EnableLayers(m_mode == Mode::NORMAL);
	m_roomview->SetRoomNum(m_roomnum);
	m_hmedit->SetRoomNum(m_roomnum);
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
	m_mode = Mode::NORMAL;
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
	m_mode = Mode::NORMAL;
	UpdateFrame();
}

void RoomViewerFrame::SetRoomNum(uint16_t roomnum, RoomViewerFrame::Mode mode)
{
	if (m_g != nullptr)
	{
		m_nb->SetPageText(0, m_g->GetRoomData()->GetRoom(roomnum)->name);
		m_nb->SetPageText(1, wxString("Heightmap: ") + m_g->GetRoomData()->GetRoom(roomnum)->map);
		m_g->GetRoomData()->CleanupChests(*m_g);
	}
	if (m_roomnum != roomnum)
	{
		m_reset_props = true;
	}
	m_roomnum = roomnum;
	m_mode = mode;
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
	return buf.WritePNG(path, palette, false);
}

bool RoomViewerFrame::ImportBin(const std::string& path)
{
	auto bytes = ReadBytes(path);
	auto data = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
	data->GetData()->Decode(bytes.data());
	return true;
}

bool RoomViewerFrame::ImportCsv(const std::array<std::string, 3>& paths)
{
	auto data = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
	std::ifstream bg(paths[0], std::ios::in);
	std::ifstream fg(paths[1], std::ios::in);
	std::ifstream hm(paths[2], std::ios::in);
	
	int w, h, t, l, hw, hh;
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

	for (int i = 0; i < h; ++i)
	{
		if (background[i].size() != w || foreground[i].size() != w)
		{
			return false;
		}
	}
	for (int i = 1; i <= hh; ++i)
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
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			data->SetBlock(background[y][x], i, Tilemap3D::Layer::BG);
			data->SetBlock(foreground[y][x], i++, Tilemap3D::Layer::FG);
		}
	}
	for (int y = 0; y < hh; ++y)
	{
		for (int x = 0; x < hw; ++x)
		{
			data->SetCellProps({ x, y }, (heightmap[y + 1][x] >> 12) & 0xF);
			data->SetHeight({ x, y }, (heightmap[y + 1][x] >> 8) & 0xF);
			data->SetCellType({ x, y }, heightmap[y + 1][x] & 0xFF);
		}
	}

	return true;
}

bool RoomViewerFrame::HandleKeyDown(unsigned int key, unsigned int modifiers)
{
	if (m_roomview != nullptr)
	{
		return m_roomview->HandleKeyDown(key, modifiers);
	}
	return true;
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

void RoomViewerFrame::InitStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(3);
	status.SetStatusText("", 0);
	status.SetStatusText("", 1);
	status.SetStatusText("", 1);
}

void RoomViewerFrame::UpdateStatusBar(wxStatusBar& status) const
{
	if (status.GetFieldsCount() != 3)
	{
		return;
	}
	if (m_mode == Mode::HEIGHTMAP)
	{
		status.SetStatusText(m_hmedit->GetStatusText(), 0);
	}
	else
	{
		status.SetStatusText(m_roomview->GetStatusText(), 0);
	}
	if (m_roomview->IsEntitySelected())
	{
		auto& entity = m_roomview->GetSelectedEntity();
		int idx = m_roomview->GetSelectedEntityIndex();
		status.SetStatusText(StrPrintf("Selected Entity %d (%04.1f, %04.1f, %04.1f) - %s",
			idx, entity.GetXDbl(), entity.GetYDbl(), entity.GetZDbl(), entity.GetTypeName().c_str()), 1);
	}
	else
	{
		status.SetStatusText("", 1);
	}
	if (m_roomview->GetErrorCount() > 0)
	{
		status.SetStatusText(StrPrintf("Total Errors: %d, Error #1 : %s",
			m_roomview->GetErrorCount(), m_roomview->GetErrorText(0).c_str()), 2);
	}
	else
	{
		status.SetStatusText("", 2);
	}
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
		props.Append(new wxPropertyCategory("Misc", "Misc"));
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
	m_bgms.Add("[00] The Marquis' Invitation");
	m_bgms.Add("[01] Premonition of Trouble");
	m_bgms.Add("[02] Overworld");
	m_bgms.Add("[03] Bustling Street");
	m_bgms.Add("[04] Torchlight");
	m_bgms.Add("[05] Prayers to God");
	m_bgms.Add("[06] Gumi");
	m_bgms.Add("[07] Beneath the Mysterious Tree");
	m_bgms.Add("[08] Overworld");
	m_bgms.Add("[09] The King's Chamber");
	m_bgms.Add("[0A] Mysterious Island");
	m_bgms.Add("[0B] The Death God's Invitation");
	m_bgms.Add("[0C] Deserted Street Corner");
	m_bgms.Add("[0D] Labrynth");
	m_bgms.Add("[0E] The Silence, the Darkness, and...");
	m_bgms.Add("[0F] Light of the Setting Sun");
	m_bgms.Add("[10] Friday and a Soft Breeze");
	m_bgms.Add("[11] Divine Guardian of the Maze");
	m_bgms.Add("[12] Fade Out");

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
		props.GetGrid()->SetPropertyValue("ILP", m_g->GetStringData()->GetMapPosition(m_roomnum));
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
			FireEvent(EVT_STATUSBAR_UPDATE);
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
	}
	else if (name == "M")
	{
		const auto map_name = m_maps.GetLabel(property->GetChoiceSelection());
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
	ctrl->GetGrid()->Thaw();
}

void RoomViewerFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, wxID_ANY, "", wxITEM_SEPARATOR);
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_BIN, "Export Map as Binary...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_CSV, "Export Map as CSV Set...");
	AddMenuItem(fileMenu, 3, ID_FILE_EXPORT_PNG, "Export Map as PNG...");
	AddMenuItem(fileMenu, 4, ID_FILE_IMPORT_BIN, "Import Map from Binary...");
	AddMenuItem(fileMenu, 5, ID_FILE_IMPORT_CSV, "Import Map from CSV...");

	auto& editMenu = AddMenu(menu, 1, ID_EDIT, "Edit");
	AddMenuItem(editMenu, 0, ID_EDIT_ENTITY_PROPERTIES, "Entity Properties...");
	AddMenuItem(editMenu, 1, ID_EDIT_FLAGS, "Flags...");
	AddMenuItem(editMenu, 2, ID_EDIT_CHESTS, "Chests...");
	AddMenuItem(editMenu, 3, ID_EDIT_DIALOGUE, "Dialogue...");

	auto& viewMenu = AddMenu(menu, 2, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_ENTITIES, "Show Entities", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_ENTITY_HITBOX, "Show Entity Hitboxes", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_WARPS, "Show Warps", wxITEM_CHECK);

	auto& toolsMenu = AddMenu(menu, 3, ID_TOOLS, "Tools");
	AddMenuItem(toolsMenu, 0, ID_TOOLS_LAYERS, "Layers", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 1, ID_TOOLS_ENTITIES, "Entity List", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 2, ID_TOOLS_WARPS, "Warp List", wxITEM_CHECK);

	wxAuiToolBar* main_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	main_tb->SetToolBitmapSize(wxSize(16, 16));
	main_tb->AddTool(TOOL_TOGGLE_ENTITIES, "Entities Visible", ilist.GetImage("entity"), "Entities Visible", wxITEM_CHECK);
	main_tb->AddTool(TOOL_TOGGLE_ENTITY_HITBOX, "Entity Hitboxes Visible", ilist.GetImage("ehitbox"), "Entity Hitboxes Visible", wxITEM_CHECK);
	main_tb->AddTool(TOOL_TOGGLE_WARPS, "Warps Visible", ilist.GetImage("warp"), "Warps Visible", wxITEM_CHECK);
	main_tb->AddSeparator();
	main_tb->AddTool(TOOL_SHOW_FLAGS, "Flags", ilist.GetImage("flags"), "Flag Editor");
	main_tb->AddTool(TOOL_SHOW_CHESTS, "Chests", ilist.GetImage("chest16"), "Chest Editor");
	main_tb->AddTool(TOOL_SHOW_DIALOGUE, "Dialogue", ilist.GetImage("dialogue"), "Dialogue Editor");
	main_tb->AddTool(TOOL_SHOW_SELECTION_PROPERTIES, "Selection Properties", ilist.GetImage("properties"), "Selection Properties");
	main_tb->AddSeparator();
	main_tb->AddTool(TOOL_SHOW_LAYERS_PANE, "Layers Pane", ilist.GetImage("layers"), "Layers Pane", wxITEM_CHECK);
	main_tb->AddTool(TOOL_SHOW_ENTITIES_PANE, "Entities Pane", ilist.GetImage("epanel"), "Entities Pane", wxITEM_CHECK);
	main_tb->AddTool(TOOL_SHOW_WARPS_PANE, "Warps Pane", ilist.GetImage("wpanel"), "Warps Pane", wxITEM_CHECK);
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
	m_tb_hmcell = new wxChoice(hm_tb, HM_TYPE_DROPDOWN, wxDefaultPosition, wxDefaultSize, celltypes);
	m_tb_hmcell->SetSelection(0);
	m_tb_hmzoom = new wxSlider(hm_tb, HM_ZOOM, 2, 1, 5);
	hm_tb->SetToolBitmapSize(wxSize(16, 16));
	hm_tb->AddTool(HM_INSERT_ROW_BEFORE, "Insert Row Before", ilist.GetImage("hm_insert_se"), "Insert Row Before", wxITEM_NORMAL);
	hm_tb->AddTool(HM_INSERT_ROW_AFTER, "Insert Row After", ilist.GetImage("hm_insert_nw"), "Insert Row After", wxITEM_NORMAL);
	hm_tb->AddTool(HM_DELETE_ROW, "Delete Row", ilist.GetImage("hm_delete_nesw"), "Delete Row", wxITEM_NORMAL);
	hm_tb->AddTool(HM_INSERT_COLUMN_BEFORE, "Insert Column Before", ilist.GetImage("hm_insert_sw"), "Insert Column Before", wxITEM_NORMAL);
	hm_tb->AddTool(HM_INSERT_COLUMN_AFTER, "Insert Column After", ilist.GetImage("hm_insert_ne"), "Insert Column After", wxITEM_NORMAL);
	hm_tb->AddTool(HM_DELETE_COLUMN, "Delete Column", ilist.GetImage("hm_delete_nwse"), "Delete Column", wxITEM_NORMAL);
	hm_tb->AddSeparator();
	hm_tb->AddControl(m_tb_hmcell, "Cell Type");
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
	hm_tb->AddControl(m_tb_hmzoom, "Zoom");
	AddToolbar(m_mgr, *hm_tb, "Heightmap", "Heightmap Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(2));

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
		case ID_FILE_EXPORT_PNG:
			OnExportPng();
			break;
		case ID_FILE_IMPORT_BIN:
			OnImportBin();
			break;
		case ID_FILE_IMPORT_CSV:
			OnImportCsv();
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
			m_hmedit->IncreaseSelectedHeight();
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
	const wxString default_file = rd->name + ".bin";
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
	wxString default_file = rd->name + "_background.csv";
	wxFileDialog fd(this, _("Export Background Layer as CSV"), "", default_file, "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		fnames[0] = fd.GetPath().ToStdString();
	}
	default_file = rd->name + "_foreground.csv";
	fd.Create(this, _("Export Foreground Layer as CSV"), "", default_file, "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		fnames[1] = fd.GetPath().ToStdString();
	}
	default_file = rd->name + "_heightmap.csv";
	fd.Create(this, _("Export Heightmap Layer as CSV"), "", default_file, "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		fnames[2] = fd.GetPath().ToStdString();
	}
	ExportCsv(fnames);
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


void RoomViewerFrame::UpdateUI() const
{
	if (m_mode == Mode::NORMAL)
	{
		EnableMenuItem(ID_VIEW_ENTITIES, true);
		CheckMenuItem(ID_VIEW_ENTITIES, m_roomview->GetEntitiesVisible());
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITIES, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITIES, m_roomview->GetEntitiesVisible());

		EnableMenuItem(ID_VIEW_ENTITY_HITBOX, true);
		CheckMenuItem(ID_VIEW_ENTITY_HITBOX, m_roomview->GetEntitiesHitboxVisible());
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, true);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, m_roomview->GetEntitiesHitboxVisible());

		EnableMenuItem(ID_EDIT_ENTITY_PROPERTIES, m_roomview->IsEntitySelected() || m_roomview->IsWarpSelected());
		EnableToolbarItem("Main", TOOL_SHOW_SELECTION_PROPERTIES, m_roomview->IsEntitySelected() || m_roomview->IsWarpSelected());

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

		CheckToolbarItem("hm", HM_TOGGLE_PLAYER, false);
		CheckToolbarItem("hm", HM_TOGGLE_NPC, false);
		CheckToolbarItem("hm", HM_TOGGLE_RAFT, false);

		EnableToolbarItem("hm", HM_INSERT_ROW_BEFORE, false);
		EnableToolbarItem("hm", HM_INSERT_ROW_AFTER, false);
		EnableToolbarItem("hm", HM_DELETE_ROW, false);
		EnableToolbarItem("hm", HM_INSERT_COLUMN_BEFORE, false);
		EnableToolbarItem("hm", HM_INSERT_COLUMN_AFTER, false);
		EnableToolbarItem("hm", HM_DELETE_COLUMN, false);
		EnableToolbarItem("hm", HM_TOGGLE_PLAYER, false);
		EnableToolbarItem("hm", HM_TOGGLE_NPC, false);
		EnableToolbarItem("hm", HM_TOGGLE_RAFT, false);
		EnableToolbarItem("hm", HM_INCREASE_HEIGHT, false);
		EnableToolbarItem("hm", HM_DECREASE_HEIGHT, false);
		if (m_tb_hmcell != nullptr && m_tb_hmzoom != nullptr)
		{
			m_tb_hmcell->SetSelection(0);
			m_tb_hmcell->Enable(false);
			m_tb_hmzoom->Enable(false);
		}
	}
	else
	{
		CheckMenuItem(ID_VIEW_ENTITIES, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITIES, false);
		EnableMenuItem(ID_VIEW_ENTITIES, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITIES, false);

		CheckMenuItem(ID_VIEW_ENTITY_HITBOX, false);
		CheckToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, false);
		EnableMenuItem(ID_VIEW_ENTITY_HITBOX, false);
		EnableToolbarItem("Main", TOOL_TOGGLE_ENTITY_HITBOX, false);

		CheckMenuItem(ID_VIEW_WARPS, false);
		CheckToolbarItem("Main", ID_VIEW_WARPS, false);
		EnableMenuItem(ID_VIEW_WARPS, false);
		EnableToolbarItem("Main", ID_VIEW_WARPS, false);

		EnableMenuItem(ID_EDIT_ENTITY_PROPERTIES, false);
		EnableToolbarItem("Main", ID_EDIT_ENTITY_PROPERTIES, false);

		CheckMenuItem(ID_TOOLS_LAYERS, false);
		CheckToolbarItem("Main", TOOL_SHOW_LAYERS_PANE, false);
		EnableMenuItem(ID_TOOLS_LAYERS, false);
		EnableToolbarItem("Main", TOOL_SHOW_LAYERS_PANE, false);

		CheckMenuItem(ID_TOOLS_ENTITIES, false);
		CheckToolbarItem("Main", TOOL_SHOW_ENTITIES_PANE, false);
		EnableMenuItem(ID_TOOLS_ENTITIES, false);
		EnableToolbarItem("Main", TOOL_SHOW_ENTITIES_PANE, false);

		CheckMenuItem(ID_TOOLS_WARPS, false);
		CheckToolbarItem("Main", TOOL_SHOW_WARPS_PANE, false);
		EnableMenuItem(ID_TOOLS_WARPS, false);
		EnableToolbarItem("Main", TOOL_SHOW_WARPS_PANE, false);

		EnableToolbarItem("Heightmap", HM_INSERT_ROW_BEFORE, m_hmedit->IsSelectionValid());
		EnableToolbarItem("Heightmap", HM_INSERT_ROW_AFTER, m_hmedit->IsSelectionValid());
		EnableToolbarItem("Heightmap", HM_DELETE_ROW, m_hmedit->IsSelectionValid());
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_BEFORE, m_hmedit->IsSelectionValid());
		EnableToolbarItem("Heightmap", HM_INSERT_COLUMN_AFTER, m_hmedit->IsSelectionValid());
		EnableToolbarItem("Heightmap", HM_DELETE_COLUMN, m_hmedit->IsSelectionValid());
		EnableToolbarItem("Heightmap", HM_TOGGLE_PLAYER, m_hmedit->IsSelectionValid());
		EnableToolbarItem("Heightmap", HM_TOGGLE_NPC, m_hmedit->IsSelectionValid());
		EnableToolbarItem("Heightmap", HM_TOGGLE_RAFT, m_hmedit->IsSelectionValid());
		EnableToolbarItem("Heightmap", HM_INCREASE_HEIGHT, m_hmedit->IsSelectionValid() && m_hmedit->GetSelectedHeight() < 15);
		EnableToolbarItem("Heightmap", HM_DECREASE_HEIGHT, m_hmedit->IsSelectionValid() && m_hmedit->GetSelectedHeight() > 0);
		m_tb_hmcell->Enable(m_hmedit->IsSelectionValid());
		m_tb_hmzoom->Enable(true);

		CheckToolbarItem("Heightmap", HM_TOGGLE_PLAYER, m_hmedit->IsSelectionValid() && m_hmedit->IsSelectedPlayerPassable());
		CheckToolbarItem("Heightmap", HM_TOGGLE_NPC, m_hmedit->IsSelectionValid() && !m_hmedit->IsSelectedNPCPassable());
		CheckToolbarItem("Heightmap", HM_TOGGLE_RAFT, m_hmedit->IsSelectionValid() && m_hmedit->IsSelectedRaftTrack());
		if (m_tb_hmcell != nullptr && m_tb_hmzoom != nullptr)
		{
			m_tb_hmcell->Enable(m_hmedit->IsSelectionValid());
			m_tb_hmzoom->Enable(true);
			if (m_hmedit->IsSelectionValid())
			{
				m_tb_hmcell->SetSelection(m_hmedit->GetSelectedType() > 0x2F ? 0x30 : m_hmedit->GetSelectedType());
			}
			else
			{
				m_tb_hmcell->SetSelection(0);
			}
		}
	}
}

void RoomViewerFrame::OnKeyDown(wxKeyEvent& evt)
{
	if (m_mode == Mode::NORMAL && m_roomview != nullptr)
	{
		evt.Skip(m_roomview->HandleKeyDown(evt.GetKeyCode(), evt.GetModifiers()));
		return;
	}
	else if (m_mode == Mode::HEIGHTMAP && m_hmedit != nullptr)
	{
		evt.Skip(m_hmedit->HandleKeyDown(evt.GetKeyCode(), evt.GetModifiers()));
		return;
	}
	evt.Skip();
}

void RoomViewerFrame::OnZoomChange(wxCommandEvent& evt)
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

void RoomViewerFrame::OnEntityUpdate(wxCommandEvent& evt)
{
	m_entityctrl->SetEntities(m_roomview->GetEntities());
	m_entityctrl->SetSelected(m_roomview->GetSelectedEntityIndex());
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	UpdateUI();
}

void RoomViewerFrame::OnEntitySelect(wxCommandEvent& evt)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	UpdateUI();
}

void RoomViewerFrame::OnEntityOpenProperties(wxCommandEvent& evt)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	m_roomview->UpdateEntityProperties(m_entityctrl->GetSelected());
}

void RoomViewerFrame::OnEntityAdd(wxCommandEvent& evt)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->AddEntity();
}

void RoomViewerFrame::OnEntityDelete(wxCommandEvent& evt)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	m_roomview->DeleteSelectedEntity();
}

void RoomViewerFrame::OnEntityMoveUp(wxCommandEvent& evt)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	m_roomview->MoveSelectedEntityUp();
}

void RoomViewerFrame::OnEntityMoveDown(wxCommandEvent& evt)
{
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_roomview->SelectEntity(m_entityctrl->GetSelected());
	m_roomview->MoveSelectedEntityDown();
}

void RoomViewerFrame::OnWarpUpdate(wxCommandEvent& evt)
{
	m_warpctrl->SetWarps(m_roomview->GetWarps());
	m_warpctrl->SetSelected(m_roomview->GetSelectedWarpIndex());
	m_entityctrl->SetSelected(m_roomview->GetSelectedEntityIndex());
}

void RoomViewerFrame::OnWarpSelect(wxCommandEvent& evt)
{
	m_roomview->SelectWarp(m_warpctrl->GetSelected());
	m_entityctrl->SetSelected(m_roomview->GetSelectedEntityIndex());
}

void RoomViewerFrame::OnWarpOpenProperties(wxCommandEvent& evt)
{
	m_roomview->SelectWarp(m_warpctrl->GetSelected());
	m_roomview->UpdateWarpProperties(m_roomview->GetSelectedWarpIndex());
}

void RoomViewerFrame::OnWarpAdd(wxCommandEvent& evt)
{
	m_roomview->AddWarp();
}

void RoomViewerFrame::OnWarpDelete(wxCommandEvent& evt)
{
	m_roomview->SelectWarp(m_warpctrl->GetSelected());
	m_roomview->DeleteSelectedWarp();
}

void RoomViewerFrame::OnHeightmapUpdate(wxCommandEvent& evt)
{
	m_roomview->RefreshHeightmap();
}

void RoomViewerFrame::OnHeightmapMove(wxCommandEvent& evt)
{
	m_roomview->RefreshGraphics();
	evt.Skip();
}

void RoomViewerFrame::OnHeightmapSelect(wxCommandEvent& evt)
{
	UpdateUI();
}

void RoomViewerFrame::OnHMTypeSelect(wxCommandEvent& evt)
{
	m_hmedit->SetSelectedType(m_tb_hmcell->GetSelection());
	evt.Skip();
}

void RoomViewerFrame::OnHMZoom(wxCommandEvent& evt)
{
	m_hmedit->SetZoom(static_cast<double>(m_tb_hmzoom->GetValue()) * 0.5);
	evt.Skip();
}

void RoomViewerFrame::OnSize(wxSizeEvent& evt)
{
	evt.Skip();
}

void RoomViewerFrame::OnTabChange(wxAuiNotebookEvent& evt)
{
	SetMode(static_cast<Mode>(evt.GetSelection()));
	evt.Skip();
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

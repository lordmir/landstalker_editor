#include "RoomViewerFrame.h"

#include <fstream>
#include <sstream>

enum MENU_IDS
{
	ID_FILE_EXPORT_BIN = 20000,
	ID_FILE_EXPORT_CSV,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_BIN,
	ID_FILE_IMPORT_CSV,
	ID_TOOLS,
	ID_TOOLS_LAYERS
};

wxBEGIN_EVENT_TABLE(RoomViewerFrame, wxWindow)
EVT_COMMAND(wxID_ANY, EVT_ZOOM_CHANGE, RoomViewerFrame::OnZoomChange)
EVT_COMMAND(wxID_ANY, EVT_OPACITY_CHANGE, RoomViewerFrame::OnOpacityChange)
wxEND_EVENT_TABLE()

RoomViewerFrame::RoomViewerFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
	  m_title(""),
	  m_mode(RoomViewerCtrl::Mode::NORMAL)
{
	m_mgr.SetManagedWindow(this);

	m_roomview = new RoomViewerCtrl(this);
	m_layerctrl = new LayerControlFrame(this);
	m_mgr.AddPane(m_layerctrl, wxAuiPaneInfo().Right().Layer(1).Resizable(false).MinSize(220,200)
		.BestSize(220,200).FloatingSize(220,200).Caption("Layers"));
	m_mgr.AddPane(m_roomview, wxAuiPaneInfo().CenterPane());

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
}

RoomViewerFrame::~RoomViewerFrame()
{
}

void RoomViewerFrame::SetMode(RoomViewerCtrl::Mode mode)
{
	m_mode = mode;
	Update();
}

void RoomViewerFrame::Update()
{
	m_layerctrl->EnableLayers(m_mode != RoomViewerCtrl::Mode::HEIGHTMAP);
	m_roomview->SetRoomNum(m_roomnum, m_mode);
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
	m_mode = RoomViewerCtrl::Mode::NORMAL;
	Update();
}

void RoomViewerFrame::ClearGameData()
{
	m_g = nullptr;
	if (m_roomview)
	{
		m_roomview->ClearGameData();
	}
	m_mode = RoomViewerCtrl::Mode::NORMAL;
	Update();
}

void RoomViewerFrame::SetRoomNum(uint16_t roomnum, RoomViewerCtrl::Mode mode)
{
	m_roomnum = roomnum;
	m_mode = mode;
	Update();
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

void RoomViewerFrame::InitStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(3);
	status.SetStatusText("", 0);
	status.SetStatusText("", 1);
	status.SetStatusText("", 1);
}

void RoomViewerFrame::UpdateStatusBar(wxStatusBar& status) const
{
	status.SetStatusText(m_roomview->GetStatusText(), 0);
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
		props.GetGrid()->Clear();
		const auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
		auto tm = m_g->GetRoomData()->GetMapForRoom(m_roomnum);

		props.Append(new wxStringProperty("Room Number", "RN", std::to_string(rd->index)));
		props.Append(new wxStringProperty("Tileset", "TS", Hex(rd->tileset)));
		props.Append(new wxStringProperty("Room Palette", "RP", Hex(rd->room_palette)));
		props.Append(new wxStringProperty("Primary Blockset", "PBT", std::to_string(rd->pri_blockset)));
		props.Append(new wxStringProperty("Secondary Blockset", "SBT", Hex(rd->sec_blockset)));
		props.Append(new wxStringProperty("BGM", "BGM", Hex(rd->bgm)));
		props.Append(new wxStringProperty("Map", "M", rd->map));
		props.Append(new wxStringProperty("Unknown Parameter 1", "UP1", std::to_string(rd->unknown_param1)));
		props.Append(new wxStringProperty("Unknown Parameter 2", "UP2", std::to_string(rd->unknown_param2)));
		props.Append(new wxStringProperty("Z Begin", "ZB", std::to_string(rd->room_z_begin)));
		props.Append(new wxStringProperty("Z End", "ZE", std::to_string(rd->room_z_end)));
		props.Append(new wxStringProperty("Tilemap Left Offset", "TLO", std::to_string(tm->GetData()->GetLeft())));
		props.Append(new wxStringProperty("Tilemap Top Offset", "TTO", std::to_string(tm->GetData()->GetTop())));
		props.Append(new wxStringProperty("Tilemap Width", "TW", std::to_string(tm->GetData()->GetWidth())));
		props.Append(new wxStringProperty("Tilemap Height", "TH", std::to_string(tm->GetData()->GetHeight())));
		props.Append(new wxStringProperty("Heightmap Width", "HW", std::to_string(tm->GetData()->GetHeightmapWidth())));
		props.Append(new wxStringProperty("Heightmap Height", "HH", std::to_string(tm->GetData()->GetHeightmapHeight())));
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void RoomViewerFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	EditorFrame::UpdateProperties(props);
	if (ArePropsInitialised() == true)
	{
		RefreshProperties(props);
	}
}

void RoomViewerFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_g != nullptr)
	{
		const auto rd = m_g->GetRoomData()->GetRoom(m_roomnum);
		auto tm = m_g->GetRoomData()->GetMapForRoom(m_roomnum);
		props.GetGrid()->SetPropertyValue("RN", _(std::to_string(rd->index)));
		props.GetGrid()->SetPropertyValue("TS", _(Hex(rd->tileset)));
		props.GetGrid()->SetPropertyValue("RP", _(Hex(rd->room_palette)));
		props.GetGrid()->SetPropertyValue("PBT", _(std::to_string(rd->pri_blockset)));
		props.GetGrid()->SetPropertyValue("SBT", _(Hex(rd->sec_blockset)));
		props.GetGrid()->SetPropertyValue("BGM", _(Hex(rd->bgm)));
		props.GetGrid()->SetPropertyValue("M", _(rd->map));
		props.GetGrid()->SetPropertyValue("UP1", _(std::to_string(rd->unknown_param1)));
		props.GetGrid()->SetPropertyValue("UP2", _(std::to_string(rd->unknown_param2)));
		props.GetGrid()->SetPropertyValue("ZB", _(std::to_string(rd->room_z_begin)));
		props.GetGrid()->SetPropertyValue("ZE", _(std::to_string(rd->room_z_end)));
		props.GetGrid()->SetPropertyValue("TLO", _(std::to_string(tm->GetData()->GetLeft())));
		props.GetGrid()->SetPropertyValue("TTO", _(std::to_string(tm->GetData()->GetTop())));
		props.GetGrid()->SetPropertyValue("TW", _(std::to_string(tm->GetData()->GetWidth())));
		props.GetGrid()->SetPropertyValue("TH", _(std::to_string(tm->GetData()->GetHeight())));
		props.GetGrid()->SetPropertyValue("HW", _(std::to_string(tm->GetData()->GetHeightmapWidth())));
		props.GetGrid()->SetPropertyValue("HH", _(std::to_string(tm->GetData()->GetHeightmapHeight())));
	}
}

void RoomViewerFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
	wxPGProperty* property = evt.GetProperty();
	if (property == nullptr)
	{
		return;
	}

	const wxString& name = property->GetName();
}

void RoomViewerFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_BIN, "Export Map as Binary...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_CSV, "Export Map as CSV Set...");
	AddMenuItem(fileMenu, 3, ID_FILE_EXPORT_PNG, "Export Map as PNG...");
	AddMenuItem(fileMenu, 4, ID_FILE_IMPORT_BIN, "Import Map from Binary...");
	AddMenuItem(fileMenu, 5, ID_FILE_IMPORT_CSV, "Import Map from CSV...");
	AddMenuItem(fileMenu, 6, wxID_ANY, "", wxITEM_SEPARATOR);
	auto& toolsMenu = AddMenu(menu, 1, ID_TOOLS, "Tools");
	AddMenuItem(toolsMenu, 0, ID_TOOLS_LAYERS, "Layers", wxITEM_CHECK);

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
		case ID_TOOLS_LAYERS:
			SetPaneVisibility(m_layerctrl, !IsPaneVisible(m_layerctrl));
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
	Update();
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
	Update();
}


void RoomViewerFrame::UpdateUI() const
{
	CheckMenuItem(ID_TOOLS_LAYERS, IsPaneVisible(m_layerctrl));
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

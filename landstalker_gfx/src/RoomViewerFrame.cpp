#include "RoomViewerFrame.h"

enum MENU_IDS
{
	ID_FILE_EXPORT_BIN = 20000,
	ID_FILE_EXPORT_CSV,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_BIN,
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

void RoomViewerFrame::UpdateStatusBar(wxStatusBar& status) const
{
	status.SetStatusText(m_roomview->GetStatusText());
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
		RefreshProperties(props);
	}
	EditorFrame::InitProperties(props);
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
	AddMenuItem(fileMenu, 4, ID_FILE_IMPORT_BIN, "Import Map...");
	AddMenuItem(fileMenu, 5, wxID_ANY, "", wxITEM_SEPARATOR);
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
			break;
		case ID_FILE_EXPORT_CSV:
			break;
		case ID_FILE_EXPORT_PNG:
			break;
		case ID_FILE_IMPORT_BIN:
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
	wxPostEvent(this, evt);
}

void RoomViewerFrame::FireEvent(const wxEventType& e, const std::string& userdata)
{
	wxCommandEvent evt(e);
	evt.SetString(userdata);
	wxPostEvent(this, evt);
}

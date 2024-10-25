#include <user_interface/rooms/include/TileSwapDataViewModel.h>

TileSwapDataViewModel::TileSwapDataViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
	: BaseDataViewModel(),
	  m_roomnum(roomnum),
	  m_gd(gd)
{
	Initialise();
}

void TileSwapDataViewModel::Initialise()
{
	m_swaps = m_gd->GetRoomData()->GetTileSwaps(m_roomnum);
	Reset(GetRowCount());
}

void TileSwapDataViewModel::CommitData()
{
	m_gd->GetRoomData()->SetTileSwaps(m_roomnum, m_swaps);
}

unsigned int TileSwapDataViewModel::GetColumnCount() const
{
	return 15;
}

unsigned int TileSwapDataViewModel::GetRowCount() const
{
	return m_swaps.size();
}

wxString TileSwapDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Index";
	case 1:
		return "Active";
	case 2:
		return "Type";
	case 3:
		return "Map Src X";
	case 4:
		return "Map Src Y";
	case 5:
		return "Map Dst X";
	case 6:
		return "Map Dst Y";
	case 7:
		return "Map Width";
	case 8:
		return "Map Height";
	case 9:
		return "HM Src X";
	case 10:
		return "HM Src Y";
	case 11:
		return "HM Dst X";
	case 12:
		return "HM Dst Y";
	case 13:
		return "HM Width";
	case 14:
		return "HM Height";
	default:
		return "?";
	}
}

wxArrayString TileSwapDataViewModel::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	if (col == 2)
	{
		choices.Add("Floor");
		choices.Add("Wall NE");
		choices.Add("Wall NW");
	}
	return choices;
}

wxString TileSwapDataViewModel::GetColumnType(unsigned int /*col*/) const
{
	return "long";
}

void TileSwapDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (col == 0)
	{
		variant = static_cast<long>(row);
	}
	else if (row < m_swaps.size())
	{
		switch (col)
		{
		case 1:
			variant = static_cast<bool>(m_swaps.at(row).active);
			break;
		case 2:
			variant = static_cast<long>(m_swaps.at(row).mode);
			break;
		case 3:
			variant = static_cast<long>(m_swaps.at(row).map.src_x);
			break;
		case 4:
			variant = static_cast<long>(m_swaps.at(row).map.src_y);
			break;
		case 5:
			variant = static_cast<long>(m_swaps.at(row).map.dst_x);
			break;
		case 6:
			variant = static_cast<long>(m_swaps.at(row).map.dst_y);
			break;
		case 7:
			variant = static_cast<long>(m_swaps.at(row).map.width);
			break;
		case 8:
			variant = static_cast<long>(m_swaps.at(row).map.height);
			break;
		case 9:
			variant = static_cast<long>(m_swaps.at(row).heightmap.src_x);
			break;
		case 10:
			variant = static_cast<long>(m_swaps.at(row).heightmap.src_y);
			break;
		case 11:
			variant = static_cast<long>(m_swaps.at(row).heightmap.dst_x);
			break;
		case 12:
			variant = static_cast<long>(m_swaps.at(row).heightmap.dst_y);
			break;
		case 13:
			variant = static_cast<long>(m_swaps.at(row).heightmap.width);
			break;
		case 14:
			variant = static_cast<long>(m_swaps.at(row).heightmap.height);
			break;
		default:
			break;
		}
	}
}

bool TileSwapDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool TileSwapDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	bool updated = false;
	if (row < m_swaps.size())
	{
		switch (col)
		{
		case 0:
			break;
		case 1:
			m_swaps[row].active = variant.GetBool();
			updated = true;
			break;
		case 2:
			m_swaps[row].mode = static_cast<TileSwap::Mode>(std::clamp<long>(variant.GetLong(), 0, 2));
			updated = true;
			break;
		case 3:
			m_swaps[row].map.src_x = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 4:
			m_swaps[row].map.src_y = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 5:
			m_swaps[row].map.dst_x = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 6:
			m_swaps[row].map.dst_y = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 7:
			m_swaps[row].map.width = std::clamp<uint8_t>(variant.GetLong(), 1, 0x40);
			updated = true;
			break;
		case 8:
			m_swaps[row].map.height = std::clamp<uint8_t>(variant.GetLong(), 1, 0x40);
			updated = true;
			break;
		case 9:
			m_swaps[row].heightmap.src_x = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 10:
			m_swaps[row].heightmap.src_y = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 11:
			m_swaps[row].heightmap.dst_x = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 12:
			m_swaps[row].heightmap.dst_y = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 13:
			m_swaps[row].heightmap.width = std::clamp<uint8_t>(variant.GetLong(), 1, 0x40);
			updated = true;
			break;
		case 14:
			m_swaps[row].heightmap.height = std::clamp<uint8_t>(variant.GetLong(), 1, 0x40);
			updated = true;
			break;
		default:
			break;
		}
	}
	return updated;
}

bool TileSwapDataViewModel::DeleteRow(unsigned int row)
{
	if (row < m_swaps.size())
	{
		m_swaps.erase(m_swaps.begin() + row);
		Reset(GetRowCount());
		return true;
	}
	return false;
}

bool TileSwapDataViewModel::AddRow(unsigned int row)
{
	if (row <= m_swaps.size())
	{
		m_swaps.insert(m_swaps.begin() + row, TileSwap());
		Reset(GetRowCount());
		return true;
	}
	return false;
}

bool TileSwapDataViewModel::SwapRows(unsigned int r1, unsigned int r2)
{
	if (r1 < m_swaps.size() && r2 < m_swaps.size() && r1 != r2)
	{
		std::swap(m_swaps[r1], m_swaps[r2]);
		return true;
	}
	return false;
}

void TileSwapDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	// Index
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer("long"), 0, 64, wxALIGN_LEFT));
	// Active
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewToggleRenderer("bool", wxDATAVIEW_CELL_ACTIVATABLE), 1, 40, wxALIGN_LEFT));
	// Type
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(2)), 2, 75, wxALIGN_LEFT));
	// Map Src X
	ctrl->InsertColumn(3, new wxDataViewColumn(this->GetColumnHeader(3),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 3, 70, wxALIGN_LEFT));
	// Map Src Y
	ctrl->InsertColumn(4, new wxDataViewColumn(this->GetColumnHeader(4),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 4, 70, wxALIGN_LEFT));
	// Map Dst X
	ctrl->InsertColumn(5, new wxDataViewColumn(this->GetColumnHeader(5),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 5, 70, wxALIGN_LEFT));
	// Map Dst Y
	ctrl->InsertColumn(6, new wxDataViewColumn(this->GetColumnHeader(6),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 6, 70, wxALIGN_LEFT));
	// Map Width
	ctrl->InsertColumn(7, new wxDataViewColumn(this->GetColumnHeader(7),
		new wxDataViewSpinRenderer(1, 0x40, wxDATAVIEW_CELL_EDITABLE), 7, 70, wxALIGN_LEFT));
	// Map Height
	ctrl->InsertColumn(8, new wxDataViewColumn(this->GetColumnHeader(8),
		new wxDataViewSpinRenderer(1, 0x40, wxDATAVIEW_CELL_EDITABLE), 8, 70, wxALIGN_LEFT));
	// HM Src X
	ctrl->InsertColumn(9, new wxDataViewColumn(this->GetColumnHeader(9),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 9, 70, wxALIGN_LEFT));
	// HM Src Y
	ctrl->InsertColumn(10, new wxDataViewColumn(this->GetColumnHeader(10),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 10, 70, wxALIGN_LEFT));
	// HM Dst X
	ctrl->InsertColumn(11, new wxDataViewColumn(this->GetColumnHeader(11),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 11, 70, wxALIGN_LEFT));
	// HM Dst Y
	ctrl->InsertColumn(12, new wxDataViewColumn(this->GetColumnHeader(12),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 12, 70, wxALIGN_LEFT));
	// HM Width
	ctrl->InsertColumn(13, new wxDataViewColumn(this->GetColumnHeader(13),
		new wxDataViewSpinRenderer(1, 0x40, wxDATAVIEW_CELL_EDITABLE), 13, 70, wxALIGN_LEFT));
	// HM Height
	ctrl->InsertColumn(14, new wxDataViewColumn(this->GetColumnHeader(14),
		new wxDataViewSpinRenderer(1, 0x40, wxDATAVIEW_CELL_EDITABLE), 14, 70, wxALIGN_LEFT));
}

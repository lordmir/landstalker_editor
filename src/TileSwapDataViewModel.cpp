#include <TileSwapDataViewModel.h>

TileSwapDataViewModel::TileSwapDataViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
	: wxDataViewVirtualListModel(),
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
	return 14;
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
		return "Type";
	case 2:
		return "Map Src X";
	case 3:
		return "Map Src Y";
	case 4:
		return "Map Dst X";
	case 5:
		return "Map Dst Y";
	case 6:
		return "Map Width";
	case 7:
		return "Map Height";
	case 8:
		return "HM Src X";
	case 9:
		return "HM Src Y";
	case 10:
		return "HM Dst X";
	case 11:
		return "HM Dst Y";
	case 12:
		return "HM Width";
	case 13:
		return "HM Height";
	default:
		return "?";
	}
}

wxArrayString TileSwapDataViewModel::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	if (col == 1)
	{
		choices.Add("Floor");
		choices.Add("Wall NE");
		choices.Add("Wall NW");
	}
	return choices;
}

wxString TileSwapDataViewModel::GetColumnType(unsigned int col) const
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
			variant = static_cast<long>(m_swaps.at(row).mode);
			break;
		case 2:
			variant = static_cast<long>(m_swaps.at(row).map.src_x);
			break;
		case 3:
			variant = static_cast<long>(m_swaps.at(row).map.src_y);
			break;
		case 4:
			variant = static_cast<long>(m_swaps.at(row).map.dst_x);
			break;
		case 5:
			variant = static_cast<long>(m_swaps.at(row).map.dst_y);
			break;
		case 6:
			variant = static_cast<long>(m_swaps.at(row).map.width);
			break;
		case 7:
			variant = static_cast<long>(m_swaps.at(row).map.height);
			break;
		case 8:
			variant = static_cast<long>(m_swaps.at(row).heightmap.src_x);
			break;
		case 9:
			variant = static_cast<long>(m_swaps.at(row).heightmap.src_y);
			break;
		case 10:
			variant = static_cast<long>(m_swaps.at(row).heightmap.dst_x);
			break;
		case 11:
			variant = static_cast<long>(m_swaps.at(row).heightmap.dst_y);
			break;
		case 12:
			variant = static_cast<long>(m_swaps.at(row).heightmap.width);
			break;
		case 13:
			variant = static_cast<long>(m_swaps.at(row).heightmap.height);
			break;
		default:
			break;
		}
	}
}

bool TileSwapDataViewModel::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const
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
			m_swaps[row].mode = static_cast<TileSwap::Mode>(std::clamp<long>(variant.GetLong(), 0, 2));
			updated = true;
			break;
		case 2:
			m_swaps[row].map.src_x = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 3:
			m_swaps[row].map.src_y = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 4:
			m_swaps[row].map.dst_x = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 5:
			m_swaps[row].map.dst_y = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 6:
			m_swaps[row].map.width = std::clamp<uint8_t>(variant.GetLong(), 1, 0x40);
			updated = true;
			break;
		case 7:
			m_swaps[row].map.height = std::clamp<uint8_t>(variant.GetLong(), 1, 0x40);
			updated = true;
			break;
		case 8:
			m_swaps[row].heightmap.src_x = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 9:
			m_swaps[row].heightmap.src_y = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 10:
			m_swaps[row].heightmap.dst_x = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 11:
			m_swaps[row].heightmap.dst_y = std::clamp<uint8_t>(variant.GetLong(), 0, 0x3F);
			updated = true;
			break;
		case 12:
			m_swaps[row].heightmap.width = std::clamp<uint8_t>(variant.GetLong(), 1, 0x40);
			updated = true;
			break;
		case 13:
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

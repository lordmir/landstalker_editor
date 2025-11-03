#include <rooms/DoorDataViewModel.h>

DoorDataViewModel::DoorDataViewModel(uint16_t roomnum, std::shared_ptr<Landstalker::GameData> gd)
	: BaseDataViewModel(),
	m_roomnum(roomnum),
	m_gd(gd)
{
	Initialise();
}

void DoorDataViewModel::Initialise()
{
	m_doors = m_gd->GetRoomData()->GetDoors(m_roomnum);
	Reset(GetRowCount());
}

void DoorDataViewModel::CommitData()
{
	m_gd->GetRoomData()->SetDoors(m_roomnum, m_doors);
}

unsigned int DoorDataViewModel::GetColumnCount() const
{
	return 4;
}

unsigned int DoorDataViewModel::GetRowCount() const
{
	return m_doors.size();
}

wxString DoorDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Index";
	case 1:
		return "X";
	case 2:
		return "Y";
	case 3:
		return "Size";
	default:
		return "?";
	}
}

wxArrayString DoorDataViewModel::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	if (col == 3)
	{
		for (const auto& s : Landstalker::Door::SIZE_NAMES)
		{
			choices.Add(s.second);
		}
	}
	return choices;
}

wxString DoorDataViewModel::GetColumnType(unsigned int /*col*/) const
{
	return "long";
}

void DoorDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (col == 0)
	{
		variant = static_cast<long>(row);
	}
	else if (row < m_doors.size())
	{
		switch (col)
		{
		case 1:
			variant = static_cast<long>(m_doors.at(row).x);
			break;
		case 2:
			variant = static_cast<long>(m_doors.at(row).y);
			break;
		case 3:
			variant = static_cast<long>(m_doors.at(row).size);
			break;
		default:
			break;
		}
	}
}

bool DoorDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool DoorDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	bool updated = false;
	if (row < m_doors.size())
	{
		switch (col)
		{
		case 0:
			break;
		case 1:
			m_doors[row].x = static_cast<uint8_t>(std::clamp<long>(variant.GetLong(), 0, 0x3F));
			updated = true;
			break;
		case 2:
			m_doors[row].y = static_cast<uint8_t>(std::clamp<long>(variant.GetLong(), 0, 0x3F));
			updated = true;
			break;
		case 3:
			m_doors[row].size = static_cast<Landstalker::Door::Size>(std::clamp<long>(variant.GetLong(), 0, 7));
			updated = true;
			break;
		default:
			break;
		}
	}
	return updated;
}

bool DoorDataViewModel::DeleteRow(unsigned int row)
{
	if (row < m_doors.size())
	{
		m_doors.erase(m_doors.begin() + row);
		Reset(GetRowCount());
		return true;
	}
	return false;
}

bool DoorDataViewModel::AddRow(unsigned int row)
{
	if (row <= m_doors.size())
	{
		m_doors.insert(m_doors.begin() + row, Landstalker::Door());
		Reset(GetRowCount());
		return true;
	}
	return false;
}

bool DoorDataViewModel::SwapRows(unsigned int r1, unsigned int r2)
{
	if (r1 < m_doors.size() && r2 < m_doors.size() && r1 != r2)
	{
		std::swap(m_doors[r1], m_doors[r2]);
		return true;
	}
	return false;
}

void DoorDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	// Index
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer("long"), 0, 64, wxALIGN_LEFT));
	// Map Src X
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 1, 100, wxALIGN_LEFT));
	// Map Src Y
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewSpinRenderer(0, 0x3F, wxDATAVIEW_CELL_EDITABLE), 2, 100, wxALIGN_LEFT));
	// Size
	ctrl->InsertColumn(3, new wxDataViewColumn(this->GetColumnHeader(3),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(3)), 3, 100, wxALIGN_LEFT));
}

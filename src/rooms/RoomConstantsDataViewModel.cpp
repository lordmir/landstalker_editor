#include <rooms/RoomConstantsDataViewModel.h>

#include <misc/LookupDataViewRenderer.h>

RoomConstantsDataViewModel::RoomConstantsDataViewModel(std::shared_ptr<Landstalker::GameData> gd)
	: BaseDataViewModel(),
	  m_gd(gd)
{
	Initialise();
}

RoomConstantsDataViewModel::~RoomConstantsDataViewModel()
{
}

void RoomConstantsDataViewModel::Initialise()
{
	m_names.clear();
	m_room_choices.Clear();
	if (m_gd)
	{
		const auto& constants = m_gd->GetRoomData()->GetRoomConstants();
		m_names.reserve(constants.size());
		for (const auto& c : constants)
		{
			m_names.push_back(c.first);
		}
		// Sorted by room so the list reads as a map of the game, matching the order the
		// constants are written back out in.
		std::sort(m_names.begin(), m_names.end(), [&](const std::string& lhs, const std::string& rhs)
		{
			const auto l = m_gd->GetRoomData()->GetRoomConstant(lhs).value_or(0);
			const auto r = m_gd->GetRoomData()->GetRoomConstant(rhs).value_or(0);
			return std::tie(l, lhs) < std::tie(r, rhs);
		});

		const auto room_count = m_gd->GetRoomData()->GetRoomCount();
		m_room_choices.reserve(room_count);
		for (std::size_t i = 0; i < room_count; ++i)
		{
			// Mirrors the "[Flag 0000] (Label)" convention the lookup control uses
			// elsewhere, so a room can be found by typing either its number or its name.
			std::wstring label = Landstalker::StrWPrintf("[Room %04d]", static_cast<int>(i));
			if (Landstalker::Labels::Exists(Landstalker::Labels::C_ROOMS, static_cast<int>(i)))
			{
				label += L" (" + *Landstalker::Labels::Get(Landstalker::Labels::C_ROOMS, static_cast<int>(i)) + L")";
			}
			m_room_choices.Add(label);
		}
	}
	Reset(GetRowCount());
}

void RoomConstantsDataViewModel::CommitData()
{
	// Edits are written straight through to the game data, so there is nothing pending.
}

unsigned int RoomConstantsDataViewModel::GetColumnCount() const
{
	return 2;
}

unsigned int RoomConstantsDataViewModel::GetRowCount() const
{
	return static_cast<unsigned int>(m_names.size());
}

wxString RoomConstantsDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return _("Constant");
	case 1:
		return _("Room");
	default:
		return "???";
	}
}

wxArrayString RoomConstantsDataViewModel::GetColumnChoices(unsigned int col) const
{
	switch (col)
	{
	case 1:
		return m_room_choices;
	default:
		return wxArrayString();
	}
}

wxString RoomConstantsDataViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "string";
	case 1:
		return "long";
	default:
		return "???";
	}
}

std::string RoomConstantsDataViewModel::GetRowName(unsigned int row) const
{
	return row < m_names.size() ? m_names[row] : std::string();
}

void RoomConstantsDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (!m_gd || row >= m_names.size())
	{
		return;
	}
	switch (col)
	{
	case 0:
		variant = m_names[row];
		break;
	case 1:
		variant = static_cast<long>(m_gd->GetRoomData()->GetRoomConstant(m_names[row]).value_or(0));
		break;
	default:
		break;
	}
}

bool RoomConstantsDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool RoomConstantsDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	if (!m_gd || row >= m_names.size())
	{
		return false;
	}
	auto rd = m_gd->GetRoomData();
	switch (col)
	{
	case 0:
	{
		// RenameRoomConstant rejects a name that is invalid or already taken, in which
		// case the edit is dropped and the row keeps its old name.
		const std::string name = variant.GetString().ToStdString();
		if (!rd->RenameRoomConstant(m_names[row], name))
		{
			return false;
		}
		m_names[row] = name;
		return true;
	}
	case 1:
	{
		const long room = variant.GetLong();
		if (room < 0 || room >= static_cast<long>(rd->GetRoomCount()))
		{
			return false;
		}
		return rd->SetRoomConstant(m_names[row], static_cast<uint16_t>(room));
	}
	default:
		break;
	}
	return false;
}

std::string RoomConstantsDataViewModel::MakeUniqueName() const
{
	const std::string prefix = "ROOM_NEW";
	if (!m_gd->GetRoomData()->GetRoomConstant(prefix).has_value())
	{
		return prefix;
	}
	for (unsigned int suffix = 1; ; ++suffix)
	{
		const std::string candidate = prefix + "_" + std::to_string(suffix);
		if (!m_gd->GetRoomData()->GetRoomConstant(candidate).has_value())
		{
			return candidate;
		}
	}
}

bool RoomConstantsDataViewModel::DeleteRow(unsigned int row)
{
	if (!m_gd || row >= m_names.size())
	{
		return false;
	}
	if (!m_gd->GetRoomData()->DeleteRoomConstant(m_names[row]))
	{
		return false;
	}
	m_names.erase(m_names.begin() + row);
	RowDeleted(row);
	return true;
}

bool RoomConstantsDataViewModel::AddRow(unsigned int row)
{
	if (!m_gd || row > m_names.size() || m_gd->GetRoomData()->GetRoomCount() == 0)
	{
		return false;
	}
	const std::string name = MakeUniqueName();
	if (!m_gd->GetRoomData()->SetRoomConstant(name, 0))
	{
		return false;
	}
	m_names.insert(m_names.begin() + row, name);
	RowInserted(row);
	return true;
}

bool RoomConstantsDataViewModel::SwapRows(unsigned int r1, unsigned int r2)
{
	// Order is presentational only - the file is written sorted by room number - so
	// there is nothing meaningful to reorder.
	return false;
}

void RoomConstantsDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer(GetColumnType(0), wxDATAVIEW_CELL_EDITABLE), 0, 320, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new LookupDataViewRenderer(wxDATAVIEW_CELL_EDITABLE, GetColumnChoices(1)), 1, 320, wxALIGN_LEFT));
}

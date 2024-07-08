#include <FlagDataViewModel.h>
#include <numeric>

template <>
unsigned int FlagDataViewModel<EntityFlag>::GetColumnCount() const
{
	return 3;
}

template <>
wxString FlagDataViewModel<EntityFlag>::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Entity";
	case 1:
		return "Flag";
	case 2:
		return "Hide Entity When";
	default:
		return "?";
	}
}

template <>
wxArrayString FlagDataViewModel<EntityFlag>::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	switch (col)
	{
	case 0:
	{
		auto ent = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
		for (std::size_t i = 0; i < 15; ++i)
		{
			if (i < ent.size())
			{
				choices.Add(StrPrintf("[%02d] %s (%04.1f, %04.1f, %04.1f)", i + 1, ent[i].GetTypeName().c_str(),
					ent[i].GetXDbl(), ent[i].GetYDbl(), ent[i].GetZDbl()));
			}
			else
			{
				choices.Add(StrPrintf("[%02d] ???", i + 1));
			}
		}
		break;
	}
	case 2:
		choices.Add("Set");
		choices.Add("Clear");
		break;
	default:
		break;
	}
	return choices;
}

template <>
wxString FlagDataViewModel<EntityFlag>::GetColumnType(unsigned int col) const
{
	switch(col)
	{
	case 0:
		return "long";
	case 1:
		return "long";
	case 2:
		return "long";
	default:
		return "string";
	}
}

template <>
void FlagDataViewModel<EntityFlag>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row < m_data.size())
	{
		switch (col)
		{
		case 0:
			variant = static_cast<long>(m_data[row].entity);
			break;
		case 1:
			variant = static_cast<long>(m_data[row].flag);
			break;
		case 2:
			variant = m_data[row].set ? 1L : 0L;
			break;
		default:
			break;
		}
	}
}

template <>
bool FlagDataViewModel<EntityFlag>::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

template <>
bool FlagDataViewModel<EntityFlag>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	bool updated = false;
	if (row < m_data.size())
	{
		switch (col)
		{
		case 0:
			m_data[row].entity = variant.GetLong();
			updated = true;
			break;
		case 1:
			m_data[row].flag = variant.GetLong();
			updated = true;
			break;
		case 2:
			m_data[row].set = variant.GetLong() != 0L;
			updated = true;
			break;
		default:
			break;
		}
	}
	return updated;
}

template <>
void FlagDataViewModel<EntityFlag>::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(0)), 0, 340, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 120, wxALIGN_LEFT));
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(2)), 2, 100, wxALIGN_LEFT));
}

template <>
unsigned int FlagDataViewModel<OneTimeEventFlag>::GetColumnCount() const
{
	return 5;
}

template <>
wxString FlagDataViewModel<OneTimeEventFlag>::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Entity";
	case 1:
		return "Show Flag";
	case 2:
		return "When";
	case 3:
		return "Hide Flag";
	case 4:
		return "When";
	default:
		return "?";
	}
}

template <>
wxArrayString FlagDataViewModel<OneTimeEventFlag>::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	switch (col)
	{
	case 0:
	{
		auto ent = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
		for (std::size_t i = 0; i < 15; ++i)
		{
			if (i < ent.size())
			{
				choices.Add(StrPrintf("[%02d] %s (%04.1f, %04.1f, %04.1f)", i + 1, ent[i].GetTypeName().c_str(),
					ent[i].GetXDbl(), ent[i].GetYDbl(), ent[i].GetZDbl()));
			}
			else
			{
				choices.Add(StrPrintf("[%02d] ???", i + 1));
			}
		}
		break;
	}
	case 2:
	case 4:
		choices.Add("Set");
		choices.Add("Clear");
		break;
	default:
		break;
	}
	return choices;
}

template <>
wxString FlagDataViewModel<OneTimeEventFlag>::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 1:
		return "long";
	case 2:
		return "long";
	case 3:
		return "long";
	case 4:
		return "long";
	default:
		return "string";
	}
}

template <>
void FlagDataViewModel<OneTimeEventFlag>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row < m_data.size())
	{
		switch (col)
		{
		case 0:
			variant = static_cast<long>(m_data[row].entity);
			break;
		case 1:
			variant = static_cast<long>(m_data[row].flag_on);
			break;
		case 2:
			variant = m_data[row].flag_on_set ? 1L : 0L;
			break;
		case 3:
			variant = static_cast<long>(m_data[row].flag_off);
			break;
		case 4:
			variant = m_data[row].flag_off_set ? 1L : 0L;
			break;
		default:
			break;
		}
	}
}

template <>
bool FlagDataViewModel<OneTimeEventFlag>::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

template <>
bool FlagDataViewModel<OneTimeEventFlag>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	bool updated = false;
	if (row < m_data.size())
	{
		switch (col)
		{
		case 0:
			m_data[row].entity = variant.GetLong();
			updated = true;
			break;
		case 1:
			m_data[row].flag_on = variant.GetLong();
			updated = true;
			break;
		case 2:
			m_data[row].flag_on_set = variant.GetLong() != 0L;
			updated = true;
			break;
		case 3:
			m_data[row].flag_off = variant.GetLong();
			updated = true;
			break;
		case 4:
			m_data[row].flag_off_set = variant.GetLong() != 0L;
			updated = true;
			break;
		default:
			break;
		}
	}
	return updated;
}

template <>
void FlagDataViewModel<OneTimeEventFlag>::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(0)), 0, 200, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 100, wxALIGN_LEFT));
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(2)), 2, 80, wxALIGN_LEFT));
	ctrl->InsertColumn(3, new wxDataViewColumn(this->GetColumnHeader(3),
		new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 3, 100, wxALIGN_LEFT));
	ctrl->InsertColumn(4, new wxDataViewColumn(this->GetColumnHeader(4),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(4)), 4, 80, wxALIGN_LEFT));
}

template <>
unsigned int FlagDataViewModel<RoomClearFlag>::GetColumnCount() const
{
	return 2;
}

template <>
wxString FlagDataViewModel<RoomClearFlag>::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Start Entity";
	case 1:
		return "When Flag Set";
	default:
		return "?";
	}
}

template <>
wxArrayString FlagDataViewModel<RoomClearFlag>::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	switch (col)
	{
	case 0:
	{
		auto ent = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
		for (std::size_t i = 0; i < 15; ++i)
		{
			if (i < ent.size())
			{
				choices.Add(StrPrintf("[%02d] %s (%04.1f, %04.1f, %04.1f)", i + 1, ent[i].GetTypeName().c_str(),
					ent[i].GetXDbl(), ent[i].GetYDbl(), ent[i].GetZDbl()));
			}
			else
			{
				choices.Add(StrPrintf("[%02d] ???", i + 1));
			}
		}
		break;
	}
	default:
		break;
	}
	return choices;
}

template <>
wxString FlagDataViewModel<RoomClearFlag>::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 1:
		return "long";
	default:
		return "string";
	}
}

template <>
void FlagDataViewModel<RoomClearFlag>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row < m_data.size())
	{
		switch (col)
		{
		case 0:
			variant = static_cast<long>(m_data[row].entity);
			break;
		case 1:
			variant = static_cast<long>(m_data[row].flag);
			break;
		default:
			break;
		}
	}
}

template <>
bool FlagDataViewModel<RoomClearFlag>::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

template <>
bool FlagDataViewModel<RoomClearFlag>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	bool updated = false;
	if (row < m_data.size())
	{
		switch (col)
		{
		case 0:
			m_data[row].entity = variant.GetLong();
			updated = true;
			break;
		case 1:
			m_data[row].flag = variant.GetLong();
			updated = true;
			break;
		default:
			break;
		}
	}
	return updated;
}

template <>
void FlagDataViewModel<RoomClearFlag>::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(0)), 0, 340, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 120, wxALIGN_LEFT));
}

wxString LockedDoorFlagViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Locked Door Entity";
	case 1:
		return "Unlocked Flag";
	default:
		return "?";
	}
}

unsigned int LockedDoorFlagViewModel::GetColumnCount() const
{
	return 2;
}

wxString PermanentSwitchFlagViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Switch Entity";
	case 1:
		return "Switch On Flag";
	default:
		return "?";
	}
}

unsigned int PermanentSwitchFlagViewModel::GetColumnCount() const
{
	return 2;
}

template <>
unsigned int FlagDataViewModel<SacredTreeFlag>::GetColumnCount() const
{
	return 2;
}

template <>
wxString FlagDataViewModel<SacredTreeFlag>::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Tree Entity";
	case 1:
		return "Cut Down Flag";
	default:
		return "?";
	}
}

template <>
wxArrayString FlagDataViewModel<SacredTreeFlag>::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	switch (col)
	{
	case 0:
	{
		choices.Add("???");
		break;
	}
	default:
		break;
	}
	return choices;
}

template <>
wxString FlagDataViewModel<SacredTreeFlag>::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "string";
	case 1:
		return "long";
	default:
		return "string";
	}
}

template <>
void FlagDataViewModel<SacredTreeFlag>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row < m_data.size())
	{
		auto ent = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
		unsigned int tree = 0;
		bool found = false;
		switch (col)
		{
		case 0:
			for (std::size_t i = 0; i < 15; ++i)
			{
				if (i < ent.size() && ent[i].GetType() == 0x4B)
				{
					if (tree == row)
					{
						variant = StrPrintf("[%02d] %s (%04.1f, %04.1f, %04.1f)", i + 1, ent[i].GetTypeName().c_str(),
							ent[i].GetXDbl(), ent[i].GetYDbl(), ent[i].GetZDbl());
						found = true;
						break;
					}
					tree++;
				}
			}
			if (!found)
			{
				variant = "???";
			}
			break;
		case 1:
			variant = static_cast<long>(m_data[row].flag);
			break;
		default:
			break;
		}
	}
}

template <>
bool FlagDataViewModel<SacredTreeFlag>::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

template <>
bool FlagDataViewModel<SacredTreeFlag>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	bool updated = false;
	if (row < m_data.size())
	{
		switch (col)
		{
		case 0:
			updated = false;
			break;
		case 1:
			m_data[row].flag = variant.GetLong();
			updated = true;
			break;
		default:
			break;
		}
	}
	return updated;
}

template <>
void FlagDataViewModel<SacredTreeFlag>::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer(), 0, 420, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 140, wxALIGN_LEFT));
}

template <>
unsigned int FlagDataViewModel<WarpList::Transition>::GetColumnCount() const
{
	return 2;
}

template <>
wxString FlagDataViewModel<WarpList::Transition>::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Destination";
	case 1:
		return "On Flag Set";
	default:
		return "?";
	}
}

template <>
wxArrayString FlagDataViewModel<WarpList::Transition>::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	switch (col)
	{
	case 0:
		for (std::size_t i = 0; i < m_gd->GetRoomData()->GetRoomCount(); ++i)
		{
			choices.Add(m_gd->GetRoomData()->GetRoom(i)->name);
		}
		break;
	default:
		break;
	}
	return choices;
}

template <>
wxString FlagDataViewModel<WarpList::Transition>::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 1:
		return "long";
	default:
		return "string";
	}
}

template <>
void FlagDataViewModel<WarpList::Transition>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row < m_data.size())
	{
		switch (col)
		{
		case 0:
			variant = static_cast<long>(m_data[row].dst_rm);
			break;
		case 1:
			variant = static_cast<long>(m_data[row].flag);
			break;
		default:
			break;
		}
	}
}

template <>
bool FlagDataViewModel<WarpList::Transition>::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

template <>
bool FlagDataViewModel<WarpList::Transition>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	bool updated = false;
	if (row < m_data.size())
	{
		switch (col)
		{
		case 0:
			m_data[row].dst_rm = variant.GetLong();
			updated = true;
			break;
		case 1:
			m_data[row].flag = variant.GetLong();
			updated = true;
			break;
		default:
			break;
		}
	}
	return updated;
}

template <>
void FlagDataViewModel<WarpList::Transition>::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(0)), 0, 420, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewSpinRenderer(0, 1023, wxDATAVIEW_CELL_EDITABLE), 1, 140, wxALIGN_LEFT));
}

template <>
unsigned int FlagDataViewModel<ChestItem>::GetColumnCount() const
{
	return 2;
}

template <>
wxString FlagDataViewModel<ChestItem>::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Chest";
	case 1:
		return "Contents";
	default:
		return "?";
	}
}

template <>
wxArrayString FlagDataViewModel<ChestItem>::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	switch (col)
	{
	case 1:
	{
		for (std::size_t i = 0; i < 0x41; ++i)
		{
			if (i < m_gd->GetStringData()->GetItemNameCount())
			{
				choices.Add(StrPrintf("[%02X] ", i) + _(m_gd->GetStringData()->GetItemName(i)));
			}
			else
			{
				choices.Add(StrPrintf("[%02X] ???", i));
			}
		}
		break;
	}
	default:
		break;
	}
	return choices;
}

template <>
wxString FlagDataViewModel<ChestItem>::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "string";
	case 1:
		return "long";
	default:
		return "string";
	}
}

template <>
void FlagDataViewModel<ChestItem>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	auto ent = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
	unsigned int count = 0;
	int idx = 0;
	switch (col)
	{
	case 0:
		for (const auto& e : ent)
		{
			if (e.GetType() == 0x12)
			{
				count++;
				if (count > row)
				{
					break;
				}
			}
			idx++;
		}
		if (count > row)
		{
			variant = StrPrintf("[%02d] %s (%04.1f, %04.1f, %04.1f)", idx + 1, ent[idx].GetTypeName().c_str(),
				ent[idx].GetXDbl(), ent[idx].GetYDbl(), ent[idx].GetZDbl());
		}
		else
		{
			variant = "???";
		}
		break;
	case 1:
		if (row < m_data.size())
		{
			variant = static_cast<long>(m_data[row]);
		}
		else
		{
			variant = 0x40L;
		}

		break;
	default:
		break;
	}
}

template <>
bool FlagDataViewModel<ChestItem>::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

template <>
bool FlagDataViewModel<ChestItem>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	bool updated = false;
	switch (col)
	{
	case 1:
		if (row >= m_data.size())
		{
			m_data.resize(row + 1);
		}
		m_data[row] = variant.GetLong() & 0x3F;
		updated = true;
		break;
	default:
		break;
	}
	return updated;
}

template <>
void FlagDataViewModel<ChestItem>::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer(), 0, 400, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(1)), 1, 200, wxALIGN_LEFT));
}

template <>
unsigned int FlagDataViewModel<Character>::GetColumnCount() const
{
	return 3;
}

template <>
wxString FlagDataViewModel<Character>::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Index";
	case 1:
		return "Entity";
	case 2:
		return "Character";
	default:
		return "?";
	}
}

template <>
wxArrayString FlagDataViewModel<Character>::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	switch (col)
	{
	case 2:
	{
		return m_list[0];
	}
	default:
		break;
	}
	return choices;
}

template <>
wxString FlagDataViewModel<Character>::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 1:
		return "string";
	case 2:
		return "long";
	default:
		return "string";
	}
}

template <>
void FlagDataViewModel<Character>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row < m_data.size())
	{
		auto ent = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
		wxString label;
		int idx = 0;
		switch (col)
		{
		case 0:
			variant = static_cast<long>(row);
			break;
		case 1:

			for (const auto& e : ent)
			{
				if (e.GetDialogue() == row && e.HasDialogue())
				{
					if (label.empty())
					{
						label = StrPrintf("[%02d] %s (%04.1f, %04.1f, %04.1f)", idx + 1, e.GetTypeName().c_str(),
							e.GetXDbl(), e.GetYDbl(), e.GetZDbl());
					}
					else
					{
						label += ", ...";
						break;
					}
				}
				idx++;
			}
			variant = label.empty() ? wxString("???") : label;
			break;
		case 2:
			variant = static_cast<long>(m_data[row]);
			break;
		default:
			break;
		}
	}
}

template <>
bool FlagDataViewModel<Character>::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

template <>
bool FlagDataViewModel<Character>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	bool updated = false;
	if (row < m_data.size())
	{
		switch (col)
		{
		case 2:
			m_data[row] = variant.GetLong() & 0x3FF;
			updated = true;
			break;
		default:
			break;
		}
	}
	return updated;
}

template <>
void FlagDataViewModel<Character>::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer("long"), 0, 80, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewTextRenderer(), 1, 320, wxALIGN_LEFT));
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(2)), 2, 200, wxALIGN_LEFT));
}

template <>
bool FlagDataViewModel<TileSwapFlag>::AddRow(unsigned int /*row*/)
{
	if (m_data.size() < 32)
	{
		uint8_t next_idx = 0;
		while (next_idx < 32)
		{
			if (std::none_of(m_data.cbegin(), m_data.cend(), [&](const auto& e)
				{
					return e.index == next_idx;
				}))
			{
				break;
			}
				next_idx++;
		};
		m_data.insert(m_data.end(), TileSwapFlag(m_roomnum, next_idx));
		RowInserted(m_data.size() - 1);
		return true;
	}
	return false;
}

template <>
bool FlagDataViewModel<TileSwapFlag>::DeleteRow(unsigned int row)
{
	if (row < m_data.size())
	{
		m_data.erase(m_data.begin() + row);
		RowDeleted(row);
		return true;
	}
	return false;
}

template <>
unsigned int FlagDataViewModel<TileSwapFlag>::GetColumnCount() const
{
	return 3;
}

template <>
wxString FlagDataViewModel<TileSwapFlag>::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Index";
	case 1:
		return "Always";
	case 2:
		return "Flag";
	default:
		return "?";
	}
}

template <>
wxArrayString FlagDataViewModel<TileSwapFlag>::GetColumnChoices(unsigned int /*col*/) const
{
	return wxArrayString();
}

template <>
wxString FlagDataViewModel<TileSwapFlag>::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 1:
		return "bool";
	case 2:
		return "long";
	default:
		return "string";
	}
}

template <>
void FlagDataViewModel<TileSwapFlag>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row < m_data.size())
	{
		auto ent = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
		wxString label;
		switch (col)
		{
		case 0:
			variant = static_cast<long>(m_data[row].index);
			break;
		case 1:
			variant = static_cast<bool>(m_data[row].always);
			break;
		case 2:
			variant = m_data[row].always ? 0 : static_cast<long>(m_data[row].flag);
			break;
		default:
			break;
		}
	}
}

template <>
bool FlagDataViewModel<TileSwapFlag>::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

template <>
bool FlagDataViewModel<TileSwapFlag>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	if (row < m_data.size())
	{
		auto ent = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
		wxString label;
		switch (col)
		{
		case 0:
			m_data[row].index = variant.GetLong();
			return true;
		case 1:
			m_data[row].always = variant.GetBool();
			return true;
		case 2:
			if (m_data[row].always)
			{
				m_data[row].flag = variant.GetLong();
				return true;
			}
			break;
		default:
			break;
		}
	}
	return false;
}

template <>
void FlagDataViewModel<TileSwapFlag>::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewSpinRenderer(0, 30, wxDATAVIEW_CELL_EDITABLE), 0, 200, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewToggleRenderer("bool", wxDATAVIEW_CELL_ACTIVATABLE), 1, 100, wxALIGN_LEFT));
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewSpinRenderer(0, 2047, wxDATAVIEW_CELL_EDITABLE), 2, 240, wxALIGN_LEFT));
}


template <>
bool FlagDataViewModel<TreeWarpFlag>::AddRow(unsigned int /*row*/)
{
	if (m_data.size() < 1)
	{
		m_data.insert(m_data.end(), TreeWarpFlag(m_roomnum, 0, 0));
		RowInserted(m_data.size() - 1);
		return true;
	}
	return false;
}

template <>
bool FlagDataViewModel<TreeWarpFlag>::DeleteRow(unsigned int row)
{
	if (row < m_data.size())
	{
		m_data.erase(m_data.begin() + row);
		RowDeleted(row);
		return true;
	}
	return false;
}

template <>
unsigned int FlagDataViewModel<TreeWarpFlag>::GetColumnCount() const
{
	return 3;
}

template <>
wxString FlagDataViewModel<TreeWarpFlag>::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Tree 1";
	case 1:
		return "Tree 2";
	case 2:
		return "Flag";
	default:
		return "?";
	}
}

template <>
wxArrayString FlagDataViewModel<TreeWarpFlag>::GetColumnChoices(unsigned int /*col*/) const
{
	return m_list[0];
}

template <>
wxString FlagDataViewModel<TreeWarpFlag>::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 1:
		return "long";
	case 2:
		return "long";
	default:
		return "string";
	}
}

template <>
void FlagDataViewModel<TreeWarpFlag>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row < m_data.size())
	{
		auto ent = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
		wxString label;
		switch (col)
		{
		case 0:
			variant = static_cast<long>(m_roomnum);
			break;
		case 1:
			variant = static_cast<long>(m_data[row].room2);
			break;
		case 2:
			variant = static_cast<long>(m_data[row].flag);
			break;
		default:
			break;
		}
	}
}

template <>
bool FlagDataViewModel<TreeWarpFlag>::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

template <>
bool FlagDataViewModel<TreeWarpFlag>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	if (row < m_data.size())
	{
		switch (col)
		{
		case 1:
			m_data[row].room2 = variant.GetLong();
			return true;
		case 2:
			m_data[row].flag = variant.GetLong();
			return true;
		default:
			break;
		}
	}
	return false;
}

template <>
void FlagDataViewModel<TreeWarpFlag>::InitControl(wxDataViewCtrl* ctrl) const
{
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(0), wxDATAVIEW_CELL_INERT), 0, 200, wxALIGN_LEFT));
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(1)), 1, 200, wxALIGN_LEFT));
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewSpinRenderer(0, 2047, wxDATAVIEW_CELL_EDITABLE), 2, 200, wxALIGN_LEFT));
}

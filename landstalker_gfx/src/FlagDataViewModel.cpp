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
		for (int i = 0; i < 15; ++i)
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
bool FlagDataViewModel<EntityFlag>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const
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
		for (int i = 0; i < 15; ++i)
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
bool FlagDataViewModel<OneTimeEventFlag>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const
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

wxString RoomClearFlagViewModel::GetColumnHeader(unsigned int col) const
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

unsigned int RoomClearFlagViewModel::GetColumnCount() const
{
	return 2;
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
		int tree = 0;
		bool found = false;
		switch (col)
		{
		case 0:
			for (int i = 0; i < 15; ++i)
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
bool FlagDataViewModel<SacredTreeFlag>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const
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
		for (int i = 0; i < m_gd->GetRoomData()->GetRoomCount(); ++i)
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
bool FlagDataViewModel<WarpList::Transition>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const
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

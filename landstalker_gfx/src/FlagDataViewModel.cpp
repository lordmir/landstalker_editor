#include <FlagDataViewModel.h>

template <>
void FlagDataViewModel<EntityVisibilityFlags>::InitData()
{
	m_data = m_gd->GetSpriteData()->GetEntityVisibilityFlagsForRoom(m_roomnum);
}

template <>
void FlagDataViewModel<EntityVisibilityFlags>::CommitData()
{
	m_gd->GetSpriteData()->SetEntityVisibilityFlagsForRoom(m_roomnum, m_data);
}

template <>
unsigned int FlagDataViewModel<EntityVisibilityFlags>::GetColumnCount() const
{
	return 3;
}

template <>
wxString FlagDataViewModel<EntityVisibilityFlags>::GetColumnHeader(unsigned int col) const
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
wxArrayString FlagDataViewModel<EntityVisibilityFlags>::GetColumnChoices(unsigned int col) const
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
wxString FlagDataViewModel<EntityVisibilityFlags>::GetColumnType(unsigned int col) const
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
void FlagDataViewModel<EntityVisibilityFlags>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
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
bool FlagDataViewModel<EntityVisibilityFlags>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const
{
	return false;
}

template <>
bool FlagDataViewModel<EntityVisibilityFlags>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
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

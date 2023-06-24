#include "include\FlagDataViewModel.h"

EntityVisiblityFlagDataViewModel::EntityVisiblityFlagDataViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
	: wxDataViewVirtualListModel(),
	  m_roomnum(roomnum),
	  m_gd(gd)
{
	m_data = gd->GetSpriteData()->GetEntityVisibilityFlagsForRoom(roomnum);
	Reset(m_data.size());
}

unsigned int EntityVisiblityFlagDataViewModel::GetColumnCount() const
{
	return 3;
}

unsigned int EntityVisiblityFlagDataViewModel::GetRowCount() const
{
	return m_data.size();
}

std::string EntityVisiblityFlagDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Entity";
	case 1:
		return "Flag";
	case 2:
		return "Set";
	default:
		return "?";
	}
}

wxArrayString EntityVisiblityFlagDataViewModel::GetColumnChoices(unsigned int col) const
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

wxString EntityVisiblityFlagDataViewModel::GetColumnType(unsigned int col) const
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

void EntityVisiblityFlagDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
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

bool EntityVisiblityFlagDataViewModel::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const
{
	return false;
}

bool EntityVisiblityFlagDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
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
	if (updated)
	{
		m_gd->GetSpriteData()->SetEntityVisibilityFlagsForRoom(m_roomnum, m_data);
	}
	return updated;
}

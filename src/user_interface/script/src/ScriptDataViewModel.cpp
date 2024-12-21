#include <user_interface/script/include/ScriptDataViewModel.h>
#include <user_interface/script/include/ScriptDataViewRenderer.h>

ScriptDataViewModel::ScriptDataViewModel(std::shared_ptr<GameData> gd)
  : BaseDataViewModel(),
	m_gd(gd)
{
	Initialise();
}

void ScriptDataViewModel::Initialise()
{
	if (m_gd)
	{
		m_script = m_gd->GetScriptData()->GetScript();
	}
	else
	{
		m_script.reset();
	}
	Reset(GetRowCount());
}

void ScriptDataViewModel::CommitData()
{
}

unsigned int ScriptDataViewModel::GetColumnCount() const
{
	return 1;
}

unsigned int ScriptDataViewModel::GetRowCount() const
{
	return m_script->GetScriptLineCount();
}

wxString ScriptDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Index";
	case 1:
		return "Clear";
	case 2:
		return "End";
	case 3:
		return "Value";
	default:
		return "???";
	}
}

wxArrayString ScriptDataViewModel::GetColumnChoices(unsigned int /*col*/) const
{
	return wxArrayString();
}

wxString ScriptDataViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 2:
		return "long";
	case 3:
		return "long";
	case 4:
		return "long";
	default:
		return "???";
	}
}

void ScriptDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row >= m_script->GetScriptLineCount())
	{
		return;
	}
	switch (col)
	{
	case 0:
	case 3:
		variant = static_cast<long>(row);
		break;
	case 1:
		if (row < m_script->GetScriptLineCount())
		{
			variant = m_script->GetScriptLineClear(row);
		}
		break;
	case 2:
		if (row < m_script->GetScriptLineCount())
		{
			variant = m_script->GetScriptLineEnd(row);
		}
		break;
	default:
		break;
	}
}

bool ScriptDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool ScriptDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	if (row >= m_script->GetScriptLineCount())
	{
		return false;
	}
	auto line = ScriptTableEntry::MakeEntry(m_script->GetScriptLine(row).GetType());
	switch(col)
	{
	case 1:
	{
		m_script->SetScriptLineClear(row, variant.GetBool());
		return true;
	}
	case 2:
	{
		m_script->SetScriptLineEnd(row, variant.GetBool());
		return true;
	}
	case 3:
	{
		bool clear = m_script->GetScriptLineClear(row);
		bool end = m_script->GetScriptLineEnd(row);
		m_script->SetScriptLine(row, ScriptTableEntry::FromBytes(static_cast<uint16_t>(variant.GetLong())));
		m_script->SetScriptLineClear(row, clear);
		m_script->SetScriptLineEnd(row, end);
		return true;
	}
	default:
		break;
	}
	return false;
}

bool ScriptDataViewModel::DeleteRow(unsigned int row)
{
	if (row < m_script->GetScriptLineCount())
	{
		m_script->DeleteScriptLine(row);
		RowDeleted(row);
		return true;
	}
	return false;
}

bool ScriptDataViewModel::AddRow(unsigned int row)
{
	if (row <= m_script->GetScriptLineCount())
	{
		m_script->AddScriptLineBefore(row, ScriptTableEntry::MakeEntry(ScriptTableEntryType::STRING));
		RowInserted(row);
		return true;
	}
	return false;
}

bool ScriptDataViewModel::SwapRows(unsigned int r1, unsigned int r2)
{
	if (r1 < m_script->GetScriptLineCount() && r2 < m_script->GetScriptLineCount() && r1 != r2)
	{
		m_script->SwapScriptLines(r1, r2);
		RowChanged(r1);
		RowChanged(r2);
		return true;
	}
	return false;
}

void ScriptDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	// Index
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer("long"), 0, 64, wxALIGN_LEFT));
	// Clear
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewToggleRenderer("bool", wxDATAVIEW_CELL_ACTIVATABLE, wxALIGN_CENTER_HORIZONTAL), 1, 64, wxALIGN_LEFT));
	// End
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewToggleRenderer("bool", wxDATAVIEW_CELL_ACTIVATABLE, wxALIGN_CENTER_HORIZONTAL), 2, 64, wxALIGN_LEFT));
	// Parameters
	ctrl->InsertColumn(3, new wxDataViewColumn(this->GetColumnHeader(3),
		new ScriptDataViewRenderer(wxDATAVIEW_CELL_EDITABLE, m_gd), 3, -1, wxALIGN_LEFT));
}

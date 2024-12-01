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
		return "Entry";
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
	if (col == 1)
	{
		return "string";
	}
	return "long";
}

void ScriptDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (col == 0)
	{
		variant = static_cast<long>(row);
	}
	else if (row < m_script->GetScriptLineCount())
	{
		variant = static_cast<long>(m_script->GetScriptLine(row).ToBytes());
	}
}

bool ScriptDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool ScriptDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	if (col > 0 && row < m_script->GetScriptLineCount())
	{
		if (col == 1)
		{
			m_script->SetScriptLine(row, ScriptTableEntry::FromBytes(static_cast<uint16_t>(variant.GetLong())));
			return true;
		}
	}
	return false;
}

bool ScriptDataViewModel::DeleteRow(unsigned int /*row*/)
{
	return false;
}

bool ScriptDataViewModel::AddRow(unsigned int /*row*/)
{
	return false;
}

bool ScriptDataViewModel::SwapRows(unsigned int /*r1*/, unsigned int /*r2*/)
{
	return false;
}

void ScriptDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	// Index
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer("long"), 0, 64, wxALIGN_LEFT));
	// Parameters
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new ScriptDataViewRenderer(wxDATAVIEW_CELL_EDITABLE, m_gd), 1, -1, wxALIGN_LEFT));
}

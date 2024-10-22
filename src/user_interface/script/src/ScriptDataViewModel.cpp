#include <user_interface/script/include/ScriptDataViewModel.h>

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
	return 2;
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
		return "Type";
	case 2:
		return "Parameters";
	default:
		return "???";
	}
}

wxArrayString ScriptDataViewModel::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	if (col == 1)
	{
		choices.Add("Invalid");
		choices.Add("String");
		choices.Add("Load Item");
		choices.Add("Load Global Character");
		choices.Add("Load Number");
		choices.Add("Set Flag");
		choices.Add("Give Item to Player");
		choices.Add("Give Money to Player");
		choices.Add("Play BGM");
		choices.Add("Set Speaker (Normal)");
		choices.Add("Set Speaker (Global)");
		choices.Add("Play Cutscene");
	}
	return choices;
}

wxString ScriptDataViewModel::GetColumnType(unsigned int col) const
{
	if (col == 2)
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
		switch (col)
		{
		case 1:
			variant = static_cast<long>(m_script->GetScriptLine(row).GetType());
			break;
		case 2:
			variant = wxString(m_script->GetScriptLine(row).ToString(m_gd));
			break;
		default:
			break;
		}
	}
}

bool ScriptDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int col, wxDataViewItemAttr& attr) const
{
	return false;
}

bool ScriptDataViewModel::SetValueByRow(const wxVariant& /*variant*/, unsigned int /*row*/, unsigned int /*col*/)
{
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
	// Type
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewChoiceByIndexRenderer(this->GetColumnChoices(1)), 1, 200, wxALIGN_LEFT));
	// Parameters
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewTextRenderer(), 2, -1, wxALIGN_LEFT));
}

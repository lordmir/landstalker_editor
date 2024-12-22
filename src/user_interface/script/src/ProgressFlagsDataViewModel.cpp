#include <user_interface/script/include/ProgressFlagsDataViewModel.h>

ProgressFlagsDataViewModel::ProgressFlagsDataViewModel(std::shared_ptr<GameData> gd)
	: BaseDataViewModel(),
	  m_gd(gd)
{
	Initialise();
}

void ProgressFlagsDataViewModel::Initialise()
{
	if (m_gd && m_gd->GetScriptData()->HasTables())
	{
		m_flags = ProgressFlags::GetFlags(*m_gd->GetScriptData()->GetProgressFlagsFuncs());
	}
	else
	{
		m_flags.clear();
	}
	Reset(GetRowCount());
}

void ProgressFlagsDataViewModel::CommitData()
{
	if (m_gd && m_gd->GetScriptData()->HasTables())
	{
		m_gd->GetScriptData()->SetProgressFlagsFuncs(ProgressFlags::MakeAsm(m_flags));
	}
}

unsigned int ProgressFlagsDataViewModel::GetColumnCount() const
{
	return 3;
}

unsigned int ProgressFlagsDataViewModel::GetRowCount() const
{
	return m_flags.size();
}

wxString ProgressFlagsDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return _("Quest");
	case 1:
		return _("Progress");
	case 2:
		return _("Flag");
	default:
		return "???";
	}
}

wxArrayString ProgressFlagsDataViewModel::GetColumnChoices(unsigned int /*col*/) const
{
	return wxArrayString();
}

wxString ProgressFlagsDataViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 1:
		return "long";
	case 2:
		return "string";
	default:
		return "???";
	}
}

void ProgressFlagsDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row >= m_flags.size() || !m_gd)
	{
		return;
	}
	const auto& elem = *(std::next(m_flags.begin(), row));
	switch (col)
	{
	case 0:
		variant = static_cast<long>(elem.first.quest);
		break;
	case 1:
		variant = static_cast<long>(elem.first.progress);
		break;
	case 2:
		variant = Hex(elem.second) + " " + ScriptData::GetFlagDisplayName(elem.second);
		break;
	default:
		break;
	}
}

bool ProgressFlagsDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool ProgressFlagsDataViewModel::SetValueByRow(const wxVariant& /*variant*/, unsigned int /*row*/, unsigned int /*col*/)
{
	CommitData();
	return false;
}

bool ProgressFlagsDataViewModel::DeleteRow(unsigned int /*row*/)
{
	return false;
}

bool ProgressFlagsDataViewModel::AddRow(unsigned int /*row*/)
{
	return false;
}

bool ProgressFlagsDataViewModel::SwapRows(unsigned int /*r1*/, unsigned int /*r2*/)
{
	return false;
}

void ProgressFlagsDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	// Quest
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer(GetColumnType(0)), 0, 80, wxALIGN_LEFT));
	// Progress
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewTextRenderer(GetColumnType(1)), 1, 80, wxALIGN_LEFT));
	// Flag
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new wxDataViewTextRenderer(GetColumnType(2), wxDATAVIEW_CELL_EDITABLE), 2, -1, wxALIGN_LEFT));
}

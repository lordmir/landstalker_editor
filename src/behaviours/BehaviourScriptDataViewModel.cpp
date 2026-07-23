#include <behaviours/BehaviourScriptDataViewModel.h>
#include <behaviours/BehaviourCommandDataViewRenderer.h>

BehaviourScriptDataViewModel::BehaviourScriptDataViewModel(std::shared_ptr<Landstalker::GameData> gd, int script_id)
  : BaseDataViewModel(),
	m_gd(gd),
	m_script_id(script_id)
{
	Initialise();
}

void BehaviourScriptDataViewModel::Initialise()
{
	m_commands.clear();
	if (m_gd && m_script_id >= 0)
	{
		m_commands = m_gd->GetSpriteData()->GetScript(m_script_id).second;
	}
	Reset(GetRowCount());
}

void BehaviourScriptDataViewModel::CommitData()
{
	if (m_gd && m_script_id >= 0)
	{
		m_gd->GetSpriteData()->SetScript(m_script_id, m_commands);
	}
}

unsigned int BehaviourScriptDataViewModel::GetColumnCount() const
{
	return 2;
}

unsigned int BehaviourScriptDataViewModel::GetRowCount() const
{
	return static_cast<unsigned int>(m_commands.size());
}

wxString BehaviourScriptDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Index";
	case 1:
		return "Command";
	default:
		return "???";
	}
}

wxArrayString BehaviourScriptDataViewModel::GetColumnChoices(unsigned int /*col*/) const
{
	return wxArrayString();
}

wxString BehaviourScriptDataViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "long";
	case 1:
		return "string";
	default:
		return "???";
	}
}

void BehaviourScriptDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row >= GetRowCount())
	{
		return;
	}
	switch (col)
	{
	case 0:
		// 1-based, deliberately: this is the same instruction number GotoInstruction's Command
		// parameter targets (see Behaviours::Unpack()'s labels map) and the "# <Command #N>"
		// numbering the YAML export uses, so the two stay directly cross-referenceable.
		variant = static_cast<long>(row + 1);
		break;
	case 1:
		// Commands travel through the variant pipeline in their single-command YAML form -
		// see BehaviourCommand::Serialise(). FromUTF8, not the locale-dependent wxString(std::
		// string) ctor: the YAML embeds label names (flag/sound/cutscene comments) that can be
		// non-ASCII UTF-8, which the locale codec would garble or drop entirely on Windows.
		variant = wxString::FromUTF8(BehaviourCommand::Serialise(m_commands[row]));
		break;
	default:
		break;
	}
}

bool BehaviourScriptDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool BehaviourScriptDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	if (row >= GetRowCount())
	{
		return false;
	}
	switch (col)
	{
	case 1:
	{
		const auto cmd = BehaviourCommand::Deserialise(std::string(variant.GetString().utf8_str()));
		if (!cmd)
		{
			return false;
		}
		m_commands[row] = *cmd;
		CommitData();
		return true;
	}
	default:
		break;
	}
	return false;
}

bool BehaviourScriptDataViewModel::DeleteRow(unsigned int row)
{
	if (row < GetRowCount())
	{
		m_commands.erase(m_commands.begin() + row);
		CommitData();
		RowDeleted(row);
		return true;
	}
	return false;
}

bool BehaviourScriptDataViewModel::AddRow(unsigned int row)
{
	if (row <= GetRowCount())
	{
		m_commands.insert(m_commands.begin() + row,
			BehaviourCommand::MakeDefault(Landstalker::Behaviours::CommandType::PAUSE));
		CommitData();
		RowInserted(row);
		return true;
	}
	return false;
}

bool BehaviourScriptDataViewModel::SwapRows(unsigned int r1, unsigned int r2)
{
	if (r1 < GetRowCount() && r2 < GetRowCount() && r1 != r2)
	{
		std::swap(m_commands[r1], m_commands[r2]);
		CommitData();
		RowChanged(r1);
		RowChanged(r2);
		return true;
	}
	return false;
}

void BehaviourScriptDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	// Index
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer("long"), 0, 64, wxALIGN_LEFT));
	// Command - the host control (BehaviourScriptEditorCtrl) resizes this column to fill the
	// remaining width on every layout, so it needs no forced minimum here (a large SetMinWidth()
	// would force a horizontal scrollbar in the narrower Entity Properties tab).
	wxDataViewColumn* command_column = new wxDataViewColumn(this->GetColumnHeader(1),
		new BehaviourCommandDataViewRenderer(wxDATAVIEW_CELL_EDITABLE,
			[this]() { return GetRowCount(); }), 1, -1, wxALIGN_LEFT);
	ctrl->InsertColumn(1, command_column);
}

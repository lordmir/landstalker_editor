#include <user_interface/script/include/ScriptTableDataViewModel.h>
#include <user_interface/script/include/DataViewScriptActionRenderer.h>

static const std::array<std::string, 5> SHOP_ACTIONS{ "On Enter", "On Exit", "On Pick Up", "On Pay", "On Steal" };
static const std::array<std::string, 3> ITEM_ACTIONS{ "On Pick Up", "On Pay", "On Steal" };

ScriptTableDataViewModel::ScriptTableDataViewModel(std::shared_ptr<GameData> gd)
	: BaseDataViewModel(),
	  m_gd(gd),
	  m_mode(Mode::CHARACTER),
	  m_index(0)
{
	Initialise();
}

void ScriptTableDataViewModel::Initialise()
{
	if (m_gd && m_gd->GetScriptData()->HasTables())
	{
		m_cutscene_script = m_gd->GetScriptData()->GetCutsceneTable();
		m_character_script = m_gd->GetScriptData()->GetCharTable();
		m_shop_script = m_gd->GetScriptData()->GetShopTable();
		m_item_script = m_gd->GetScriptData()->GetItemTable();
	}
	else
	{
		m_cutscene_script.reset();
		m_character_script.reset();
		m_shop_script.reset();
		m_item_script.reset();
	}
	m_mode = Mode::CHARACTER;
	m_index = 0;
	Reset(GetRowCount());
}

void ScriptTableDataViewModel::SetMode(const Mode& mode, unsigned int index)
{
	m_mode = mode;
	m_index = index;
	Reset(GetRowCount());
}

void ScriptTableDataViewModel::CommitData()
{
}

unsigned int ScriptTableDataViewModel::GetColumnCount() const
{
	return 4;
}

unsigned int ScriptTableDataViewModel::GetRowCount() const
{
	switch (m_mode)
	{
	case Mode::CUTSCENE:
		return m_cutscene_script ? m_cutscene_script->size() : 0;
	case Mode::CHARACTER:
		return m_character_script ? m_character_script->size() : 0;
	case Mode::SHOP:
		return m_shop_script && m_index < m_shop_script->size() ? m_shop_script->at(m_index).actions.size() : 0;
	case Mode::ITEM:
		return m_item_script && m_index < m_item_script->size() ? m_item_script->at(m_index).actions.size() : 0;
	default:
		return 0;
	}
}

wxString ScriptTableDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		switch (m_mode)
		{
		case Mode::CHARACTER:
			return _("Character");
		case Mode::CUTSCENE:
			return _("Cutscene");
		default:
			return _("Event");
		}
	case 1:
		return _("Type");
	case 2:
		return _("Value");
	case 3:
		return _("Comment");
	default:
		return "???";
	}
}

wxArrayString ScriptTableDataViewModel::GetColumnChoices(unsigned int col) const
{
	wxArrayString choices;
	switch (col)
	{
	case 1:
		choices.Add("Script");
		choices.Add("Jump");
		break;
	default:
		break;
	}
	return choices;
}

wxString ScriptTableDataViewModel::GetColumnType(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "string";
	case 1:
		return "long";
	case 2:
		return "string";
	case 3:
		return "string";
	default:
		return "string";
	}
}

void ScriptTableDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	auto cell = GetCell(row);
	if (!cell || !m_gd)
	{
		return;
	}
	switch(col)
	{
	case 0:
		switch (m_mode)
		{
		case Mode::CUTSCENE:
			variant = _(ScriptData::GetCutsceneDisplayName(row));
			break;
		case Mode::CHARACTER:
			variant = _(StrWPrintf("[%03d] %ls",row, m_gd->GetStringData()->GetCharacterDisplayName(row).c_str()));
			break;
		case Mode::SHOP:
			variant = _(SHOP_ACTIONS.at(row));
			break;
		case Mode::ITEM:
			variant = row < ITEM_ACTIONS.size() ? _(ITEM_ACTIONS.at(row)) : _(StrPrintf("Custom Event %d", row));
			break;
		default:
			variant = _(std::to_string(row));
			break;
		}
		break;
	case 1:
		variant = std::visit([](const auto& arg)
			{
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, uint16_t>)
				{
					return 0L;
				}
				else if constexpr (std::is_same_v<T, std::string>)
				{
					return 1L;
				}
			}, *cell);
		break;
	case 2:
		variant = ScriptTable::ToString(*cell);
		break;
	case 3:
		variant = ScriptTable::GetActionSummary(*cell, m_gd);
		break;
	}
}

bool ScriptTableDataViewModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool ScriptTableDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	auto cell = GetCell(row);
	if (!cell)
	{
		return false;
	}
	switch (col)
	{
	case 1:
		if (variant.GetLong() == 0)
		{
			*cell = 0_u16;
		}
		else
		{
			*cell = "FUNC";
		}
		break;
	case 2:
		*cell = ScriptTable::FromString(variant.GetString().ToStdString());
		break;
	}
	return false;
}

bool ScriptTableDataViewModel::DeleteRow(unsigned int row)
{
	auto table = GetTable();
	if (table && row < table->size())
	{
		table->erase(table->begin() + row);
		RowDeleted(row);
		return true;
	}
	return false;
}

bool ScriptTableDataViewModel::AddRow(unsigned int row)
{
	auto table = GetTable();
	if (table && row <= table->size())
	{
		table->insert(table->begin() + row, 0_u16);
		RowInserted(row);
		return true;
	}
	return false;
}

bool ScriptTableDataViewModel::SwapRows(unsigned int r1, unsigned int r2)
{
	if (m_mode == Mode::SHOP && m_shop_script && m_index < m_shop_script->size())
	{
		if (r1 < m_shop_script->at(m_index).actions.size() && r2 < m_shop_script->at(m_index).actions.size() && r1 != r2)
		{
			std::iter_swap(m_shop_script->at(m_index).actions.begin() + r1, m_shop_script->at(m_index).actions.begin() + r2);
			RowChanged(r1);
			RowChanged(r2);
			return true;
		}
	}
	else
	{
		auto table = GetTable();
		if (r1 < table->size() && r2 < table->size() && r1 != r2)
		{
			std::iter_swap(table->begin() + r1, table->begin() + r2);
			RowChanged(r1);
			RowChanged(r2);
			return true;
		}
	}
	return false;
}

void ScriptTableDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
	// Index
	ctrl->InsertColumn(0, new wxDataViewColumn(this->GetColumnHeader(0),
		new wxDataViewTextRenderer(GetColumnType(0)), 0, 120, wxALIGN_LEFT));
	// Type
	ctrl->InsertColumn(1, new wxDataViewColumn(this->GetColumnHeader(1),
		new wxDataViewChoiceByIndexRenderer(GetColumnChoices(1)), 1, 100, wxALIGN_LEFT));
	// Value
	ctrl->InsertColumn(2, new wxDataViewColumn(this->GetColumnHeader(2),
		new DataViewScriptActionRenderer(wxDATAVIEW_CELL_EDITABLE, m_gd), 2, -1, wxALIGN_LEFT));
}

ScriptTable::Action ScriptTableDataViewModel::GetAction(unsigned int row) const
{
	auto cell = GetCell(row);
	if (cell)
	{
		return *GetCell(row);
	}
	else
	{
		return 0xFFFF_u16;
	}
}

std::vector<ScriptTable::Action>* ScriptTableDataViewModel::GetTable()
{
	switch (m_mode)
	{
	case Mode::CUTSCENE:
		if (m_cutscene_script)
		{
			return &(*m_cutscene_script);
		}
		break;
	case Mode::CHARACTER:
		if (m_character_script)
		{
			return &(*m_character_script);
		}
		break;
	case Mode::ITEM:
		if (m_item_script && m_index < m_item_script->size())
		{
			return &(m_item_script->at(m_index).actions);
		}
		break;
	case Mode::SHOP:
	default:
		break;
	}
	return nullptr;
}

const std::vector<ScriptTable::Action>* ScriptTableDataViewModel::GetTable() const
{
	switch (m_mode)
	{
	case Mode::CUTSCENE:
		if (m_cutscene_script)
		{
			return &(*m_cutscene_script);
		}
		break;
	case Mode::CHARACTER:
		if (m_character_script)
		{
			return &(*m_character_script);
		}
		break;
	case Mode::ITEM:
		if (m_item_script && m_index < m_item_script->size())
		{
			return &(m_item_script->at(m_index).actions);
		}
		break;
	case Mode::SHOP:
	default:
		break;
	}
	return nullptr;
}

const ScriptTable::Action* ScriptTableDataViewModel::GetCell(unsigned int row) const
{
	auto table = GetTable();
	if (m_mode == Mode::SHOP && m_shop_script && m_index < m_shop_script->size()
		&& row < m_shop_script->at(m_index).actions.size())
	{
		return &m_shop_script->at(m_index).actions.at(row);
	}
	else if (table && row < table->size())
	{
		return &table->at(row);
	}
	return nullptr;
}

ScriptTable::Action* ScriptTableDataViewModel::GetCell(unsigned int row)
{
	auto table = GetTable();
	if (m_mode == Mode::SHOP && m_shop_script && m_index < m_shop_script->size()
		&& row < m_shop_script->at(m_index).actions.size())
	{
		return &m_shop_script->at(m_index).actions.at(row);
	}
	else if (table && row < table->size())
	{
		return &table->at(row);
	}
	return nullptr;
}

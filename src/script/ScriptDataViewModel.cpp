#include <script/ScriptDataViewModel.h>
#include <script/ScriptDataViewRenderer.h>

#include <algorithm>

ScriptDataViewModel::ScriptDataViewModel(std::shared_ptr<Landstalker::GameData> gd)
  : BaseDataViewModel(),
	m_gd(gd)
{
	Initialise();
}

ScriptDataViewModel::ScriptDataViewModel(std::shared_ptr<Landstalker::GameData> gd, unsigned int start, unsigned int count)
  : BaseDataViewModel(),
	m_gd(gd),
	m_start(start),
	m_window_count(static_cast<int>(count))
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
	const unsigned int total = m_script->GetScriptLineCount();
	if (m_window_count < 0)
	{
		return total;
	}
	if (m_start >= total)
	{
		return 0;
	}
	// m_window_count is kept in step by AddRow()/DeleteRow(); the clamp is a safety net for
	// script mutations made elsewhere while this windowed model is alive.
	return std::min(static_cast<unsigned int>(m_window_count), total - m_start);
}

wxString ScriptDataViewModel::GetColumnHeader(unsigned int col) const
{
	switch (col)
	{
	case 0:
		return "Index";
	case 1:
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
	case 1:
		return "long";
	default:
		return "???";
	}
}

void ScriptDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (row >= GetRowCount())
	{
		return;
	}
	switch (col)
	{
	case 0:
	case 1:
		// The renderer/editor resolve the real entry straight from GameData using this absolute
		// script line index - see ScriptDataViewRenderer::SetValue()/CreateEditorCtrl(). It's also
		// what the Index column displays, so a windowed model shows true script IDs.
		variant = static_cast<long>(ToScriptLine(row));
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
	if (row >= GetRowCount())
	{
		return false;
	}
	switch(col)
	{
	case 1:
	{
		// Clear/End are encoded directly in the entry's own ToBytes()/FromBytes() (see
		// ScriptTableEntry.cpp), so this round-trip already carries them correctly - no separate
		// preserve step needed (every editor either owns them for real, i.e. STRING's checkboxes,
		// or passes through whatever the entry already had, see ScriptEntryEditorCtrl::m_clear/m_end).
		m_script->SetScriptLine(ToScriptLine(row), Landstalker::ScriptTableEntry::FromBytes(static_cast<uint16_t>(variant.GetLong())));
		return true;
	}
	default:
		break;
	}
	return false;
}

bool ScriptDataViewModel::ChangeRowType(unsigned int row, Landstalker::ScriptTableEntryType new_type)
{
	if (row >= GetRowCount())
	{
		return false;
	}
	const auto& old_entry = m_script->GetScriptLine(ToScriptLine(row));
	auto new_entry = Landstalker::ScriptTableEntry::MakeEntry(new_type);
	new_entry->SetClear(old_entry.GetClear());
	new_entry->SetEnd(old_entry.GetEnd());
	new_entry->SetData(old_entry.GetData());
	m_script->SetScriptLine(ToScriptLine(row), std::move(new_entry));
	RowChanged(row);
	return true;
}

bool ScriptDataViewModel::DeleteRow(unsigned int row)
{
	if (row < GetRowCount())
	{
		m_script->DeleteScriptLine(ToScriptLine(row));
		if (m_window_count > 0)
		{
			--m_window_count;
		}
		RowDeleted(row);
		return true;
	}
	return false;
}

bool ScriptDataViewModel::AddRow(unsigned int row)
{
	return AddRow(row, Landstalker::ScriptTableEntryType::STRING);
}

bool ScriptDataViewModel::AddRow(unsigned int row, Landstalker::ScriptTableEntryType type)
{
	if (row <= GetRowCount())
	{
		m_script->AddScriptLineBefore(ToScriptLine(row), Landstalker::ScriptTableEntry::MakeEntry(type));
		if (m_window_count >= 0)
		{
			++m_window_count;
		}
		RowInserted(row);
		return true;
	}
	return false;
}

bool ScriptDataViewModel::SwapRows(unsigned int r1, unsigned int r2)
{
	if (r1 < GetRowCount() && r2 < GetRowCount() && r1 != r2)
	{
		m_script->SwapScriptLines(ToScriptLine(r1), ToScriptLine(r2));
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
	// Value (includes Clear/End, folded into the STRING editor/renderer specifically)
	wxDataViewColumn* value_column = new wxDataViewColumn(this->GetColumnHeader(1),
		new ScriptDataViewRenderer(wxDATAVIEW_CELL_EDITABLE, m_gd), 1, -1, wxALIGN_LEFT);
	// ScriptDataViewRenderer::GetSize() intentionally reports a modest per-type width (needed to
	// keep the floating editor correctly positioned - see its comment), which would otherwise also
	// pull in this auto-fill (-1) column's own grow-only natural width. Pin an explicit floor here,
	// independent of that, so normal (non-editing) display stays comfortably wide regardless.
	value_column->SetMinWidth(800);
	ctrl->InsertColumn(1, value_column);
}

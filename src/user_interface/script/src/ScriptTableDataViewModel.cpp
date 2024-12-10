#include <user_interface/script/include/ScriptTableDataViewModel.h>

ScriptTableDataViewModel::ScriptTableDataViewModel(std::shared_ptr<GameData> gd)
{
}

void ScriptTableDataViewModel::Initialise()
{
}

void ScriptTableDataViewModel::SetMode(const Mode& mode, int index)
{
}

void ScriptTableDataViewModel::CommitData()
{
}

unsigned int ScriptTableDataViewModel::GetColumnCount() const
{
	return 0;
}

unsigned int ScriptTableDataViewModel::GetRowCount() const
{
	return 0;
}

wxString ScriptTableDataViewModel::GetColumnHeader(unsigned int col) const
{
	return wxString();
}

wxArrayString ScriptTableDataViewModel::GetColumnChoices(unsigned int col) const
{
	return wxArrayString();
}

wxString ScriptTableDataViewModel::GetColumnType(unsigned int col) const
{
	return wxString();
}

void ScriptTableDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
}

bool ScriptTableDataViewModel::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const
{
	return false;
}

bool ScriptTableDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
	return false;
}

bool ScriptTableDataViewModel::DeleteRow(unsigned int row)
{
	return false;
}

bool ScriptTableDataViewModel::AddRow(unsigned int row)
{
	return false;
}

bool ScriptTableDataViewModel::SwapRows(unsigned int r1, unsigned int r2)
{
	return false;
}

void ScriptTableDataViewModel::InitControl(wxDataViewCtrl* ctrl) const
{
}

#include "DataViewCtrlPaletteModel.h"

DataViewCtrlPaletteModel::DataViewCtrlPaletteModel(const std::vector<std::shared_ptr<PaletteEntry>>& palettes)
	: wxDataViewVirtualListModel(palettes.size()),
	  m_palettes(palettes)
{
}

DataViewCtrlPaletteModel::~DataViewCtrlPaletteModel()
{
}

wxString DataViewCtrlPaletteModel::GetColumnType(unsigned int col) const
{
	if (col == static_cast<int>(Col::NAME))
	{
		return "string";
	}
	else if (col == static_cast<int>(Col::PALETTE))
	{
		return "void*";
	}
	else
	{
		return "";
	}
}

int DataViewCtrlPaletteModel::GetColumnMaxElements() const
{
	int max = 0;
	for (const auto& elem : m_palettes)
	{
		int v = elem->GetData()->GetSize();
		if (v > max)
		{
			max = v;
		}
	}
	return max;
}

void DataViewCtrlPaletteModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
	if (col == static_cast<int>(Col::NAME))
	{
		if (row < m_palettes.size())
		{
			variant = m_palettes[row]->GetName();
		}
	}
	else if (col == static_cast<int>(Col::PALETTE))
	{
		if (row < m_palettes.size())
		{
			variant = reinterpret_cast<void*>(m_palettes[row].get());
		}
	}
}

bool DataViewCtrlPaletteModel::GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const
{
	return false;
}

bool DataViewCtrlPaletteModel::SetValueByRow(const wxVariant& /*variant*/, unsigned int row, unsigned int col)
{
	if (col == static_cast<int>(Col::NAME))
	{
		return false;
	}
	else if (col == static_cast<int>(Col::PALETTE))
	{
		if (row < m_palettes.size())
		{
			return true;
		}
	}
	return false;
}

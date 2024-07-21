#include <user_interface/text/include/StringDataViewModel.h>

StringDataViewModel::StringDataViewModel(StringData::Type type, std::shared_ptr<StringData> sd)
	: wxDataViewVirtualListModel(sd->GetStringCount(type)),
      m_type(type),
      m_sd(sd)
{
}

unsigned int StringDataViewModel::GetColumnCount() const
{
    if (m_type == StringData::Type::INTRO)
    {
        return 8;
    }
    else if (m_type == StringData::Type::END_CREDITS)
    {
        return 4;
    }
    return 2;
}

unsigned int StringDataViewModel::GetRowCount() const
{
    return m_sd->GetStringCount(m_type);
}

std::string StringDataViewModel::GetColumnHeader(unsigned int col) const
{
    if (col == 0)
    {
        return "#";
    }
    else if (m_type == StringData::Type::INTRO)
    {
        switch (col)
        {
        case 1:
            return "Display Time";
        case 2:
            return "Line 1 X";
        case 3:
            return "Line 1 Y";
        case 4:
            return "Line 2 X";
        case 5:
            return "Line 2 Y";
        case 6:
            return "Line 1";
        default:
            return "Line 2";
        }
    }
    else if (m_type == StringData::Type::END_CREDITS)
    {
        switch (col)
        {
        case 1:
            return "Column";
        case 2:
            return "Height";
        default:
            return "String";
        }
    }
    else
    {
        return "String";
    }
}

wxString StringDataViewModel::GetColumnType(unsigned int col) const
{
    if (col == 0)
    {
        return "long";
    }
    else if (m_type == StringData::Type::INTRO && col < 6)
    {
        return "long";
    }
    else if (m_type == StringData::Type::END_CREDITS && col < 3)
    {
        return "long";
    }
    return "string";
}

void StringDataViewModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const
{
    assert(row < m_sd->GetStringCount(m_type));
    if (col == 0)
    {
        variant = static_cast<long>(row);
    }
    else if(m_type == StringData::Type::INTRO)
    {
        switch (col)
        {
        case 1:
            variant = static_cast<long>(m_sd->GetIntroString(row).GetDisplayTime());
            break;
        case 2:
            variant = static_cast<long>(m_sd->GetIntroString(row).GetLine1X());
            break;
        case 3:
            variant = static_cast<long>(m_sd->GetIntroString(row).GetLine1Y());
            break;
        case 4:
            variant = static_cast<long>(m_sd->GetIntroString(row).GetLine2X());
            break;
        case 5:
            variant = static_cast<long>(m_sd->GetIntroString(row).GetLine2Y());
            break;
        case 6:
            variant = m_sd->GetIntroString(row).GetLine(0);
            break;
        default:
            variant = m_sd->GetIntroString(row).GetLine(1);
            break;
        }
    }
    else if (m_type == StringData::Type::END_CREDITS)
    {
        switch (col)
        {
        case 1:
            variant = static_cast<long>(m_sd->GetEndCreditString(row).GetColumn());
            break;
        case 2:
            variant = static_cast<long>(m_sd->GetEndCreditString(row).GetHeight());
            break;
        default:
            variant = m_sd->GetEndCreditString(row).Str();
            break;
        }
    }
    else
    {
        variant = m_sd->GetString(m_type, row);
    }
}

bool StringDataViewModel::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const
{
    assert(row < m_sd->GetStringCount(m_type));
    if (col == 0)
    {
        attr.SetBold(true);
        return true;
    }
    if (m_type == StringData::Type::INTRO)
    {
        const auto& olds = m_sd->GetOrigIntroString(row);
        const auto& news = m_sd->GetIntroString(row);
        switch (col)
        {
        case 1:
            attr.SetColour((olds.GetDisplayTime() != news.GetDisplayTime()) ? *wxRED : *wxBLACK);
            break;
        case 2:
            attr.SetColour((olds.GetLine1X() != news.GetLine1X()) ? *wxRED : *wxBLACK);
            break;
        case 3:
            attr.SetColour((olds.GetLine1Y() != news.GetLine1Y()) ? *wxRED : *wxBLACK);
            break;
        case 4:
            attr.SetColour((olds.GetLine2X() != news.GetLine2X()) ? *wxRED : *wxBLACK);
            break;
        case 5:
            attr.SetColour((olds.GetLine2Y() != news.GetLine2Y()) ? *wxRED : *wxBLACK);
            break;
        case 6:
            attr.SetColour((olds.GetLine(0) != news.GetLine(0)) ? *wxRED : *wxBLACK);
            break;
        default:
            attr.SetColour((olds.GetLine(1) != news.GetLine(1)) ? *wxRED : *wxBLACK);
            break;
        }
    }
    else if (m_type == StringData::Type::END_CREDITS)
    {
        const auto& olds = m_sd->GetOrigEndCreditString(row);
        const auto& news = m_sd->GetEndCreditString(row);
        switch (col)
        {
        case 1:
            attr.SetColour((olds.GetColumn() != news.GetColumn()) ? *wxRED : *wxBLACK);
            break;
        case 2:
            attr.SetColour((olds.GetHeight() != news.GetHeight()) ? *wxRED : *wxBLACK);
            break;
        default:
            attr.SetColour((olds.Str() != news.Str()) ? *wxRED : *wxBLACK);
            break;
        }
    }
    else
    {
        attr.SetColour(m_sd->HasStringChanged(m_type, row) ? *wxRED : *wxBLACK);
    }
    return true;
}

bool StringDataViewModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col)
{
    assert(row < m_sd->GetStringCount(m_type));
    if (col == 0)
    {
        return false;
    }
    else if (m_type == StringData::Type::INTRO)
    {
        IntroString news = m_sd->GetIntroString(row);
        switch (col)
        {
        case 1:
            news.SetDisplayTime(static_cast<uint16_t>(variant.GetLong()));
            break;
        case 2:
            news.SetLine1X(static_cast<uint16_t>(variant.GetLong()));
            break;
        case 3:
            news.SetLine1Y(static_cast<uint16_t>(variant.GetLong()));
            break;
        case 4:
            news.SetLine2X(static_cast<uint16_t>(variant.GetLong()));
            break;
        case 5:
            news.SetLine2Y(static_cast<uint16_t>(variant.GetLong()));
            break;
        case 6:
            news.SetLine(0, variant.GetString().ToStdWstring());
            break;
        default:
            news.SetLine(1, variant.GetString().ToStdWstring());
            break;
        }
        m_sd->SetIntroString(row, news);
    }
    else if (m_type == StringData::Type::END_CREDITS)
    {
        auto news = m_sd->GetEndCreditString(row);
        switch (col)
        {
        case 1:
            news.SetColumn(static_cast<uint16_t>(variant.GetLong()));
            break;
        case 2:
            news.SetHeight(static_cast<uint16_t>(variant.GetLong()));
            break;
        default:
            news.SetStr(variant.GetString().ToStdWstring());
            break;
        }
        m_sd->SetEndCreditString(row, news);
    }
    else
    {
        m_sd->SetString(m_type, row, variant.GetString().ToStdWstring());
    }
    return true;
}

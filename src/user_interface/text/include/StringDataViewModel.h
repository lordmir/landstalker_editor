#ifndef _STRING_DATA_VIEW_MODEL_H_
#define _STRING_DATA_VIEW_MODEL_H_

#include <wx/dataview.h>
#include <landstalker/main/include/StringData.h>


class StringDataViewModel : public wxDataViewVirtualListModel
{
public:
    StringDataViewModel(StringData::Type type, std::shared_ptr<StringData> sd);

    virtual unsigned int GetColumnCount() const override;

    unsigned int GetRowCount() const;

    std::string GetColumnHeader(unsigned int col) const;

    virtual wxString GetColumnType(unsigned int col) const override;

    virtual void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override;

    virtual bool GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const override;

    virtual bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override;

private:
    StringData::Type m_type;
    std::shared_ptr<StringData> m_sd;
};

#endif // _STRING_DATA_VIEW_MODEL_H_

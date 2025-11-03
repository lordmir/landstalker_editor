#ifndef _BASE_DATA_VIEW_MODEL_H_
#define _BASE_DATA_VIEW_MODEL_H_

#include <wx/dataview.h>

class BaseDataViewModel : public wxDataViewVirtualListModel
{
public:
    BaseDataViewModel() : wxDataViewVirtualListModel() {}

    virtual ~BaseDataViewModel() {}

    virtual void Initialise() = 0;

    virtual void CommitData() = 0;

    virtual unsigned int GetColumnCount() const override
    {
        return 0;
    }

    virtual unsigned int GetRowCount() const = 0;

    virtual wxString GetColumnHeader(unsigned int col) const = 0;

    virtual wxArrayString GetColumnChoices(unsigned int col) const = 0;

    virtual wxString GetColumnType(unsigned int /*col*/) const override
    {
        return "string";
    }

    virtual void GetValueByRow(wxVariant& /*variant*/, unsigned int /*row*/, unsigned int /*col*/) const override
    {
    }

    virtual bool GetAttrByRow(unsigned int /*row*/, unsigned int /*col*/, wxDataViewItemAttr& /*attr*/) const override
    {
        return false;
    }

    virtual bool SetValueByRow(const wxVariant& /*variant*/, unsigned int /*row*/, unsigned int /*col*/) override
    {
        return false;
    }

    virtual bool DeleteRow(unsigned int row) = 0;

    virtual bool AddRow(unsigned int row) = 0;

    virtual bool SwapRows(unsigned int r1, unsigned int r2) = 0;

    virtual void InitControl(wxDataViewCtrl* ctrl) const = 0;
};

#endif // _BASE_DATA_VIEW_MODEL_H_

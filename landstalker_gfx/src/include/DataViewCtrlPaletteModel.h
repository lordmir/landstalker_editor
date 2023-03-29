#ifndef _DATA_VIEW_CTRL_PALETTE_MODEL_H_
#define _DATA_VIEW_CTRL_PALETTE_MODEL_H_

#include <wx/dataview.h>

#include <string>
#include "DataTypes.h"

class DataViewCtrlPaletteModel : public wxDataViewVirtualListModel
{
public:
    enum class Col
    {
        NAME,
        PALETTE
    };

    DataViewCtrlPaletteModel(const std::vector<std::shared_ptr<PaletteEntry>>& palettes);
    virtual ~DataViewCtrlPaletteModel();

    virtual unsigned int GetColumnCount() const override { return 2; }

    virtual wxString GetColumnType(unsigned int col) const override;

    int GetColumnMaxElements() const;

    virtual void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override;
    virtual bool GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const override;
    virtual bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override;

private:
    std::vector<std::shared_ptr<PaletteEntry>> m_palettes;
};

#endif // _DATA_VIEW_CTRL_PALETTE_MODEL_H_
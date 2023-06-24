#ifndef _FLAG_DATA_VIEW_MODEL_
#define _FLAG_DATA_VIEW_MODEL_

#include <cstdint>
#include <memory>
#include <wx/dataview.h>
#include <GameData.h>
#include <Flags.h>


class EntityVisiblityFlagDataViewModel : public wxDataViewVirtualListModel
{
public:
    EntityVisiblityFlagDataViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd);

    virtual unsigned int GetColumnCount() const override;

    unsigned int GetRowCount() const;

    std::string GetColumnHeader(unsigned int col) const;

    wxArrayString GetColumnChoices(unsigned int col) const;

    virtual wxString GetColumnType(unsigned int col) const override;

    virtual void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override;

    virtual bool GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const override;

    virtual bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override;

private:
    std::vector<EntityVisibilityFlags> m_data;
    uint16_t m_roomnum;
    std::shared_ptr<GameData> m_gd;
};

#endif // _FLAG_DATA_VIEW_MODEL_

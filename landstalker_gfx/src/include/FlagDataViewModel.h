#ifndef _FLAG_DATA_VIEW_MODEL_
#define _FLAG_DATA_VIEW_MODEL_

#include <cstdint>
#include <memory>
#include <wx/dataview.h>
#include <GameData.h>
#include <Flags.h>

class BaseFlagDataViewModel : public wxDataViewVirtualListModel
{
public:
    BaseFlagDataViewModel() : wxDataViewVirtualListModel() {}

    virtual void InitData() = 0;

    virtual void CommitData() = 0;

    virtual unsigned int GetColumnCount() const override
    {
        return 0;
    }

    virtual unsigned int GetRowCount() const = 0;

    virtual wxString GetColumnHeader(unsigned int col) const = 0;

    virtual wxArrayString GetColumnChoices(unsigned int col) const = 0;

    virtual wxString GetColumnType(unsigned int col) const override
    {
        return "string";
    }

    virtual void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override
    {
    }

    virtual bool GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const override
    {
        return false;
    }

    virtual bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override
    {
        return false;
    }

    virtual bool DeleteRow(unsigned int row) = 0;

    virtual bool AddRow(unsigned int row) = 0;

    virtual bool SwapRows(unsigned int r1, unsigned int r2) = 0;
};

template <class T>
class FlagDataViewModel : public BaseFlagDataViewModel
{
public:
    FlagDataViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : BaseFlagDataViewModel(),
          m_roomnum(roomnum),
          m_gd(gd)
    {
        InitData();
        Reset(m_data.size());
    }

    std::vector<T> GetData() { return m_data; }

    virtual void InitData() override
    {
    }

    virtual void CommitData() override
    {
    }

    virtual unsigned int GetColumnCount() const override
    {
        return 0;
    }

    virtual unsigned int GetRowCount() const override
    {
        return m_data.size();
    }

    virtual wxString GetColumnHeader(unsigned int col) const override
    {
        return wxString();
    }

    virtual wxArrayString GetColumnChoices(unsigned int col) const override
    {
        return wxArrayString();
    }

    virtual wxString GetColumnType(unsigned int col) const override
    {
        return "string";
    }

    virtual void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override
    {
    }

    virtual bool GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const override
    {
        return false;
    }

    virtual bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override
    {
        return false;
    }

    virtual bool DeleteRow(unsigned int row) override
    {
        if (row < m_data.size())
        {
            m_data.erase(m_data.begin() + row);
            RowDeleted(row);
            return true;
        }
        return false;
    }

    virtual bool AddRow(unsigned int row) override
    {
        if (row <= m_data.size())
        {
            m_data.insert(m_data.begin() + row, T(m_roomnum));
            RowInserted(row);
            return true;
        }
        return false;
    }

    virtual bool SwapRows(unsigned int r1, unsigned int r2) override
    {
        if (r1 < m_data.size() && r2 < m_data.size() && r1 != r2)
        {
            std::swap(m_data[r1], m_data[r2]);
            RowChanged(r1);
            RowChanged(r2);
            return true;
        }
        return false;
    }
private:
    uint16_t m_roomnum;
    std::shared_ptr<GameData> m_gd;
    std::vector<T> m_data;
};

template <>
void FlagDataViewModel<EntityVisibilityFlags>::InitData();

template <>
void FlagDataViewModel<EntityVisibilityFlags>::CommitData();

template <>
unsigned int FlagDataViewModel<EntityVisibilityFlags>::GetColumnCount() const;

template <>
wxString FlagDataViewModel<EntityVisibilityFlags>::GetColumnHeader(unsigned int col) const;

template <>
wxArrayString FlagDataViewModel<EntityVisibilityFlags>::GetColumnChoices(unsigned int col) const;

template <>
wxString FlagDataViewModel<EntityVisibilityFlags>::GetColumnType(unsigned int col) const;

template <>
void FlagDataViewModel<EntityVisibilityFlags>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const;

template <>
bool FlagDataViewModel<EntityVisibilityFlags>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const;

template <>
bool FlagDataViewModel<EntityVisibilityFlags>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col);

#endif // _FLAG_DATA_VIEW_MODEL_

#ifndef _FLAG_DATA_VIEW_MODEL_
#define _FLAG_DATA_VIEW_MODEL_

#include <cstdint>
#include <memory>
#include <wx/dataview.h>
#include <GameData.h>
#include <Flags.h>
#include <Chests.h>
#include <RoomDialogueTable.h>
#include <WarpList.h>

class BaseFlagDataViewModel : public wxDataViewVirtualListModel
{
public:
    BaseFlagDataViewModel() : wxDataViewVirtualListModel() {}

    virtual void Initialise() = 0;

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

protected:

    virtual void InitData() = 0;
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
    }

    virtual void Initialise() override
    {
        InitData();
        Reset(GetRowCount());
    }

    std::vector<T> GetData() { return m_data; }

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
protected:
    uint16_t m_roomnum;
    std::shared_ptr<GameData> m_gd;
    std::vector<T> m_data;
    mutable std::vector<wxArrayString> m_list;
};

class EntityVisibilityFlagViewModel : public FlagDataViewModel<EntityFlag>
{
public:
    EntityVisibilityFlagViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : FlagDataViewModel<EntityFlag>(roomnum, gd) {}

    virtual void CommitData() override
    {
        m_gd->GetSpriteData()->SetEntityVisibilityFlagsForRoom(m_roomnum, m_data);
    }

protected:
    virtual void InitData() override
    {
        m_data = m_gd->GetSpriteData()->GetEntityVisibilityFlagsForRoom(m_roomnum);
    }
};

class OneTimeEventFlagViewModel : public FlagDataViewModel<OneTimeEventFlag>
{
public:
    OneTimeEventFlagViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : FlagDataViewModel<OneTimeEventFlag>(roomnum, gd) {}

    virtual void CommitData() override
    {
        m_gd->GetSpriteData()->SetOneTimeEventFlagsForRoom(m_roomnum, m_data);
    }

protected:
    virtual void InitData() override
    {
        m_data = m_gd->GetSpriteData()->GetOneTimeEventFlagsForRoom(m_roomnum);
    }
};

class RoomClearFlagViewModel : public FlagDataViewModel<EntityFlag>
{
public:
    RoomClearFlagViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : FlagDataViewModel<EntityFlag>(roomnum, gd) {}

    virtual void CommitData() override
    {
        m_gd->GetSpriteData()->SetMultipleEntityHideFlagsForRoom(m_roomnum, m_data);
    }

    virtual wxString GetColumnHeader(unsigned int col) const override;
    virtual unsigned int GetColumnCount() const override;

protected:
    virtual void InitData() override
    {
        m_data = m_gd->GetSpriteData()->GetMultipleEntityHideFlagsForRoom(m_roomnum);
    }
};

class LockedDoorFlagViewModel : public FlagDataViewModel<EntityFlag>
{
public:
    LockedDoorFlagViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : FlagDataViewModel<EntityFlag>(roomnum, gd) {}

    virtual void CommitData() override
    {
        m_gd->GetSpriteData()->SetLockedDoorFlagsForRoom(m_roomnum, m_data);
    }

    virtual wxString GetColumnHeader(unsigned int col) const override;
    virtual unsigned int GetColumnCount() const override;

protected:
    virtual void InitData() override
    {
        m_data = m_gd->GetSpriteData()->GetLockedDoorFlagsForRoom(m_roomnum);
    }
};

class PermanentSwitchFlagViewModel : public FlagDataViewModel<EntityFlag>
{
public:
    PermanentSwitchFlagViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : FlagDataViewModel<EntityFlag>(roomnum, gd) {}

    virtual void CommitData() override
    {
        m_gd->GetSpriteData()->SetPermanentSwitchFlagsForRoom(m_roomnum, m_data);
    }

    virtual wxString GetColumnHeader(unsigned int col) const override;
    virtual unsigned int GetColumnCount() const override;

protected:
    virtual void InitData() override
    {
        m_data = m_gd->GetSpriteData()->GetPermanentSwitchFlagsForRoom(m_roomnum);
    }
};

class SacredTreeFlagViewModel : public FlagDataViewModel<SacredTreeFlag>
{
public:
    SacredTreeFlagViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : FlagDataViewModel<SacredTreeFlag>(roomnum, gd) {}

    virtual void CommitData() override
    {
        m_gd->GetSpriteData()->SetSacredTreeFlagsForRoom(m_roomnum, m_data);
    }

protected:
    virtual void InitData() override
    {
        m_data = m_gd->GetSpriteData()->GetSacredTreeFlagsForRoom(m_roomnum);
    }
};

class RoomTransitionFlagViewModel : public FlagDataViewModel<WarpList::Transition>
{
public:
    RoomTransitionFlagViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : FlagDataViewModel<WarpList::Transition>(roomnum, gd) {}

    virtual void CommitData() override
    {
        m_gd->GetRoomData()->SetSrcTransitions(m_roomnum, m_data);
    }

protected:
    virtual void InitData() override
    {
        m_data = m_gd->GetRoomData()->GetSrcTransitions(m_roomnum);
    }
};

class ChestFlagViewModel : public FlagDataViewModel<ChestItem>
{
public:
    ChestFlagViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : FlagDataViewModel<ChestItem>(roomnum, gd), m_row_count(0), m_no_chest_flag(false) {}

    virtual unsigned int GetRowCount() const override
    {
        auto ents = m_gd->GetSpriteData()->GetRoomEntities(m_roomnum);
        m_row_count = std::count_if(ents.begin(), ents.end(), [](const auto& ent) { return ent.GetType() == 0x12; });
        return m_no_chest_flag ? m_row_count : 0;
    }

    virtual void CommitData() override
    {
        m_gd->GetRoomData()->SetChestsForRoom(m_roomnum, m_data);
        m_gd->GetRoomData()->SetNoChestFlagForRoom(m_roomnum, !m_no_chest_flag);
        m_gd->GetRoomData()->CleanupChests(*m_gd);
    }

    bool GetChestSetFlag() const
    {
        return m_no_chest_flag;
    }

    void SetChestSetFlag(bool flag)
    {
        m_no_chest_flag = flag;
        Reset(flag ? m_row_count : 0);
    }

protected:
    virtual void InitData() override
    {
        m_gd->GetRoomData()->CleanupChests(*m_gd);
        m_no_chest_flag = !m_gd->GetRoomData()->GetNoChestFlagForRoom(m_roomnum);
        m_data = m_gd->GetRoomData()->GetChestsForRoom(m_roomnum);
        m_data.resize(GetRowCount());
    }
private:
    mutable unsigned int m_row_count;
    bool m_no_chest_flag;
};

class RoomDialogueFlagViewModel : public FlagDataViewModel<Character>
{
public:
    RoomDialogueFlagViewModel(uint16_t roomnum, std::shared_ptr<GameData> gd)
        : FlagDataViewModel<Character>(roomnum, gd) {}

    virtual void CommitData() override
    {
        m_gd->GetStringData()->SetRoomCharacters(m_roomnum, m_data);
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
        if (m_data.size() < 64)
        {
            m_data.insert(m_data.end(), Character());
            RowInserted(m_data.size() - 1);
            return true;
        }
        return false;
    }

protected:
    virtual void InitData() override
    {
        m_data = m_gd->GetStringData()->GetRoomCharacters(m_roomnum);
        m_list.push_back(wxArrayString());
        m_list[0].clear();
        for (int i = 0; i < 0x400; ++i)
        {
            if (i < m_gd->GetStringData()->GetCharNameCount())
            {
                m_list[0].Add(StrPrintf("[%03X] ", i) + _(m_gd->GetStringData()->GetCharName(i)));
            }
            else
            {
                m_list[0].Add(StrPrintf("[%03X] ", i) + _(m_gd->GetStringData()->GetDefaultCharName()));
            }
        }
    }
};

template <>
unsigned int FlagDataViewModel<EntityFlag>::GetColumnCount() const;

template <>
wxString FlagDataViewModel<EntityFlag>::GetColumnHeader(unsigned int col) const;

template <>
wxArrayString FlagDataViewModel<EntityFlag>::GetColumnChoices(unsigned int col) const;

template <>
wxString FlagDataViewModel<EntityFlag>::GetColumnType(unsigned int col) const;

template <>
void FlagDataViewModel<EntityFlag>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const;

template <>
bool FlagDataViewModel<EntityFlag>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const;

template <>
bool FlagDataViewModel<EntityFlag>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col);

template <>
unsigned int FlagDataViewModel<OneTimeEventFlag>::GetColumnCount() const;

template <>
wxString FlagDataViewModel<OneTimeEventFlag>::GetColumnHeader(unsigned int col) const;

template <>
wxArrayString FlagDataViewModel<OneTimeEventFlag>::GetColumnChoices(unsigned int col) const;

template <>
wxString FlagDataViewModel<OneTimeEventFlag>::GetColumnType(unsigned int col) const;

template <>
void FlagDataViewModel<OneTimeEventFlag>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const;

template <>
bool FlagDataViewModel<OneTimeEventFlag>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const;

template <>
bool FlagDataViewModel<OneTimeEventFlag>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col);

template <>
unsigned int FlagDataViewModel<SacredTreeFlag>::GetColumnCount() const;

template <>
wxString FlagDataViewModel<SacredTreeFlag>::GetColumnHeader(unsigned int col) const;

template <>
wxArrayString FlagDataViewModel<SacredTreeFlag>::GetColumnChoices(unsigned int col) const;

template <>
wxString FlagDataViewModel<SacredTreeFlag>::GetColumnType(unsigned int col) const;

template <>
void FlagDataViewModel<SacredTreeFlag>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const;

template <>
bool FlagDataViewModel<SacredTreeFlag>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const;

template <>
bool FlagDataViewModel<SacredTreeFlag>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col);

template <>
unsigned int FlagDataViewModel<WarpList::Transition>::GetColumnCount() const;

template <>
wxString FlagDataViewModel<WarpList::Transition>::GetColumnHeader(unsigned int col) const;

template <>
wxArrayString FlagDataViewModel<WarpList::Transition>::GetColumnChoices(unsigned int col) const;

template <>
wxString FlagDataViewModel<WarpList::Transition>::GetColumnType(unsigned int col) const;

template <>
void FlagDataViewModel<WarpList::Transition>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const;

template <>
bool FlagDataViewModel<WarpList::Transition>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const;

template <>
bool FlagDataViewModel<WarpList::Transition>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col);

template <>
unsigned int FlagDataViewModel<ChestItem>::GetColumnCount() const;

template <>
wxString FlagDataViewModel<ChestItem>::GetColumnHeader(unsigned int col) const;

template <>
wxArrayString FlagDataViewModel<ChestItem>::GetColumnChoices(unsigned int col) const;

template <>
wxString FlagDataViewModel<ChestItem>::GetColumnType(unsigned int col) const;

template <>
void FlagDataViewModel<ChestItem>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const;

template <>
bool FlagDataViewModel<ChestItem>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const;

template <>
bool FlagDataViewModel<ChestItem>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col);

template <>
unsigned int FlagDataViewModel<Character>::GetColumnCount() const;

template <>
wxString FlagDataViewModel<Character>::GetColumnHeader(unsigned int col) const;

template <>
wxArrayString FlagDataViewModel<Character>::GetColumnChoices(unsigned int col) const;

template <>
wxString FlagDataViewModel<Character>::GetColumnType(unsigned int col) const;

template <>
void FlagDataViewModel<Character>::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const;

template <>
bool FlagDataViewModel<Character>::GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const;

template <>
bool FlagDataViewModel<Character>::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col);

#endif // _FLAG_DATA_VIEW_MODEL_

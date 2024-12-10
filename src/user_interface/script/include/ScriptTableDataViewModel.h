#ifndef _SCRIPT_TABLE_DATA_VIEW_MODEL_H_
#define _SCRIPT_TABLE_DATA_VIEW_MODEL_H_

#include <user_interface/misc/include/BaseDataViewModel.h>
#include <landstalker/main/include/GameData.h>
#include <landstalker/script/include/ScriptTable.h>

class ScriptTableDataViewModel : public BaseDataViewModel
{
public:
    enum class Mode
    {
        CUTSCENE,
        CHARACTER,
        SHOP,
        ITEM
    };

    ScriptTableDataViewModel(std::shared_ptr<GameData> gd);

    virtual void Initialise();

    virtual void SetMode(const Mode& mode, unsigned int index = 0);

    virtual void CommitData();

    virtual unsigned int GetColumnCount() const override;

    virtual unsigned int GetRowCount() const;

    virtual wxString GetColumnHeader(unsigned int col) const;

    virtual wxArrayString GetColumnChoices(unsigned int col) const;

    virtual wxString GetColumnType(unsigned int col) const override;

    virtual void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override;

    virtual bool GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const override;

    virtual bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override;

    virtual bool DeleteRow(unsigned int row);

    virtual bool AddRow(unsigned int row);

    virtual bool SwapRows(unsigned int r1, unsigned int r2);

    virtual void InitControl(wxDataViewCtrl* ctrl) const override;
private:
    std::vector<ScriptTable::Action>* GetTable();
    ScriptTable::Action* GetCell(unsigned int row);
    const std::vector<ScriptTable::Action>* GetTable() const;
    const ScriptTable::Action* GetCell(unsigned int row) const;

    std::shared_ptr<GameData> m_gd;

    Mode m_mode;
    unsigned int m_index;

    std::shared_ptr<std::vector<ScriptTable::Action>> m_cutscene_script;
    std::shared_ptr<std::vector<ScriptTable::Action>> m_character_script;
    std::shared_ptr<std::vector<ScriptTable::Shop>> m_shop_script;
    std::shared_ptr<std::vector<ScriptTable::Item>> m_item_script;
};

#endif // _SCRIPT_TABLE_DATA_VIEW_MODEL_H_

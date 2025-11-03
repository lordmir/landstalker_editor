#ifndef _SCRIPT_TABLE_DATA_VIEW_MODEL_H_
#define _SCRIPT_TABLE_DATA_VIEW_MODEL_H_

#include <misc/BaseDataViewModel.h>
#include <landstalker/main/GameData.h>
#include <landstalker/script/ScriptTable.h>

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

    ScriptTableDataViewModel(std::shared_ptr<Landstalker::GameData> gd);

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

    Landstalker::ScriptTable::Action GetAction(unsigned int row) const;
private:
    std::vector<Landstalker::ScriptTable::Action>* GetTable();
    Landstalker::ScriptTable::Action* GetCell(unsigned int row);
    const std::vector<Landstalker::ScriptTable::Action>* GetTable() const;
    const Landstalker::ScriptTable::Action* GetCell(unsigned int row) const;

    std::shared_ptr<Landstalker::GameData> m_gd;

    Mode m_mode;
    unsigned int m_index;

    std::shared_ptr<std::vector<Landstalker::ScriptTable::Action>> m_cutscene_script;
    std::shared_ptr<std::vector<Landstalker::ScriptTable::Action>> m_character_script;
    std::shared_ptr<std::vector<Landstalker::ScriptTable::Shop>> m_shop_script;
    std::shared_ptr<std::vector<Landstalker::ScriptTable::Item>> m_item_script;
};

#endif // _SCRIPT_TABLE_DATA_VIEW_MODEL_H_

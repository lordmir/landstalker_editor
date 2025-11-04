#ifndef _SCRIPT_DATA_VIEW_MODEL_
#define _SCRIPT_DATA_VIEW_MODEL_

#include <misc/BaseDataViewModel.h>
#include <landstalker/main/GameData.h>

class ScriptDataViewModel : public BaseDataViewModel
{
public:
    ScriptDataViewModel(std::shared_ptr<Landstalker::GameData> gd);

    virtual void Initialise();

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
    std::shared_ptr<Landstalker::GameData> m_gd;

    std::shared_ptr<Landstalker::Script> m_script;
};

#endif // _SCRIPT_DATA_VIEW_MODEL

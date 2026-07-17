#ifndef _SCRIPT_DATA_VIEW_MODEL_
#define _SCRIPT_DATA_VIEW_MODEL_

#include <misc/BaseDataViewModel.h>
#include <landstalker/main/GameData.h>

class ScriptDataViewModel : public BaseDataViewModel
{
public:
    ScriptDataViewModel(std::shared_ptr<Landstalker::GameData> gd);
    // Windowed variant: presents only the contiguous run of `count` script lines starting at
    // `start` (a script segment), as rows 0..count-1. Rows added/deleted through this model
    // grow/shrink the window; the Index column still shows absolute script line numbers.
    ScriptDataViewModel(std::shared_ptr<Landstalker::GameData> gd, unsigned int start, unsigned int count);

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

    virtual bool AddRow(unsigned int row) override;
    // Context-menu "Add Above/Below" overload (lets the user pick a type up front instead of
    // always defaulting to STRING then needing a separate "Change Type").
    bool AddRow(unsigned int row, Landstalker::ScriptTableEntryType type);

    virtual bool SwapRows(unsigned int r1, unsigned int r2);

    // Row-level "Change Type" (context menu): rebuilds the row as a default-valued entry of
    // new_type, carrying over the old entry's Clear/End/raw data - same preserve behaviour the
    // Main Script Editor's old inline type dropdown used to have.
    bool ChangeRowType(unsigned int row, Landstalker::ScriptTableEntryType new_type);

    virtual void InitControl(wxDataViewCtrl* ctrl) const override;

    // The absolute script line a view-relative row maps to (identity when not windowed).
    unsigned int ToScriptLine(unsigned int row) const { return m_start + row; }
private:
    std::shared_ptr<Landstalker::GameData> m_gd;

    std::shared_ptr<Landstalker::Script> m_script;

    // Window onto the script: m_window_count < 0 means the whole table (m_start is then 0).
    unsigned int m_start = 0;
    int m_window_count = -1;
};

#endif // _SCRIPT_DATA_VIEW_MODEL

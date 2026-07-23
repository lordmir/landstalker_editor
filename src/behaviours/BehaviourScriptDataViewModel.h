#ifndef _BEHAVIOUR_SCRIPT_DATA_VIEW_MODEL_H_
#define _BEHAVIOUR_SCRIPT_DATA_VIEW_MODEL_H_

#include <vector>

#include <misc/BaseDataViewModel.h>
#include <landstalker/main/GameData.h>
#include <landstalker/behaviours/Behaviours.h>

class BehaviourScriptDataViewModel : public BaseDataViewModel
{
public:
    BehaviourScriptDataViewModel(std::shared_ptr<Landstalker::GameData> gd, int script_id);

    virtual void Initialise() override;

    // Pushes the working copy back into SpriteData - called after every mutation, so the
    // control has "live" commit semantics like the Main Script Editor (no separate Save step).
    virtual void CommitData() override;

    virtual unsigned int GetColumnCount() const override;

    virtual unsigned int GetRowCount() const override;

    virtual wxString GetColumnHeader(unsigned int col) const override;

    virtual wxArrayString GetColumnChoices(unsigned int col) const override;

    virtual wxString GetColumnType(unsigned int col) const override;

    virtual void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override;

    virtual bool GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const override;

    virtual bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override;

    virtual bool DeleteRow(unsigned int row) override;

    virtual bool AddRow(unsigned int row) override;

    virtual bool SwapRows(unsigned int r1, unsigned int r2) override;

    virtual void InitControl(wxDataViewCtrl* ctrl) const override;

private:
    std::shared_ptr<Landstalker::GameData> m_gd;
    int m_script_id;

    // Working copy of the script's command list; every mutation writes it straight back via
    // CommitData(), so this never diverges from SpriteData outside a single mutation call.
    std::vector<Landstalker::Behaviours::Command> m_commands;
};

#endif // _BEHAVIOUR_SCRIPT_DATA_VIEW_MODEL_H_

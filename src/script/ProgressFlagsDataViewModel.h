#ifndef _PROGRESS_FLAGS_DATA_VIEW_MODEL_H_
#define _PROGRESS_FLAGS_DATA_VIEW_MODEL_H_

#include <misc/BaseDataViewModel.h>
#include <landstalker/main/GameData.h>
#include <landstalker/script/ProgressFlags.h>

class ProgressFlagsDataViewModel : public BaseDataViewModel
{
public:

    ProgressFlagsDataViewModel(std::shared_ptr<Landstalker::GameData> gd);

    virtual ~ProgressFlagsDataViewModel();

    virtual void Initialise();

    virtual void CommitData();

    virtual unsigned int GetColumnCount() const override;

    virtual unsigned int GetRowCount() const;

    std::pair<int, int> GetQuestProgressFromRow(int row) const;

    int GetTotalQuests() const;

    int GetMaxQuest() const;

    int GetNextFreeQuest() const;

    int GetRowFromQuestProgress(int quest, int progress) const;

    int GetTotalProgressInQuest(int quest) const;

    virtual wxString GetColumnHeader(unsigned int col) const;

    virtual wxArrayString GetColumnChoices(unsigned int col) const;

    virtual wxString GetColumnType(unsigned int col) const override;

    virtual void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override;

    virtual bool GetAttrByRow(unsigned int row, unsigned int col, wxDataViewItemAttr& attr) const override;

    virtual bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override;

    virtual bool DeleteRow(unsigned int row);

    virtual bool AddRow(unsigned int row);

    virtual bool AppendRow(unsigned int row);

    virtual bool DeleteQuest(unsigned int row);

    virtual bool AddQuest();

    virtual bool SwapRows(unsigned int r1, unsigned int r2);

    virtual void InitControl(wxDataViewCtrl* ctrl) const override;
private:
    std::shared_ptr<Landstalker::GameData> m_gd;

    Landstalker::ProgressFlags::Flags m_flags;
};

#endif // _PROGRESS_FLAGS_DATA_VIEW_MODEL_H_

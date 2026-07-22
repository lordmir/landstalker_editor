#ifndef _ROOM_CONSTANTS_DATA_VIEW_MODEL_H_
#define _ROOM_CONSTANTS_DATA_VIEW_MODEL_H_

#include <string>
#include <vector>

#include <misc/BaseDataViewModel.h>
#include <landstalker/main/GameData.h>

// Editing model for the room index constants in the disassembly's rooms.inc. The
// underlying store is a name-keyed map, which would reshuffle rows as soon as a name is
// edited, so this keeps its own ordered list of names purely to give the grid stable row
// indices. Values are always read and written through RoomData rather than cached here.
class RoomConstantsDataViewModel : public BaseDataViewModel
{
public:
    RoomConstantsDataViewModel(std::shared_ptr<Landstalker::GameData> gd);

    virtual ~RoomConstantsDataViewModel();

    virtual void Initialise() override;

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

    // Row -> constant name, or an empty string if the row is out of range.
    std::string GetRowName(unsigned int row) const;
private:
    // A name not already in use, for a newly added row.
    std::string MakeUniqueName() const;

    std::shared_ptr<Landstalker::GameData> m_gd;

    // Display order. Rebuilt from the game data on Initialise, then maintained in step
    // with the edits made through this model.
    std::vector<std::string> m_names;
    wxArrayString m_room_choices;
};

#endif // _ROOM_CONSTANTS_DATA_VIEW_MODEL_H_

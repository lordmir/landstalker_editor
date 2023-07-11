#ifndef _DOOR_CONTROL_FRAME_H_
#define _DOOR_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <Doors.h>
#include <SelectionControlFrame.h>

class RoomViewerFrame;

class DoorControlFrame : public SelectionControlFrame
{
public:
	DoorControlFrame(RoomViewerFrame* parent, ImageList* imglst);
	virtual ~DoorControlFrame();

	void SetDoors(const std::vector<Door>& doors);
	void ResetSwaps();
	virtual int GetMaxSelection() const;
protected:
	virtual void Select();
	virtual void Add();
	virtual void Delete();
	virtual void MoveUp();
	virtual void MoveDown();
	virtual void OpenElement();
	virtual bool HandleKeyPress(unsigned int key, unsigned int modifiers);
	virtual std::string MakeLabel(int index) const;
private:
	std::vector<Door> m_doors;

	RoomViewerFrame* m_rvf;
};

wxDECLARE_EVENT(EVT_DOOR_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_OPEN_PROPERTIES, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_MOVE_DOWN, wxCommandEvent);

#endif // _DOOR_CONTROL_FRAME_H_

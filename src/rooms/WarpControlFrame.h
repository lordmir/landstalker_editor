#ifndef _WARP_CONTROL_FRAME_H_
#define _WARP_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <landstalker/rooms/WarpList.h>
#include <misc/SelectionControlFrame.h>

class RoomViewerFrame;

class WarpControlFrame : public SelectionControlFrame
{
public:
	WarpControlFrame(RoomViewerFrame* parent, ImageList* imglst);
	virtual ~WarpControlFrame();

	void SetWarps(const std::vector<Landstalker::WarpList::Warp>& entities);
	void ResetWarps();
	virtual int GetMaxSelection() const;
protected:
	virtual void Select();
	virtual void Add();
	virtual void Delete();
	virtual void MoveUp();
	virtual void MoveDown();
	virtual void OpenElement();
	virtual bool HandleKeyPress(unsigned int key, unsigned int modifiers);
	virtual std::wstring MakeLabel(int index) const;
private:
	std::vector<Landstalker::WarpList::Warp> m_warps;

	RoomViewerFrame* m_rvf;
};

wxDECLARE_EVENT(EVT_WARP_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_WARP_OPEN_PROPERTIES, wxCommandEvent);
wxDECLARE_EVENT(EVT_WARP_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_WARP_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_WARP_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_WARP_MOVE_DOWN, wxCommandEvent);

#endif // _WARP_CONTROL_FRAME_H_

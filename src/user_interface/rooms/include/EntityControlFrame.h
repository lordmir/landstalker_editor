#ifndef _ENTITY_CONTROL_FRAME_H_
#define _ENTITY_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <landstalker/rooms/include/Entity.h>
#include <user_interface/main/include/ImageList.h>
#include <user_interface/misc/include/SelectionControlFrame.h>

class RoomViewerFrame;

class EntityControlFrame : public SelectionControlFrame
{
public:
	EntityControlFrame(RoomViewerFrame* parent, ImageList* imglst);
	virtual ~EntityControlFrame();

	void SetEntities(const std::vector<Entity>& entities);
	void ResetEntities();
	virtual int GetMaxSelection() const;
protected:
	virtual void Select();
	virtual void Add();
	virtual void Delete();
	virtual void MoveUp();
	virtual void MoveDown();
	virtual void OpenElement();
	virtual std::wstring MakeLabel(int index) const;
	virtual bool HandleKeyPress(unsigned int key, unsigned int modifiers);
private:
	std::vector<Entity> m_entities;

	RoomViewerFrame* m_rvf;
};

wxDECLARE_EVENT(EVT_ENTITY_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_OPEN_PROPERTIES, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_MOVE_DOWN, wxCommandEvent);

#endif // _ENTITY_CONTROL_FRAME_H_

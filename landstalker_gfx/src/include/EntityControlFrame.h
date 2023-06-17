#ifndef _ENTITY_CONTROL_FRAME_H_
#define _ENTITY_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <Entity.h>
#include <ImageList.h>

class RoomViewerFrame;

class EntityControlFrame : public wxWindow
{
public:
	EntityControlFrame(RoomViewerFrame* parent, ImageList* imglst);
	virtual ~EntityControlFrame();

	void SetEntities(const std::vector<Entity>& entities);
	void SetSelected(int selected);
	int GetSelected();
	void ResetEntities();
private:
	void FireEvent(const wxEventType& e, intptr_t userdata = 0);
	void UpdateUI();

	void OnSelect(wxCommandEvent& evt);
	void OnListDoubleClick(wxCommandEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);
	void OnEntityAdd(wxCommandEvent& evt);
	void OnEntityDelete(wxCommandEvent& evt);
	void OnEntityMoveUp(wxCommandEvent& evt);
	void OnEntityMoveDown(wxCommandEvent& evt);

	wxListBox* m_ctrl_entity_list;
	wxBitmapButton* m_ctrl_add_entity;
	wxBitmapButton* m_ctrl_delete_entity;
	wxBitmapButton* m_ctrl_move_entity_up;
	wxBitmapButton* m_ctrl_move_entity_down;
	std::vector<Entity> m_entities;

	RoomViewerFrame* m_rvf;
	ImageList* m_imglst;
};

wxDECLARE_EVENT(EVT_ENTITY_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_OPEN_PROPERTIES, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_ENTITY_MOVE_DOWN, wxCommandEvent);

#endif // _ENTITY_CONTROL_FRAME_H_

#ifndef _SELECTION_CONTROL_FRAME_H_
#define _SELECTION_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <ImageList.h>

class SelectionControlFrame : public wxWindow
{
public:
	SelectionControlFrame(wxWindow* parent, ImageList* imglst);
	virtual ~SelectionControlFrame();

	void SetSelected(int selected);
	int GetSelected();
	virtual int GetMaxSelection() const = 0;
protected:
	virtual void Select() = 0;
	virtual void Add() = 0;
	virtual void Delete() = 0;
	virtual void MoveUp() = 0;
	virtual void MoveDown() = 0;
	virtual void OpenElement() = 0;
	virtual bool HandleKeyPress(unsigned int key, unsigned int modifiers) = 0;
	void FireEvent(const wxEventType& e);
	void UpdateUI();
	virtual std::string MakeLabel(int index) const = 0;

	wxListBox* m_ctrl_list;
	wxBitmapButton* m_ctrl_add;
	wxBitmapButton* m_ctrl_delete;
	wxBitmapButton* m_ctrl_move_up;
	wxBitmapButton* m_ctrl_move_down;
	ImageList* m_imglst;
private:
	void OnSelect(wxCommandEvent& evt);
	void OnListDoubleClick(wxCommandEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnDelete(wxCommandEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
};

#endif // _SELECTION_CONTROL_FRAME_H_
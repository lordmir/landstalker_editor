#ifndef _ROOM_ERROR_DIALOG_H_
#define _ROOM_ERROR_DIALOG_H_

#include <wx/wx.h>
#include <vector>
#include <string>

class RoomErrorDialog : public wxDialog
{
public:
	RoomErrorDialog(wxWindow* parent, const std::vector<std::string>& errors);

	virtual ~RoomErrorDialog();
private:
	void OnOK(wxCommandEvent& evt);
	wxButton* m_ok;
};

#endif // _ROOM_ERROR_DIALOG_H_

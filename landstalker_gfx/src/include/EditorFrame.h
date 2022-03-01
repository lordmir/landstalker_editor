#ifndef _EDITOR_FRAME_H_
#define _EDITOR_FRAME_H_

#include <wx/wx.h>
#include <string>
#include <vector>
#include <wx/propgrid/manager.h>

class EditorFrame : public wxWindow
{
public:
	EditorFrame(wxWindow* parent, wxWindowID id);
	virtual ~EditorFrame();
	virtual void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status) const;
	virtual void ClearStatusBar(wxStatusBar& status) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	virtual void ClearProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual bool Show(bool show = true);
protected:
	void FireEvent(const wxEventType& e, const std::string& data);
	void FireEvent(const wxEventType& e);
};

wxDECLARE_EVENT(EVT_STATUSBAR_INIT, wxCommandEvent);
wxDECLARE_EVENT(EVT_STATUSBAR_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_STATUSBAR_CLEAR, wxCommandEvent);
wxDECLARE_EVENT(EVT_PROPERTIES_INIT, wxCommandEvent);
wxDECLARE_EVENT(EVT_PROPERTIES_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_PROPERTIES_CLEAR, wxCommandEvent);

#endif // _EDITOR_FRAME_H_
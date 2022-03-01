#ifndef _EDITOR_FRAME_H_
#define _EDITOR_FRAME_H_

#include <wx/wx.h>
#include <string>
#include <vector>
#include <unordered_map>
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
	virtual void InitMenu(wxMenuBar& menu) const;
	virtual void ClearMenu(wxMenuBar& menu) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	virtual bool Show(bool show = true);
protected:
	void FireEvent(const wxEventType& e, const std::string& data);
	void FireEvent(const wxEventType& e);
	wxMenu& AddMenu(wxMenuBar& parent, int position, int id, const std::string& name) const;
	wxMenuItem& AddMenuItem(wxMenu& parent, int position, int id, const std::string& name, wxItemKind kind = wxITEM_NORMAL, const std::string& help = "") const;
	wxMenu& GetMenu(int id) const;
	wxMenuItem& GetMenuItem(int id) const;
private:
	mutable std::unordered_map<int, std::pair<wxMenuBar*, wxMenu*>> m_menus;
	mutable std::unordered_map<int, std::pair<wxMenu*, wxMenuItem*>> m_menuitems;
};

wxDECLARE_EVENT(EVT_STATUSBAR_INIT, wxCommandEvent);
wxDECLARE_EVENT(EVT_STATUSBAR_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_STATUSBAR_CLEAR, wxCommandEvent);
wxDECLARE_EVENT(EVT_PROPERTIES_INIT, wxCommandEvent);
wxDECLARE_EVENT(EVT_PROPERTIES_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_PROPERTIES_CLEAR, wxCommandEvent);
wxDECLARE_EVENT(EVT_MENU_INIT, wxCommandEvent);
wxDECLARE_EVENT(EVT_MENU_CLEAR, wxCommandEvent);

#endif // _EDITOR_FRAME_H_
#ifndef _EDITOR_FRAME_H_
#define _EDITOR_FRAME_H_

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/propgrid/manager.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <ImageList.h>

#include <ImageList.h>
#include "GameData.h"

class EditorFrame : public wxWindow
{
public:
	EditorFrame(wxWindow* parent, wxWindowID id, ImageList* imglst);
	virtual ~EditorFrame();
	virtual void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const;
	virtual void ClearStatusBar(wxStatusBar& status) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	virtual void ClearProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void ClearMenu(wxMenuBar& menu) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	virtual bool Show(bool show = true);
	virtual void UpdateUI() const;
	virtual void SetGameData(std::shared_ptr<GameData> gd) { m_gd = gd; }
	virtual void ClearGameData() { m_gd = nullptr; }
	virtual void SetImageList(ImageList* imglst);
protected:
	void CheckMenuItem(int id, bool checked) const;
	void CheckToolbarItem(const std::string& name, int id, bool checked) const;
	void EnableMenuItem(int id, bool enabled) const;
	void EnableToolbarItem(const std::string& name, int id, bool enabled) const;
	wxControl* GetToolbarItem(const std::string& name, int id);
	wxControl* GetToolbarItem(const std::string& name, int id) const;
	void SetPaneVisibility(wxWindow* pane, bool visible);
	bool IsPaneVisible(wxWindow* pane) const;
	void InsertPane(wxWindow* pane, const wxAuiPaneInfo& insert_location, int insert_level = wxAUI_INSERT_PANE);
	void SetToolbarVisibility(const std::string& name, bool visible);
	bool IsToolbarVisible(const std::string& name) const;
	bool ArePropsInitialised() const;
	void FireEvent(const wxEventType& e, const std::string& data);
	void FireEvent(const wxEventType& e);
	wxMenu& AddMenu(wxMenuBar& parent, int position, int id, const std::string& name) const;
	wxMenuItem& AddMenuItem(wxMenu& parent, int position, int id, const std::string& name, wxItemKind kind = wxITEM_NORMAL, const std::string& help = std::string()) const;
	wxAuiToolBar& AddToolbar(wxAuiManager& mgr, wxAuiToolBar& tb, const std::string& name, const std::string title, wxAuiPaneInfo& position) const;
	wxMenu* GetMenu(int id) const;
	wxMenuItem* GetMenuItem(int id) const;
	wxAuiToolBar* GetToolbar(const std::string name) const;
	ImageList* GetImageList();

	void OnPaneClose(wxAuiManagerEvent& event);

	std::shared_ptr<GameData> m_gd;

private:
	mutable bool m_props_init;

	static std::unordered_map<int, std::pair<wxMenuBar*, wxMenu*>> m_menus;
	static std::unordered_map<int, std::pair<wxMenu*, wxMenuItem*>> m_menuitems;
	mutable std::unordered_map<std::string, wxAuiToolBar*> m_toolbars;

	ImageList* m_imglst;
};

wxDECLARE_EVENT(EVT_STATUSBAR_INIT, wxCommandEvent);
wxDECLARE_EVENT(EVT_STATUSBAR_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_STATUSBAR_CLEAR, wxCommandEvent);
wxDECLARE_EVENT(EVT_PROPERTIES_INIT, wxCommandEvent);
wxDECLARE_EVENT(EVT_PROPERTIES_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_PROPERTIES_CLEAR, wxCommandEvent);
wxDECLARE_EVENT(EVT_MENU_INIT, wxCommandEvent);
wxDECLARE_EVENT(EVT_MENU_CLEAR, wxCommandEvent);
wxDECLARE_EVENT(EVT_GO_TO_NAV_ITEM, wxCommandEvent);

#endif // _EDITOR_FRAME_H_
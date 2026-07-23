#ifndef _BEHAVIOUR_SCRIPT_EDITOR_FRAME_H_
#define _BEHAVIOUR_SCRIPT_EDITOR_FRAME_H_

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <behaviours/BehaviourScriptEditorCtrl.h>

#include <wx/scrolwin.h>

#include <string>
#include <memory>
#include <vector>

class BehaviourScriptEditorFrame : public EditorFrame
{
public:
	BehaviourScriptEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~BehaviourScriptEditorFrame();

	bool Open(int script_id = 0);
	int GetOpenScriptId() const { return m_editor ? m_editor->GetOpenScriptId() : m_script_id; }
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
	// Rebuilds the script list on every show - behaviour names can be renamed from the rooms
	// editor's entity properties dialog while this editor is hidden.
	virtual bool Show(bool show = true) override;
private:
	// Left-pane script list (same layout as the entity editor's entity list): rebuilds the
	// entries from SpriteData, keeps row -> script id in m_script_ids.
	void RefreshScriptList();
	void SelectScriptInList(int script_id);
	void OnScriptSelected();
	void OnRenameScript();

	// Rebuilds the bottom "Used By" pane for the currently open script: one clickable hyperlink
	// per room entity that runs this behaviour, each navigating to that room (same nav mechanism
	// and hyperlink style as the entity editor's stats panel).
	void RefreshUsage();
	void NavigateTo(const wxString& path);

	void OnSaveAsYaml();
	void OnSaveAllAsYaml();
	void OnLoadFromYaml();
	void OnLoadAllFromYaml();

	bool SaveAsYaml(const std::string& filename, int behaviour_id);
	bool SaveAllAsYaml(const std::string& dirname);
	int LoadFromYaml(const std::string& filename);
	bool LoadAllFromYaml(const std::string& filename);

	virtual void InitProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	virtual void ClearMenu(wxMenuBar& menu) const;

	mutable bool m_reset_props = false;

	mutable wxAuiManager m_mgr;
	BehaviourScriptEditorCtrl* m_editor = nullptr;
	wxListBox* m_script_list = nullptr;
	wxButton* m_rename = nullptr;
	wxScrolledWindow* m_usage_pane = nullptr;
	std::vector<int> m_script_ids;
	int m_script_id = -1;
};

#endif // _BEHAVIOUR_SCRIPT_EDITOR_FRAME_H_

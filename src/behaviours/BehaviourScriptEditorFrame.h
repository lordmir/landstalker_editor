#ifndef _BEHAVIOUR_SCRIPT_EDITOR_FRAME_H_
#define _BEHAVIOUR_SCRIPT_EDITOR_FRAME_H_

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <behaviours/BehaviourScriptEditorCtrl.h>

#include <string>
#include <memory>

class BehaviourScriptEditorFrame : public EditorFrame
{
public:
	BehaviourScriptEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~BehaviourScriptEditorFrame();

	bool Open(int script_id = 0);
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
private:
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
	int m_script_id = -1;
};

#endif // _BEHAVIOUR_SCRIPT_EDITOR_FRAME_H_

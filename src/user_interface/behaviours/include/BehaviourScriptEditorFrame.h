#ifndef _BEHAVIOUR_SCRIPT_EDITOR_FRAME_H_
#define _BEHAVIOUR_SCRIPT_EDITOR_FRAME_H_

#include <landstalker/main/include/GameData.h>
#include <user_interface/main/include/EditorFrame.h>
#include <user_interface/behaviours/include/BehaviourScriptEditorCtrl.h>

#include <string>
#include <memory>

class BehaviourScriptEditorFrame : public EditorFrame
{
public:
	BehaviourScriptEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~BehaviourScriptEditorFrame();

	bool Open(int script_id = 0);
	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();
private:
	virtual void InitProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);

	mutable bool m_reset_props = false;

	mutable wxAuiManager m_mgr;
	BehaviourScriptEditorCtrl* m_editor = nullptr;
	int m_script_id = -1;
};

#endif // _BEHAVIOUR_SCRIPT_EDITOR_FRAME_H_

#ifndef _ROOM_CONSTANTS_FRAME_H_
#define _ROOM_CONSTANTS_FRAME_H_

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <rooms/RoomConstantsCtrl.h>

class RoomConstantsEditorFrame : public EditorFrame
{
public:
	RoomConstantsEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~RoomConstantsEditorFrame();

	bool Open(int row = -1);
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

	void UpdateUI() const;
private:
	virtual void InitProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);

	mutable bool m_reset_props = false;
	mutable wxAuiManager m_mgr;
	RoomConstantsEditorCtrl* m_editor = nullptr;
};

#endif // _ROOM_CONSTANTS_FRAME_H_

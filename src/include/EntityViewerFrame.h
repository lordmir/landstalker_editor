#ifndef _ENTITY_VIEWER_FRAME_H_
#define _ENTITY_VIEWER_FRAME_H_

#include <EditorFrame.h>
#include <EntityViewerCtrl.h>
#include <GameData.h>

#include <string>
#include <memory>

class EntityViewerFrame : public EditorFrame
{
public:
	EntityViewerFrame(wxWindow* parent, ImageList* imglst);
	virtual ~EntityViewerFrame();

	bool Open(int entity_id);
	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();
private:
	mutable wxAuiManager m_mgr;
	EntityViewerCtrl* m_entity_ctrl;
	std::shared_ptr<Palette> m_palette;
	int entity_id = -1;
};

#endif // _ENTITY_VIEWER_FRAME_H_

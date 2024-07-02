#ifndef _ENTITY_VIEWER_FRAME_H_
#define _ENTITY_VIEWER_FRAME_H_

#include <EditorFrame.h>
#include <EntityViewerCtrl.h>

#include <string>

class EntityViewerFrame : public EditorFrame
{
public:
	EntityViewerFrame(wxWindow* parent, ImageList* imglst);
	virtual ~EntityViewerFrame();

	bool Open(const std::string& entity);
private:
	mutable wxAuiManager m_mgr;
	EntityViewerCtrl* m_entity_ctrl;
};

#endif // _ENTITY_VIEWER_FRAME_H_

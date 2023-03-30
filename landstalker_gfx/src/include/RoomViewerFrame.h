#ifndef _ROOM_VIEWER_FRAME_H_
#define _ROOM_VIEWER_FRAME_H_

#include "EditorFrame.h"
#include "RoomViewerCtrl.h"
#include "GameData.h"
#include <memory>

class RoomViewerFrame : public EditorFrame
{
public:
	enum class Mode : uint8_t
	{
		NORMAL,
		HEIGHTMAP,
		WARPS,
		ENTITY_PLACEMENT
	};

	RoomViewerFrame(wxWindow* parent);
	virtual ~RoomViewerFrame();

	Mode GetMode() const { return m_mode; }
	void SetMode(Mode mode);
	void Update();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetRoomNum(uint16_t roomnum);
	uint16_t GetRoomNum() const { return m_roomnum; }
private:
	Mode m_mode;
	mutable wxAuiManager m_mgr;
	std::string m_title;
	RoomViewerCtrl* m_roomview;
	std::shared_ptr<GameData> m_g;
	uint16_t m_roomnum;
};

#endif // _ROOM_VIEWER_FRAME_H_

#ifndef _PALETTE_LIST_FRAME_H_
#define _PALETTE_LIST_FRAME_H_

#include "EditorFrame.h"
#include "GameData.h"
#include "wx/listbox.h"

class PaletteListFrame : public EditorFrame
{
public:
	enum class Mode : uint8_t
	{
		ROOM,
		ROOM_MISC,
		SPRITE_LO,
		SPRITE_HI,
		PROJECTILE,
		EQUIP,
		MISC
	};

	PaletteListFrame(wxWindow* parent);
	virtual ~PaletteListFrame();

	Mode GetMode() const { return m_mode; }
	void SetMode(Mode mode);
	void Update();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();
private:
	Mode m_mode;
	mutable wxAuiManager m_mgr;
	wxListBox* m_test;
	std::string m_title;
};

#endif // _PALETTE_LIST_FRAME_H_

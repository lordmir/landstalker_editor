#ifndef _PALETTE_LIST_FRAME_H_
#define _PALETTE_LIST_FRAME_H_

#include <wx/dataview.h>
#include "DataViewCtrlPaletteModel.h"
#include "DataViewCtrlPaletteRenderer.h"
#include "EditorFrame.h"
#include "GameData.h"

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

	PaletteListFrame(wxWindow* parent, ImageList* imglst);
	virtual ~PaletteListFrame();

	Mode GetMode() const { return m_mode; }
	void SetMode(Mode mode);
	void Update();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	bool ExportAllPalettes(const filesystem::path& filename);
	bool ImportPalettes(const filesystem::path& filename);
private:
	virtual void UpdateStatusBar(wxStatusBar& status) const;
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);

	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnKeyPress(wxKeyEvent& evt);
	void OnMenuImport();
	void OnMenuExport();

	Mode m_mode;
	mutable wxAuiManager m_mgr;
	wxDataViewCtrl* m_list;
	DataViewCtrlPaletteModel* m_model;
	DataViewCtrlPaletteRenderer* m_renderer;
	mutable wxDataViewItem m_prev_itm;
	mutable int m_prev_colour = -1;
	std::string m_title;
};

#endif // _PALETTE_LIST_FRAME_H_

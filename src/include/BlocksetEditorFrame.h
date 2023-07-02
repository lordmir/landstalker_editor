#ifndef _BLOCKSET_EDITOR_FRAME_H_
#define _BLOCKSET_EDITOR_FRAME_H_

#include <wx/wx.h>
#include <string>
#include <map>
#include <memory>
#include "Palette.h"
#include "Tileset.h"
#include "Block.h"
#include "BlocksetCmp.h"
#include "BlocksetEditorCtrl.h"
#include "EditorFrame.h"
#include "TilesetEditor.h"

class BlocksetEditorFrame : public EditorFrame
{
public:
	BlocksetEditorFrame(wxWindow* parent, ImageList* imglst);
	~BlocksetEditorFrame();

	void Open(const std::string& blockset_name);
	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetActivePalette(const std::string& name);
	void SetDrawTile(const Tile& tile);
	void Redraw();
	void RedrawTiles(int index = -1) const;
private:
	void OnZoomChange(wxCommandEvent& evt);
	void OnButtonClicked(wxCommandEvent& evt);
	void UpdateStatusBar();

	BlocksetEditorCtrl* m_editor;
	TilesetEditor* m_tileset;
	mutable wxAuiManager m_mgr;
	std::string m_title;

	wxStatusBar* m_statusbar = nullptr;
	wxSlider* m_zoomslider = nullptr;
	wxToolBar* m_toolbar = nullptr;

	wxDECLARE_EVENT_TABLE();
};

#endif // _BLOCKSET_EDITOR_FRAME_H_
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

	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);

	void ExportBin(const std::string& filename) const;
	void ExportPng(const std::string& filename) const;
	void ExportCsv(const std::string& filename) const;
	void ImportBin(const std::string& filename);
	void ImportCsv(const std::string& filename);
private:
	void OnZoomChange(wxCommandEvent& evt);
	void OnButtonClicked(wxCommandEvent& evt);
	void UpdateStatus();

	void OnExportBin();
	void OnExportPng();
	void OnExportCsv();
	void OnImportBin();
	void OnImportCsv();

	void FireUpdateStatusEvent(const std::string& data, int pane = 0);

	BlocksetEditorCtrl* m_editor;
	TilesetEditor* m_tileset;
	mutable wxAuiManager m_mgr;
	std::string m_title;
	std::shared_ptr<BlocksetEntry> m_blocks;
	std::shared_ptr<TilesetEntry> m_tiles;

	wxStatusBar* m_statusbar = nullptr;
	wxSlider* m_zoomslider = nullptr;
	wxToolBar* m_toolbar = nullptr;

	wxDECLARE_EVENT_TABLE();
};

#endif // _BLOCKSET_EDITOR_FRAME_H_
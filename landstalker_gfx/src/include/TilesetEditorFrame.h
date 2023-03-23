#ifndef _TILESETEDITORFRAME_H_
#define _TILESETEDITORFRAME_H_

#include <wx/wx.h>
#include <wx/aui/aui.h>

#include <vector>
#include <cstdint>
#include <memory>
#include <string>
#include <map>

#include "TilesetEditor.h"
#include "PaletteEditor.h"
#include "TileEditor.h"
#include "EditorFrame.h"
#include "GameData.h"


class TilesetEditorFrame : public EditorFrame
{
public:
	TilesetEditorFrame(wxWindow *parent);
	virtual ~TilesetEditorFrame();
	virtual void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();
	void SetActivePalette(std::string name);
	bool Open(std::vector<uint8_t>& pixels, bool uses_compression = false, int tile_width = 8, int tile_height = 8, int tile_bitdepth = 4);
	bool Open(const std::string& name);
	bool OpenAnimated(const std::string& name);

private:
	void OnZoom(wxCommandEvent& evt);
	void OnTileSelectionChanged(wxCommandEvent& evt);
	void OnTileEditRequested(wxCommandEvent& evt);
	void OnPaletteChanged(wxCommandEvent& evt);
	void OnPaletteColourSelect(wxCommandEvent& evt);
	void OnTileChanged(wxCommandEvent& evt);
	void OnTilesetChange(wxCommandEvent& evt);
	void OnTilePixelHover(wxCommandEvent& evt);

	void ToggleAlpha();
	void ToggleTileNums();
	void ToggleGrid();
	void InsertTileBefore();
	void InsertTileAfter();
	void ExtendTileset();
	void DeleteTile();
	void SwapTiles();
	void CutTile();
	void CopyTile();
	void PasteTile();

	void ToggleDrawGrid();
	void ClearTile();
	void SelectDrawPencil();

	void Save();
	void SaveAs();
	void New();
	void ImportFromBin();
	void ImportFromPng();
	void ImportFromRom();
	void ExportAsBin();
	void ExportAsPng();
	void ExportAll();
	void InjectIntoRom();

	void UpdateUI() const;
	void RefreshProperties(wxPropertyGridManager& props) const;

	TilesetEditor* m_tilesetEditor = nullptr;
	PaletteEditor* m_paletteEditor = nullptr;
	TileEditor* m_tileEditor = nullptr;
	mutable wxSlider* m_zoomslider = nullptr;
	mutable wxPGChoices m_palette_list;
	mutable wxPGChoices m_blocktype_list;
	std::shared_ptr<PaletteEntry> m_selected_palette;
	std::shared_ptr<Tileset> m_tileset;
	std::shared_ptr<TilesetEntry> m_tileset_entry;
	std::shared_ptr<AnimatedTilesetEntry> m_animated_tileset_entry;
	Tile m_tile;
	int m_inputBuffer;
	mutable wxAuiManager m_mgr;
	std::shared_ptr<GameData> m_gd;
	bool m_animated;

	std::string m_title;
	std::shared_ptr<wxFont> m_normal_font;
	std::shared_ptr<wxFont> m_bold_font;

	wxDECLARE_EVENT_TABLE();
};

#endif //_TILESETEDITORFRAME_H_

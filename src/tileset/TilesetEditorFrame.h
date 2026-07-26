#ifndef _TILESETEDITORFRAME_H_
#define _TILESETEDITORFRAME_H_

#include <wx/wx.h>
#include <wx/aui/aui.h>

#include <vector>
#include <cstdint>
#include <memory>
#include <string>
#include <map>

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <tileset/TilesetEditor.h>
#include <palettes/PaletteEditor.h>

// Small dockable pane that plays an animated tileset's frames in a loop. Defined in the .cpp
// as it is only used from there.
class AnimatedTilesetPreview;

class TilesetEditorFrame : public EditorFrame
{
public:
	TilesetEditorFrame(wxWindow *parent, ImageList* imglst);
	virtual ~TilesetEditorFrame();
	virtual void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	virtual void ClearMenu(wxMenuBar& menu) const override;
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
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
	void OnColourPicked(wxCommandEvent& evt);
	void OnPaletteColourHover(wxCommandEvent& evt);
	void OnTilesetChange(wxCommandEvent& evt);
	void OnTilePixelChanged(wxCommandEvent& evt);
	// Coalesces status bar rebuilds behind a short one-shot timer: hover events arrive per
	// mouse sample, and rebuilding the status bar for each one starves the input queue.
	void RequestStatusBarUpdate();
	void OnStatusBarTimer(wxTimerEvent& evt);

	// Shows the animation preview pane and starts it playing the given animated tileset. Hiding
	// stops playback and collapses the pane again.
	void ShowAnimationPreview(std::shared_ptr<Landstalker::AnimatedTileset> ats);
	void HideAnimationPreview();

	void ShowTilesetManagerDialog();
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
	void SelectDrawSelect();
	void SelectDrawPencil();
	void SelectDrawTool(TilesetEditor::Tool tool);

	void Save();
	void SaveAs();
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
	AnimatedTilesetPreview* m_animPreview = nullptr;
	mutable wxSlider* m_zoomslider = nullptr;
	wxTimer m_statusbar_timer;
	mutable wxPGChoices m_palette_list;
	mutable wxPGChoices m_blocktype_list;
	std::shared_ptr<Landstalker::PaletteEntry> m_selected_palette;
	std::shared_ptr<Landstalker::Tileset> m_tileset;
	std::shared_ptr<Landstalker::TilesetEntry> m_tileset_entry;
	std::shared_ptr<Landstalker::AnimatedTilesetEntry> m_animated_tileset_entry;
	// Set only for the end credit font, whose glyphs carry an editable advance width.
	std::shared_ptr<Landstalker::EndCreditFontEntry> m_font_entry;
	Landstalker::Tile m_tile;
	int m_inputBuffer;
	mutable wxAuiManager m_mgr;
	bool m_animated;

	std::string m_title;
	std::shared_ptr<wxFont> m_normal_font;
	std::shared_ptr<wxFont> m_bold_font;

	wxDECLARE_EVENT_TABLE();
};

#endif //_TILESETEDITORFRAME_H_

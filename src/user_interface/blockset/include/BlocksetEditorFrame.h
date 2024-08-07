#ifndef _BLOCKSET_EDITOR_FRAME_H_
#define _BLOCKSET_EDITOR_FRAME_H_

#include <wx/wx.h>
#include <string>
#include <map>
#include <memory>
#include <landstalker/palettes/include/Palette.h>
#include <landstalker/tileset/include/Tileset.h>
#include <landstalker/blockset/include/Block.h>
#include <landstalker/blockset/include/BlocksetCmp.h>
#include <user_interface/blockset/include/BlocksetEditorCtrl.h>
#include <user_interface/main/include/EditorFrame.h>
#include <user_interface/tileset/include/TilesetEditor.h>

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

	void ExportBin(const std::string& filename) const;
	void ExportPng(const std::string& filename) const;
	void ExportCsv(const std::string& filename) const;
	void ImportBin(const std::string& filename);
	void ImportCsv(const std::string& filename);
private:
	virtual void InitProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	void InitPaletteList() const;

	void OnZoomChange(wxCommandEvent& evt);
	void OnButtonClicked(wxCommandEvent& evt);
	void OnPaletteSelect(wxCommandEvent& evt);
	void OnBlockSelect(wxCommandEvent& evt);
	void OnTileSelect(wxCommandEvent& evt);
	void OnKeyPress(wxKeyEvent& evt);
	void ProcessEvent(int id);
	virtual void UpdateUI() const override;

	virtual void InitStatusBar(wxStatusBar& status) const;

	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	virtual void ClearMenu(wxMenuBar& menu) const;

	void OnExportBin();
	void OnExportPng();
	void OnExportCsv();
	void OnImportBin();
	void OnImportCsv();

	void FireUpdateStatusEvent(const std::string& data, int pane = 0);

	BlocksetEditorCtrl* m_editor = nullptr;
	TilesetEditor* m_tileset = nullptr;
	mutable wxAuiManager m_mgr;
	std::string m_title;
	std::shared_ptr<BlocksetEntry> m_blocks = nullptr;
	std::shared_ptr<TilesetEntry> m_tiles = nullptr;
	std::shared_ptr<PaletteEntry> m_palette = nullptr;
	int m_zoom = 1;

	std::optional<Tile> m_tileclipboard;
	std::optional<MapBlock> m_blockclipboard;
	int m_tileswap = -1;
	int m_blockswap = -1;

	mutable wxSlider* m_zoomslider = nullptr;
	mutable wxComboBox* m_palette_select = nullptr;
	mutable wxPGChoices m_palette_list;
	mutable std::vector<wxString> m_palette_vec;

	wxDECLARE_EVENT_TABLE();
};

#endif // _BLOCKSET_EDITOR_FRAME_H_
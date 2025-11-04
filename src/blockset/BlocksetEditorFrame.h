#ifndef _BLOCKSET_EDITOR_FRAME_H_
#define _BLOCKSET_EDITOR_FRAME_H_

#include <wx/wx.h>
#include <string>
#include <map>
#include <memory>
#include <landstalker/palettes/Palette.h>
#include <landstalker/tileset/Tileset.h>
#include <landstalker/blockset/Block.h>
#include <landstalker/blockset/BlocksetCmp.h>
#include <blockset/BlocksetEditorCtrl.h>
#include <main/EditorFrame.h>
#include <tileset/TilesetEditor.h>

class BlocksetEditorFrame : public EditorFrame
{
public:
	BlocksetEditorFrame(wxWindow* parent, ImageList* imglst);
	~BlocksetEditorFrame();

	void Open(const std::string& blockset_name);
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

	void SetActivePalette(const std::string& name);
	void SetDrawTile(const Landstalker::Tile& tile);
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
	std::shared_ptr<Landstalker::BlocksetEntry> m_blocks = nullptr;
	std::shared_ptr<Landstalker::TilesetEntry> m_tiles = nullptr;
	std::shared_ptr<Landstalker::PaletteEntry> m_palette = nullptr;
	int m_zoom = 1;

	std::optional<Landstalker::Tile> m_tileclipboard;
	std::optional<Landstalker::MapBlock> m_blockclipboard;
	int m_tileswap = -1;
	int m_blockswap = -1;

	mutable wxSlider* m_zoomslider = nullptr;
	mutable wxComboBox* m_palette_select = nullptr;
	mutable wxPGChoices m_palette_list;
	mutable std::vector<wxString> m_palette_vec;

	wxDECLARE_EVENT_TABLE();
};

#endif // _BLOCKSET_EDITOR_FRAME_H_
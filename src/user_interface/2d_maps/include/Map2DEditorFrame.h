#ifndef _MAP_2D_EDITOR_FRAME_H_
#define _MAP_2D_EDITOR_FRAME_H_

#include <user_interface/main/include/EditorFrame.h>
#include <user_interface/2d_maps/include/Map2DEditor.h>
#include <user_interface/tileset/include/TilesetEditor.h>

class Map2DEditorFrame : public EditorFrame
{
public:
	Map2DEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~Map2DEditorFrame();

	void Open(const std::string& map_name);

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetActivePalette(const std::string& name);
	void SetTileset(const std::string& name);
	void SetDrawTile(const Tile& tile);
	void Redraw();
	void RedrawTiles(int index = -1) const;

	bool ExportBin(const std::string& filename) const;
	bool ExportPng(const std::string& filename) const;
	bool ExportCsv(const std::string& filename) const;
	bool ImportBin(const std::string& filename, uint16_t base, int width = 0, int height = 0) const;
	bool ImportCsv(const std::string& filename) const;
private:
	enum class State
	{
		SELECT,
		PENCIL
	};
	void OnZoomChange(wxCommandEvent& evt);
	void OnTilesetZoomChange(wxCommandEvent& evt);
	void OnTileChanged(wxCommandEvent& evt);
	void OnTileHovered(wxCommandEvent& evt);
	void OnTileSelect(wxCommandEvent& evt);
	void OnTileEditRequested(wxCommandEvent& evt);
	void OnTilesetSelect(wxCommandEvent& evt);
	void OnPaletteSelect(wxCommandEvent& evt);
	void InitCombos() const;
	std::string PrettyPrintTile(const Map2DEditor::TilePosition& tp) const;
	std::string PrettyPrintMode() const;
	virtual void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	virtual void ClearMenu(wxMenuBar& menu) const;
	void UpdateUI() const;
	void RefreshProperties(wxPropertyGridManager& props) const;

	void OnExportBin();
	void OnExportPng();
	void OnExportCsv();
	void OnImportBin();
	void OnImportCsv();

	mutable wxAuiManager m_mgr;
	std::string m_title;
	TilesetEditor* m_tileset;
	Map2DEditor* m_mapedit;
	mutable wxSlider* m_zoomslider;
	mutable wxSlider* m_tileset_zoomslider;
	mutable wxComboBox* m_tileset_select;
	mutable wxComboBox* m_palette_select;
	mutable wxPGChoices m_tileset_list;
	mutable wxPGChoices m_palette_list;

	std::shared_ptr<Tilemap2DEntry> m_map;
	std::shared_ptr<TilesetEntry> m_tiles;
	std::shared_ptr<PaletteEntry> m_palette;
	Tile m_tile;
	int m_zoom;

	wxDECLARE_EVENT_TABLE();
};

#endif // _MAP_2D_EDITOR_FRAME_H_
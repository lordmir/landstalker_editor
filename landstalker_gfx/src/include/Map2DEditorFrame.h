#ifndef _MAP_2D_EDITOR_FRAME_H_
#define _MAP_2D_EDITOR_FRAME_H_

#include <EditorFrame.h>
#include <Map2DEditor.h>
#include <TilesetEditor.h>

class Map2DEditorFrame : public EditorFrame
{
public:
	Map2DEditorFrame(wxWindow* parent);
	virtual ~Map2DEditorFrame();

	void Open(const std::string& map_name);

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetActivePalette(const std::string& name);
	void SetTileset(const std::string& name);
	void SetDrawTile(const Tile& tile);
	void Redraw();
	void RedrawTiles(int index = -1) const;
private:
	enum class State
	{
		SELECT,
		PENCIL
	};
	void OnZoomChange(wxCommandEvent& evt);
	void OnButtonClicked(wxCommandEvent& evt);
	void OnTileChanged(wxCommandEvent& evt);
	void OnTileHovered(wxCommandEvent& evt);
	void OnTileSelect(wxCommandEvent& evt);
	void OnTileEditRequested(wxCommandEvent& evt);
	std::string PrettyPrintTile(const Map2DEditor::TilePosition& tp) const;
	std::string PrettyPrintMode() const;
	virtual void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	void UpdateUI() const;
	void RefreshProperties(wxPropertyGridManager& props) const;

	mutable wxAuiManager m_mgr;
	std::string m_title;
	TilesetEditor* m_tileset;
	Map2DEditor* m_mapedit;
	mutable wxSlider* m_zoomslider;

	std::shared_ptr<Tilemap2DEntry> m_map;
	std::shared_ptr<TilesetEntry> m_tiles;
	std::shared_ptr<PaletteEntry> m_palette;
	Tile m_tile;
	int m_zoom;

	wxDECLARE_EVENT_TABLE();
};

#endif // _MAP_2D_EDITOR_FRAME_H_
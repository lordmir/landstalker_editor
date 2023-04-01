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
private:
	mutable wxAuiManager m_mgr;
	std::string m_title;
	TilesetEditor* m_tileset;
	Map2DEditor* m_mapedit;

	std::shared_ptr<Tilemap2DEntry> m_map;
	std::shared_ptr<TilesetEntry> m_tiles;
	std::shared_ptr<PaletteEntry> m_palette;
	Tile m_tile;
};

#endif // _MAP_2D_EDITOR_FRAME_H_
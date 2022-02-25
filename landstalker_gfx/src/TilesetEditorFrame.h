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
#include "Palette.h"

class TilesetEditorFrame : public wxWindow
{
public:
	TilesetEditorFrame(wxWindow *parent);
	~TilesetEditorFrame();
	std::string GetStatusBarText() const;
	void SetPalettes(std::shared_ptr<std::map<std::string, Palette>> palettes);
	void SetActivePalette(const std::string& name);
	bool Open(std::vector<uint8_t>& pixels, bool uses_compression = false, int tile_width = 8, int tile_height = 8, int tile_bitdepth = 4);

private:
	void OnZoomChange(wxCommandEvent& evt);
	void OnTileSelectionChanged(wxCommandEvent& evt);
	void OnTileEditRequested(wxCommandEvent& evt);
	void OnPaletteChanged(wxCommandEvent& evt);
	void OnPaletteColourSelect(wxCommandEvent& evt);
	void OnTileChanged(wxCommandEvent& evt);
	void OnTilesetChange(wxCommandEvent& evt);

	TilesetEditor* m_tilesetEditor = nullptr;
	PaletteEditor* m_paletteEditor = nullptr;
	TileEditor* m_tileEditor = nullptr;
	std::shared_ptr<std::map<std::string, Palette>> m_palettes;
	std::string m_selected_palette;
	std::shared_ptr<Tileset> m_tileset;
	Tile m_tile;
	wxAuiManager m_mgr;

	std::string m_title;

	wxDECLARE_EVENT_TABLE();
};

#endif //_TILESETEDITORFRAME_H_

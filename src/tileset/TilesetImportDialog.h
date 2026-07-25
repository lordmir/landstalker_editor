#ifndef _TILESET_IMPORT_DIALOG_H_
#define _TILESET_IMPORT_DIALOG_H_

#include <wx/wx.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include <landstalker/main/GameData.h>
#include <landstalker/main/ImageBuffer.h>

class wxSpinCtrl;
class wxScrolledWindow;
class LookupChoiceControl;

// Interactive PNG-into-tileset import. Shows the sheet with the chosen palette applied plus a tile
// grid and unused-tile overlay, and lets the user set how many tiles to bring in. The palette
// section depends on the tileset:
//   - RoomMatch: a room tileset (>=4bpp). The 13 room colours are quantised and matched against the
//     existing room palettes; the user can pick one or name a new one to create.
//   - Overwrite: any other >=4bpp tileset. A single checkbox offers to overwrite the tileset's
//     current palette from the image, or leave it.
//   - None: a <4bpp tileset - no palette can be imported.
class TilesetImportDialog : public wxDialog
{
public:
	enum class PaletteMode { None, RoomMatch, Overwrite };

	// `current_palette` is the tileset's present palette (Genesis), used to preview when the import
	// is not recolouring. For RoomMatch, `palette_names` lists the existing room palettes; for
	// Overwrite, `overwrite_name` is the palette the checkbox offers to overwrite.
	TilesetImportDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
		const Landstalker::ImageBuffer::IndexedImage& image, int tile_w, int tile_h, int bpp,
		int max_tiles, PaletteMode mode, const wxArrayString& palette_names,
		const std::string& overwrite_name, const std::array<uint16_t, 16>& current_palette);

	int GetTileCount() const;
	PaletteMode GetMode() const { return m_mode; }

	// RoomMatch results.
	bool RoomCreateNew() const;
	int RoomExistingIndex() const;        // valid when !RoomCreateNew()
	std::wstring RoomNewName() const;      // display name for a new room palette
	std::vector<uint16_t> RoomColours() const;  // 13 Genesis colours for a new room palette

	// Overwrite result.
	bool Overwrite() const;

private:
	void Preprocess(const wxArrayString& palette_names);
	void BuildControls(int max_tiles, const wxArrayString& palette_names,
		const std::string& overwrite_name);
	void UpdatePaletteEnable();
	void RebuildPreview();

	std::array<uint16_t, 16> DisplayPalette() const;
	std::vector<uint16_t> ImageRoomColours() const;   // image palette indices 2-14
	std::vector<uint16_t> CurrentRoomColours() const; // from the selected room palette, or image
	int FindRoomPaletteIndex(const wxString& name) const;
	int MatchRoomPalette() const;
	int ChooseZoom() const;
	wxBitmap MakeSheetBitmap(const std::array<uint16_t, 16>& pal, int zoom) const;
	wxBitmap MakeSwatchBitmap(const std::array<uint16_t, 16>& pal) const;

	std::shared_ptr<Landstalker::GameData> m_gd;
	Landstalker::ImageBuffer::IndexedImage m_image;
	std::array<uint16_t, 16> m_image_palette;    // processed image palette (fixed + downsampled)
	std::array<uint16_t, 16> m_current_palette;  // the tileset's present palette
	wxArrayString m_room_names;
	PaletteMode m_mode;
	int m_tile_w = 8;
	int m_tile_h = 8;
	int m_bpp = 4;
	bool m_updating = false;

	wxSpinCtrl* m_count = nullptr;
	LookupChoiceControl* m_room_choice = nullptr;
	wxCheckBox* m_overwrite_check = nullptr;
	wxScrolledWindow* m_preview_scroll = nullptr;
	wxStaticBitmap* m_preview_bmp = nullptr;
	wxStaticBitmap* m_orig_swatch = nullptr;
	wxStaticBitmap* m_sel_swatch = nullptr;
};

#endif // _TILESET_IMPORT_DIALOG_H_

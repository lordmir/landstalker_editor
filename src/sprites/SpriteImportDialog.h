#ifndef _SPRITE_IMPORT_DIALOG_H_
#define _SPRITE_IMPORT_DIALOG_H_

#include <wx/wx.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include <landstalker/main/GameData.h>
#include <landstalker/main/ImageBuffer.h>
#include <landstalker/misc/Point.h>

class wxSpinCtrl;
class wxScrolledWindow;
class LookupChoiceControl;

// Interactive sprite-sheet import. Shows the sheet with the chosen palette applied plus grid,
// origin and unused-frame overlays, and lets the user set the tiling geometry, names and the
// low/high palette each frame draws with. Preprocesses the image on open: the fixed slots
// (0 transparent, 1 = 0xCCC, 15 black) are forced and every other colour is quantised to the
// Mega Drive's 3-bit-per-channel depth. See low = indices 2-7, high = indices 8-14.
class SpriteImportDialog : public wxDialog
{
public:
	// What the dialog decided for one palette half (low or high).
	struct PaletteResult
	{
		bool used = false;                 // the half has a palette at all
		bool create_new = false;           // its colours are not an existing palette; make one
		int existing_index = -1;           // index into the low/high palette list when !create_new
		std::wstring new_name;             // display name to give a newly created palette
		std::vector<uint16_t> colours;     // Genesis colours (unlocked order) for a new palette
	};

	// `image` must be an indexed PNG (see ImageBuffer::ReadIndexedPNG). The def_* values seed the
	// controls; pass a YAML export's geometry when one was found, else 32/32/-1/16/16 (a count of
	// -1 or 0 means "every whole cell").
	// When `editable_name` is false (importing into an existing sprite) the name fields show the
	// sprite's current names, read-only, and are not validated - the id keeps its identity.
	SpriteImportDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
		const Landstalker::ImageBuffer::IndexedImage& image, const std::string& suggested_name,
		int def_cell_w, int def_cell_h, int def_count, int def_origin_x, int def_origin_y,
		bool metadata_found, const std::vector<std::string>& animation_names,
		bool editable_name = true, const std::wstring& display_suggestion = std::wstring());

	int GetCellWidth() const;
	int GetCellHeight() const;
	int GetFrameCount() const;
	Landstalker::Point GetOrigin() const;
	std::string GetInternalName() const;
	std::wstring GetDisplayName() const;
	PaletteResult GetLowResult() const;
	PaletteResult GetHighResult() const;

private:
	void Preprocess();
	void BuildControls(const std::string& suggested_name, int def_cell_w, int def_cell_h,
		int def_count, int def_ox, int def_oy, bool metadata_found,
		const std::vector<std::string>& animation_names, bool editable_name,
		const std::wstring& display_suggestion);
	void DetectPaletteUsage();
	void UpdatePaletteEnable();
	void RebuildPreview();

	std::array<uint16_t, 16> BuildSelectedPalette() const;
	std::vector<uint16_t> ImageHalfColours(bool high) const;
	std::vector<uint16_t> CurrentHalfColours(bool high) const;
	int FindExistingPaletteIndex(bool high, const wxString& name) const;
	int MatchExisting(bool high) const;
	PaletteResult HalfResult(bool high) const;
	int ChooseZoom() const;
	wxBitmap MakeSheetBitmap(const std::array<uint16_t, 16>& pal, int zoom) const;
	wxBitmap MakeSwatchBitmap(const std::array<uint16_t, 16>& pal) const;

	void OnOk(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	Landstalker::ImageBuffer::IndexedImage m_image;
	std::array<uint16_t, 16> m_image_palette;   // processed image palette (fixed + downsampled)
	wxArrayString m_low_names;
	wxArrayString m_high_names;
	bool m_updating = false;
	bool m_editable_name = true;

	wxTextCtrl* m_internal = nullptr;
	wxTextCtrl* m_display = nullptr;
	wxSpinCtrl* m_tile_w = nullptr;
	wxSpinCtrl* m_tile_h = nullptr;
	wxSpinCtrl* m_count = nullptr;
	wxSpinCtrl* m_origin_x = nullptr;
	wxSpinCtrl* m_origin_y = nullptr;
	wxCheckBox* m_low_check = nullptr;
	wxCheckBox* m_high_check = nullptr;
	LookupChoiceControl* m_low_choice = nullptr;
	LookupChoiceControl* m_high_choice = nullptr;
	wxScrolledWindow* m_preview_scroll = nullptr;
	wxStaticBitmap* m_preview_bmp = nullptr;
	wxStaticBitmap* m_orig_swatch = nullptr;
	wxStaticBitmap* m_sel_swatch = nullptr;
};

#endif // _SPRITE_IMPORT_DIALOG_H_

#ifndef _PALETTE_EDITOR_H_
#define _PALETTE_EDITOR_H_

#include <wx/wx.h>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <landstalker/palettes/Palette.h>
#include <landstalker/main/GameData.h>
#include <landstalker/main/DataTypes.h>

class PaletteEditor : public wxWindow
{
public:
	PaletteEditor(wxWindow* parent);
	~PaletteEditor();

	void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	void SelectPalette(const std::string& name);
	void SelectPalette(std::shared_ptr<Landstalker::Palette> pal);
	void SetBitsPerPixel(uint8_t bpp);
	void SetColourIndicies(const std::vector<uint8_t>& entries);
	void DisableEntries(const std::vector<bool>& entries);
	const std::vector<bool>& GetDisabledEntries() const;
	void EnableAllEntries();

	bool SetPrimaryColour(int colour, bool send_event = true);
	int GetPrimaryColour() const;
	bool SetSecondaryColour(int colour, bool send_event = true);
	int GetSecondaryColour() const;

	int GetHoveredColour() const;

private:
	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseDown(wxMouseEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnMouseEnter(wxMouseEvent& evt);
	void ForceRedraw();

	int ConvertXYToColour(const wxPoint& p) const;
	wxBrush GetBrush(int index);
	void InitialiseBrushes();
	void FireEvent(const wxEventType& e, const std::string& data);
	void OnHover(int colour);
	int GetColour(int index) const;

	std::shared_ptr<Landstalker::GameData> m_gd;
	std::string m_selected_palette_name;
	std::shared_ptr<Landstalker::PaletteEntry> m_selected_palette_entry;
	std::shared_ptr<Landstalker::Palette> m_selected_palette;
	std::unique_ptr<wxBrush> m_alpha_brush;
	std::unique_ptr<wxBitmap> m_stipple;
	Landstalker::Palette m_default_palette;
	std::vector<bool> m_disabled;
	std::vector<bool> m_locked;
	std::vector<uint8_t> m_indicies;
	uint8_t m_bpp;

	int m_pri_colour;
	int m_sec_colour;
	int m_hovered_colour;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_PALETTE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_PALETTE_COLOUR_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_PALETTE_COLOUR_HOVER, wxCommandEvent);

#endif // _PALETTE_EDITOR_H_
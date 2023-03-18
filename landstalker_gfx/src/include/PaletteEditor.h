#ifndef _PALETTE_EDITOR_H_
#define _PALETTE_EDITOR_H_

#include <wx/wx.h>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include "Palette.h"
#include "GameData.h"
#include "DataTypes.h"

class PaletteEditor : public wxWindow
{
public:
	PaletteEditor(wxWindow* parent);
	~PaletteEditor();

	void SetGameData(std::shared_ptr<GameData> gd);
	void SelectPalette(const std::string& name);
	void DisableEntries(const std::vector<bool>& entries);
	const std::vector<bool>& GetDisabledEntries() const;
	void EnableAllEntries();

	bool SetPrimaryColour(int colour, bool send_event = true);
	int GetPrimaryColour() const;
	bool SetSecondaryColour(int colour, bool send_event = true);
	int GetSecondaryColour() const;

private:
	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseDown(wxMouseEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);
	void ForceRedraw();

	int ConvertXYToColour(const wxPoint& p) const;
	wxBrush GetBrush(int index);
	void InitialiseBrushes();
	void FireEvent(const wxEventType& e, const std::string& data);

	std::shared_ptr<GameData> m_gd;
	std::string m_selected_palette_name;
	std::shared_ptr<PaletteEntry> m_selected_palette_entry;
	std::shared_ptr<Palette> m_selected_palette;
	wxBrush* m_alpha_brush = nullptr;
	Palette m_default_palette;
	std::vector<bool> m_disabled;
	std::vector<bool> m_locked;

	int m_pri_colour;
	int m_sec_colour;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_PALETTE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_PALETTE_COLOUR_SELECT, wxCommandEvent);

#endif // _PALETTE_EDITOR_H_
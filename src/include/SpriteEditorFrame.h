#ifndef _SPRITEEDITORFRAME_H_
#define _SPRITEEDITORFRAME_H_

#include <wx/wx.h>

#include <vector>
#include <cstdint>
#include <memory>
#include <string>
#include <map>

#include "SpriteEditorCtrl.h"
#include "EditorFrame.h"
#include "Palette.h"
#include "PaletteEditor.h"
#include "TileEditor.h"

class SpriteEditorFrame : public EditorFrame
{
public:
	SpriteEditorFrame(wxWindow* parent, ImageList* imglst);
	~SpriteEditorFrame();

	bool Open(uint8_t spr, int frame = -1, int anim = -1, int ent = -1);
	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetColour(int c);
	void SetActivePalette(const std::string& name);
	void Redraw() const;
	void RedrawTiles(int index = -1) const;

	bool Save();
	bool SaveAs(wxString filename, bool compressed = false);
	bool Open(wxString filename);
	bool Open(std::vector<uint8_t>& pixels);
	bool New(int r, int c);
	const std::string& GetFilename() const;

	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);

	void ExportFrm(const std::string& filename) const;
	void ExportTiles(const std::string& filename) const;
	void ExportVdpSpritemap(const std::string& filename) const;
	void ExportPng(const std::string& filename) const;
	void ImportFrm(const std::string& filename);
	void ImportTiles(const std::string& filename);
	void ImportVdpSpritemap(const std::string& filename);
private:
	void OnZoomChange(wxCommandEvent& evt);
	void OnTileHovered(wxCommandEvent& evt);
	void OnTileSelected(wxCommandEvent& evt);
	void OnTileChanged(wxCommandEvent& evt);
	void OnTileEditRequested(wxCommandEvent& evt);
	void OnButtonClicked(wxCommandEvent& evt);
	void OnPaletteColourSelect(wxCommandEvent& evt);
	void OnPaletteColourHover(wxCommandEvent& evt);
	void OnTilePixelHover(wxCommandEvent& evt);

	void OnExportFrm();
	void OnExportTiles();
	void OnExportVdpSpritemap();
	void OnExportPng();
	void OnImportFrm();
	void OnImportTiles();
	void OnImportVdpSpritemap();

	void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const;

	SpriteEditorCtrl* m_spriteeditor = nullptr;
	PaletteEditor* m_paledit = nullptr;
	TileEditor* m_tileedit = nullptr;
	mutable wxAuiManager m_mgr;
	std::string m_title;
	std::shared_ptr<SpriteFrameEntry> m_sprite;
	std::shared_ptr<Palette> m_palette;

	wxStatusBar* m_statusbar = nullptr;
	wxSlider* m_zoomslider = nullptr;
	wxToolBar* m_toolbar = nullptr;
	std::string m_filename;

	std::string m_name;

	wxDECLARE_EVENT_TABLE();
};

#endif //_SPRITEEDITORFRAME_H_

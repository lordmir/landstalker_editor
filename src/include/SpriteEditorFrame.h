#ifndef _SPRITEEDITORFRAME_H_
#define _SPRITEEDITORFRAME_H_

#include <wx/wx.h>

#include <vector>
#include <cstdint>
#include <memory>
#include <string>
#include <map>

#include "SpriteFrameEditorCtrl.h"
#include "SpriteAnimationEditorCtrl.h"
#include "EditorFrame.h"
#include "Palette.h"
#include "PaletteEditor.h"
#include "TileEditor.h"
#include "SubspriteControlFrame.h"
#include "FrameControlFrame.h"
#include "AnimationControlFrame.h"
#include "AnimationFrameControlFrame.h"

class SpriteEditorFrame : public EditorFrame
{
public:
	SpriteEditorFrame(wxWindow* parent, ImageList* imglst);
	~SpriteEditorFrame();

	bool Open(uint8_t spr, int frame = -1, int anim = -1, int ent = -1);
	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetActivePalette(const std::string& name);
	void SetActivePalette(const std::vector<std::string>& names);
	void Redraw() const;
	void RedrawTiles(int index = -1) const;
	void Update();

	bool Save();
	bool SaveAs(wxString filename, bool compressed = false);
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
	virtual void InitProperties(wxPropertyGridManager& props) const;
	void RefreshLists() const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);

	void OnFrameSelect(wxCommandEvent& evt);
	void OnFrameAdd(wxCommandEvent& evt);
	void OnFrameDelete(wxCommandEvent& evt);
	void OnFrameMoveUp(wxCommandEvent& evt);
	void OnFrameMoveDown(wxCommandEvent& evt);
	void OnSubSpriteSelect(wxCommandEvent& evt);
	void OnSubSpriteAdd(wxCommandEvent& evt);
	void OnSubSpriteDelete(wxCommandEvent& evt);
	void OnSubSpriteMoveUp(wxCommandEvent& evt);
	void OnSubSpriteMoveDown(wxCommandEvent& evt);
	void OnAnimationSelect(wxCommandEvent& evt);
	void OnAnimationAdd(wxCommandEvent& evt);
	void OnAnimationDelete(wxCommandEvent& evt);
	void OnAnimationMoveUp(wxCommandEvent& evt);
	void OnAnimationMoveDown(wxCommandEvent& evt);
	void OnAnimationFrameSelect(wxCommandEvent& evt);
	void OnAnimationFrameAdd(wxCommandEvent& evt);
	void OnAnimationFrameDelete(wxCommandEvent& evt);
	void OnAnimationFrameMoveUp(wxCommandEvent& evt);
	void OnAnimationFrameMoveDown(wxCommandEvent& evt);

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

	SpriteFrameEditorCtrl* m_spriteeditor = nullptr;
	SpriteAnimationEditorCtrl* m_spriteanimeditor = nullptr;
	PaletteEditor* m_paledit = nullptr;
	TileEditor* m_tileedit = nullptr;
	FrameControlFrame* m_framectrl = nullptr;
	SubspriteControlFrame* m_subspritectrl = nullptr;
	AnimationControlFrame* m_animctrl = nullptr;
	AnimationFrameControlFrame* m_animframectrl = nullptr;
	mutable wxAuiManager m_mgr;
	mutable wxAuiNotebook* m_nb;
	std::string m_title;

	mutable wxPGChoices m_hi_palettes;
	mutable wxPGChoices m_lo_palettes;
	mutable wxPGChoices m_misc_palettes;
	mutable wxPGChoices m_idle_frame_count_options;
	mutable wxPGChoices m_idle_frame_source_options;
	mutable wxPGChoices m_jump_frame_source_options;
	mutable wxPGChoices m_walk_frame_count_options;
	mutable wxPGChoices m_take_damage_frame_source_options;

	std::shared_ptr<SpriteFrameEntry> m_sprite;
	std::shared_ptr<Palette> m_palette;

	wxStatusBar* m_statusbar = nullptr;
	wxSlider* m_zoomslider = nullptr;
	wxToolBar* m_toolbar = nullptr;
	mutable bool m_reset_props = false;
	std::string m_filename;
	int m_frame = -1;
	int m_anim = -1;

	std::string m_name;

	wxDECLARE_EVENT_TABLE();
};

#endif //_SPRITEEDITORFRAME_H_

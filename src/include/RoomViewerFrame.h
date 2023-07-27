#ifndef _ROOM_VIEWER_FRAME_H_
#define _ROOM_VIEWER_FRAME_H_

#include "EditorFrame.h"
#include "LayerControlFrame.h"
#include "EntityControlFrame.h"
#include "WarpControlFrame.h"
#include "TileSwapControlFrame.h"
#include "HeightmapEditorCtrl.h"
#include "BlocksetEditorCtrl.h"
#include "Map3DEditor.h"
#include "GameData.h"
#include <memory>

class RoomViewerCtrl;

namespace RoomEdit
{
	enum class Mode : uint8_t
	{
		NORMAL,
		HEIGHTMAP,
		BACKGROUND,
		FOREGROUND
	};
}

class RoomViewerFrame : public EditorFrame
{
public:

	RoomViewerFrame(wxWindow* parent, ImageList* imglst);
	virtual ~RoomViewerFrame();

	RoomEdit::Mode GetMode() const { return m_mode; }
	void SetMode(RoomEdit::Mode mode);
	void UpdateFrame();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetRoomNum(uint16_t roomnum);
	uint16_t GetRoomNum() const { return m_roomnum; }

	bool ExportBin(const std::string& path);
	bool ExportCsv(const std::array<std::string, 3>& paths);
	bool ExportTmx(const std::string& tmx_path, const std::string& bs_path, uint16_t roomnum);
	bool ExportAllTmx(const std::string& dir);
	bool ExportPng(const std::string& path);
	bool ImportBin(const std::string& path);
	bool ImportCsv(const std::array<std::string, 3>& paths);
	bool ImportTmx(const std::string& paths, uint16_t roomnum);
	bool ImportAllTmx(const std::string& dir);

	bool HandleKeyDown(unsigned int key, unsigned int modifiers);

	void ShowFlagDialog();
	void ShowChestsDialog();
	void ShowCharDialog();
	void ShowErrorDialog();
private:
	virtual void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
	void RefreshLists() const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	void OnExportBin();
	void OnExportCsv();
	void OnExportTmx();
	void OnExportAllTmx();
	void OnExportPng();
	void OnImportBin();
	void OnImportCsv();
	void OnImportTmx();
	void OnImportAllTmx();
	void UpdateUI() const;

	void OnKeyDown(wxKeyEvent& evt);
	void OnZoomChange(wxCommandEvent& evt);
	void OnOpacityChange(wxCommandEvent& evt);

	void OnEntityUpdate(wxCommandEvent& evt);
	void OnEntitySelect(wxCommandEvent& evt);
	void OnEntityOpenProperties(wxCommandEvent& evt);
	void OnEntityAdd(wxCommandEvent& evt);
	void OnEntityDelete(wxCommandEvent& evt);
	void OnEntityMoveUp(wxCommandEvent& evt);
	void OnEntityMoveDown(wxCommandEvent& evt);

	void OnWarpUpdate(wxCommandEvent& evt);
	void OnWarpSelect(wxCommandEvent& evt);
	void OnWarpOpenProperties(wxCommandEvent& evt);
	void OnWarpAdd(wxCommandEvent& evt);
	void OnWarpDelete(wxCommandEvent& evt);

	void TileSwapRefresh();
	void OnSwapUpdate(wxCommandEvent& evt);
	void OnSwapSelect(wxCommandEvent& evt);
	void OnSwapAdd(wxCommandEvent& evt);
	void OnSwapDelete(wxCommandEvent& evt);
	void OnDoorUpdate(wxCommandEvent& evt);
	void OnDoorSelect(wxCommandEvent& evt);
	void OnDoorAdd(wxCommandEvent& evt);
	void OnDoorDelete(wxCommandEvent& evt);

	void OnHeightmapUpdate(wxCommandEvent& evt);
	void OnHeightmapMove(wxCommandEvent& evt);
	void OnHeightmapSelect(wxCommandEvent& evt);
	void OnHMTypeSelect(wxCommandEvent& evt);
	void OnHMZoom(wxCommandEvent& evt);

	void OnBlockSelect(wxCommandEvent& evt);
	void OnMapUpdate(wxCommandEvent& evt);

	void OnSize(wxSizeEvent& evt);
	void OnTabChange(wxAuiNotebookEvent& evt);

	void FireUpdateStatusEvent(const std::string& data, int pane = 0);
	void FireEvent(const wxEventType& e);
	void FireEvent(const wxEventType& e, const std::string& userdata);

	void SetPaneSizes();

	RoomEdit::Mode m_mode;
	mutable wxAuiManager m_mgr;
	mutable wxAuiNotebook* m_nb;
	std::string m_title;
	RoomViewerCtrl* m_roomview;
	HeightmapEditorCtrl* m_hmedit;
	Map3DEditor* m_bgedit;
	Map3DEditor* m_fgedit;
	LayerControlFrame* m_layerctrl;
	EntityControlFrame* m_entityctrl;
	WarpControlFrame* m_warpctrl;
	TileSwapControlFrame* m_swapctrl;
	BlocksetEditorCtrl* m_blkctrl;

	std::shared_ptr<GameData> m_g;
	uint16_t m_roomnum;
	double m_zoom;

	bool m_layerctrl_visible;
	bool m_entityctrl_visible;
	bool m_warpctrl_visible;
	bool m_swapctrl_visible;
	bool m_blkctrl_visible;

	mutable bool m_sizes_set;
	mutable bool m_reset_props;
	mutable wxPGChoices m_palettes;
	mutable wxPGChoices m_bgms;
	mutable wxPGChoices m_tilesets;
	mutable wxPGChoices m_pri_blocksets;
	mutable wxPGChoices m_sec_blocksets;
	mutable wxPGChoices m_maps;
	mutable wxPGChoices m_rooms;
	mutable wxPGChoices m_menustrings;

	wxDECLARE_EVENT_TABLE();
};

#endif // _ROOM_VIEWER_FRAME_H_

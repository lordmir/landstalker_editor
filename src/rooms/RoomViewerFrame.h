#ifndef _ROOM_VIEWER_FRAME_H_
#define _ROOM_VIEWER_FRAME_H_

#include <memory>
#include <vector>

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <rooms/LayerControlFrame.h>
#include <rooms/EntityControlFrame.h>
#include <rooms/WarpControlFrame.h>
#include <rooms/TileSwapControlFrame.h>
#include <blockset/BlocksetEditorCtrl.h>
#include <rooms/TileSwapDialog.h>
#include <rooms/gpu/GLCanvasInputTypes.h>

class GLCanvas;
class EntityPropertiesWindow;

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
	void SetDirectionInputMode(GLCanvasDirectionInputMode mode);
	void UpdateFrame();

	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

	void SetRoomNum(uint16_t roomnum);
	uint16_t GetRoomNum() const { return m_roomnum; }

	bool ExportBin(const std::string& path);
	bool ExportCsv(const std::array<std::string, 3>& paths);
	bool ExportAllCsv(const std::string& dir);
	bool ExportTmx(const std::string& tmx_path, const std::string& bs_path, uint16_t roomnum);
	bool ExportAllTmx(const std::string& dir);
	bool ExportPng(const std::string& path);
	bool ExportAllRoomsTmx(const std::string& dir);
	bool ExportRoomTmx(const std::string& tmx_path, const std::string& bs_path, uint16_t roomnum);
	bool ExportRoomMetadata(const std::string& path, uint16_t roomnum);
	bool ExportAllRoomMetadata(const std::string& dir);
	bool ImportRoomMetadata(const std::string& path, uint16_t roomnum);
	bool ImportBin(const std::string& path);
	bool ImportCsv(const std::array<std::string, 3>& paths);
	bool ImportTmx(const std::string& paths, uint16_t roomnum);
	bool ImportAllTmx(const std::string& dir);

	bool HandleKeyDown(unsigned int key, unsigned int modifiers);

	void ShowFlagDialog();
	void ShowChestsDialog();
	void ShowCharDialog();
	void ShowTileswapDialog(bool force = false, TileSwapDialog::PageType type = TileSwapDialog::PageType::SWAPS, int row = -1);
	void ShowErrorDialog();
	void ShowMapManagerDialog();
	void ShowRoomManagerDialog();
	void ShowTilesetManagerDialog();
	void ShowBlocksetManagerDialog();
private:
	// Applies the outcome of a manager dialog: refreshes the view if it changed anything,
	// and navigates to the room the user double-clicked.
	void ApplyManagerDialogResult(bool changed, int room_to_open);
	virtual void InitStatusBar(wxStatusBar& status) const;
	virtual void UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const;
	virtual void InitProperties(wxPropertyGridManager& props) const;
	void RefreshObjectLists();
	void RefreshLists() const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	virtual void OnStatusBarClick(wxStatusBar& status, int field);

	void OnExportBin();
	void OnExportCsv();
	void OnExportAllCsv();
	void OnExportTmx();
	void OnExportAllTmx();
	void OnExportRoomTmx();
	void OnExportAllRoomsTmx();
	void OnExportRoomMetadata();
	void OnExportAllRoomMetadata();
	void OnImportRoomMetadata();
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
	void OnSwapMoveUp(wxCommandEvent& evt);
	void OnSwapMoveDown(wxCommandEvent& evt);
	void OnSwapProperties(wxCommandEvent& evt);
	void OnDoorUpdate(wxCommandEvent& evt);
	void OnDoorSelect(wxCommandEvent& evt);
	void OnDoorAdd(wxCommandEvent& evt);
	void OnDoorDelete(wxCommandEvent& evt);
	void OnDoorMoveUp(wxCommandEvent& evt);
	void OnDoorMoveDown(wxCommandEvent& evt);
	void OnDoorProperties(wxCommandEvent& evt);

	void OnHMTypeSelect(wxCommandEvent& evt);
	void OnHMZoom(wxCommandEvent& evt);

	void OnBlockSelect(wxCommandEvent& evt);
	void OnGpuEditorModeChange(wxCommandEvent& evt);
	void OnOpenRoomActions(wxCommandEvent& evt);
	void OnOpenRoomShop(wxCommandEvent& evt);
	void OnGpuLayerOpacityChange(wxCommandEvent& evt);
	void OnGpuLayerBlockSelect(wxCommandEvent& evt);
	void OnGpuHeightmapTargetChange(wxCommandEvent& evt);

	void OnSize(wxSizeEvent& evt);

	void FireRenameNavItemEvent(const std::wstring& old_name, const std::wstring& new_name);
	void FireEvent(const wxEventType& e);
	void FireEvent(const wxEventType& e, const std::string& userdata);
	void FireEvent(const wxEventType& e, int userdata);

	void SyncGpuViewControls();
	void SyncGpuViewLayerControls();
	bool IsGpuViewSelected() const;
	void SetGpuEditorMode(RoomEdit::Mode mode, bool select_gpu_page = true);
	void SyncFrameModeFromGpuView();
	std::vector<Landstalker::Entity> GetRoomEntities() const;
	std::vector<Landstalker::WarpList::Warp> GetRoomWarps() const;
	std::vector<std::string> GetRoomErrors() const;
	void UpdateEntityProperties(int entity);
	void UpdateWarpProperties(int warp);

	RoomEdit::Mode m_mode;
	mutable wxAuiManager m_mgr;
	std::string m_title;
	GLCanvas* m_gpuview;
	GLCanvasDirectionInputMode m_direction_input_mode;
	LayerControlFrame* m_layerctrl;
	EntityControlFrame* m_entityctrl;
	WarpControlFrame* m_warpctrl;
	TileSwapControlFrame* m_swapctrl;
	BlocksetEditorCtrl* m_blkctrl;
	// Cached entity properties dialog - expensive to construct, so created once per loaded
	// game and reused across opens (see UpdateEntityProperties). Destroyed on game data change.
	EntityPropertiesWindow* m_entity_dialog = nullptr;

	std::shared_ptr<Landstalker::GameData> m_g;
	uint16_t m_roomnum;

	// Cached summary text for status bar field 4, recomputed in UpdateFrame() from
	// GetRoomErrors() rather than on every status bar refresh.
	std::string m_room_error_status;

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

wxDECLARE_EVENT(EVT_ENTITY_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_WARP_UPDATE, wxCommandEvent);

#endif // _ROOM_VIEWER_FRAME_H_

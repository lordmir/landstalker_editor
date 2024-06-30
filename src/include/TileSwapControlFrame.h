#ifndef _TILE_SWAP_CONTROL_FRAME_H_
#define _TILE_SWAP_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <wx/tglbtn.h>
#include <vector>
#include <memory>
#include <TileSwaps.h>
#include <Doors.h>
#include <SelectionControlFrame.h>
#include <GameData.h>

class RoomViewerFrame;
namespace RoomEdit
{
	enum class Mode : uint8_t;
}

class TileSwapControlFrame : public SelectionControlFrame
{
public:
	enum class ID
	{
		TILESWAP = 20000,
		DOOR
	};
	TileSwapControlFrame(RoomViewerFrame* parent, ImageList* imglst);
	virtual ~TileSwapControlFrame();
	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();

	void SetSwaps(const std::vector<TileSwap>& swaps, const std::vector<Door>& doors, uint16_t m_roomnum);
	void ResetSwaps();
	ID GetPage() const;
	void SetPage(ID id);
	virtual int GetMaxSelection() const;
	void SetMode(RoomEdit::Mode mode);
protected:
	virtual void Select();
	virtual void Add();
	virtual void Delete();
	virtual void MoveUp();
	virtual void MoveDown();
	virtual void OpenElement();
	virtual bool HandleKeyPress(unsigned int key, unsigned int modifiers);
	virtual void UpdateOtherControls();
	virtual std::string MakeLabel(int index) const;

private:
	void OnToggle(wxCommandEvent& evt);

	std::vector<TileSwap> m_swaps;
	std::vector<Door> m_doors;
	RoomViewerFrame* m_rvf;

	wxBitmapToggleButton* m_swap_btn;
	wxBitmapToggleButton* m_door_btn;

	ID m_selected;
	RoomEdit::Mode m_mode;
	uint16_t m_roomnum;
	std::shared_ptr<GameData> m_gd;
};

wxDECLARE_EVENT(EVT_TILESWAP_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_OPEN_PROPERTIES, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_MOVE_DOWN, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_OPEN_PROPERTIES, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_DOOR_MOVE_DOWN, wxCommandEvent);

#endif // _TILE_SWAP_CONTROL_FRAME_H_

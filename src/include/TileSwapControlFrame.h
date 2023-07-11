#ifndef _TILE_SWAP_CONTROL_FRAME_H_
#define _TILE_SWAP_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <TileSwaps.h>
#include <SelectionControlFrame.h>

class RoomViewerFrame;

class TileSwapControlFrame : public SelectionControlFrame
{
public:
	TileSwapControlFrame(RoomViewerFrame* parent, ImageList* imglst);
	virtual ~TileSwapControlFrame();

	void SetSwaps(const std::vector<TileSwap>& swaps);
	void ResetSwaps();
	virtual int GetMaxSelection() const;
protected:
	virtual void Select();
	virtual void Add();
	virtual void Delete();
	virtual void MoveUp();
	virtual void MoveDown();
	virtual void OpenElement();
	virtual bool HandleKeyPress(unsigned int key, unsigned int modifiers);
	virtual std::string MakeLabel(int index) const;
private:
	std::vector<TileSwap> m_swaps;

	RoomViewerFrame* m_rvf;
};

wxDECLARE_EVENT(EVT_TILESWAP_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_OPEN_PROPERTIES, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESWAP_MOVE_DOWN, wxCommandEvent);

#endif // _TILE_SWAP_CONTROL_FRAME_H_

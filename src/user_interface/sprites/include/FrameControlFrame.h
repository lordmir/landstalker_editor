#ifndef _FRAME_CONTROL_FRAME_H_
#define _FRAME_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <memory>
#include <string>

#include <landstalker/main/include/GameData.h>
#include <user_interface/main/include/ImageList.h>
#include <user_interface/misc/include/SelectionControlFrame.h>

class SpriteEditorFrame;

class FrameControlFrame : public SelectionControlFrame
{
public:
	FrameControlFrame(SpriteEditorFrame* parent, ImageList* imglst);
	virtual ~FrameControlFrame();

	void SetSprite(uint8_t sprite);
	void ResetSprite();
	virtual int GetMaxSelection() const;

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();
protected:
	virtual void Select();
	virtual void Add();
	virtual void Delete();
	virtual void MoveUp();
	virtual void MoveDown();
	virtual void OpenElement();
	virtual std::wstring MakeLabel(int index) const;
	virtual bool HandleKeyPress(unsigned int key, unsigned int modifiers);
private:
	uint8_t m_sprite_id;

	std::vector<wxString> m_frames;
	std::shared_ptr<GameData> m_gd;

	SpriteEditorFrame* m_sef;
};

wxDECLARE_EVENT(EVT_FRAME_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_FRAME_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_FRAME_DELETE, wxCommandEvent);

#endif // _FRAME_CONTROL_FRAME_H_
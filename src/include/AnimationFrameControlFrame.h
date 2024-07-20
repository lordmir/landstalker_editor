#ifndef _ANIMATION_FRAME_CONTROL_FRAME_H_
#define _ANIMATION_FRAME_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <memory>
#include <string>
#include <ImageList.h>
#include <SelectionControlFrame.h>
#include <GameData.h>

class SpriteEditorFrame;

class AnimationFrameControlFrame : public SelectionControlFrame
{
public:
	AnimationFrameControlFrame(SpriteEditorFrame* parent, ImageList* imglst);
	virtual ~AnimationFrameControlFrame();

	void SetAnimation(uint8_t sprite_id, uint8_t anim_id);
	void ResetAnimation();
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
	virtual std::string MakeLabel(int index) const;
	virtual bool HandleKeyPress(unsigned int key, unsigned int modifiers);
private:
	std::vector<std::string> m_frames;
	std::shared_ptr<GameData> m_gd;

	SpriteEditorFrame* m_sef;
};

wxDECLARE_EVENT(EVT_ANIMATION_FRAME_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_FRAME_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_FRAME_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_FRAME_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_FRAME_MOVE_DOWN, wxCommandEvent);

#endif // _SUBSPRITE_CONTROL_FRAME_H_

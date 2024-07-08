#ifndef _ANIMATION_CONTROL_FRAME_H_
#define _ANIMATION_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <memory>
#include <string>
#include <ImageList.h>
#include <SelectionControlFrame.h>
#include <GameData.h>

class SpriteEditorFrame;

class AnimationControlFrame : public SelectionControlFrame
{
public:
	AnimationControlFrame(SpriteEditorFrame* parent, ImageList* imglst);
	virtual ~AnimationControlFrame();

	void SetAnimations(const std::vector<std::string>& frames);
	void ResetAnimations();
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

wxDECLARE_EVENT(EVT_ANIMATION_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_MOVE_DOWN, wxCommandEvent);

#endif // _ANIMATION_CONTROL_FRAME_H_

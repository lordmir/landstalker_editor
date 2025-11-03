#ifndef _ANIMATION_CONTROL_FRAME_H_
#define _ANIMATION_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>
#include <memory>
#include <string>
#include <landstalker/main/GameData.h>
#include <main/ImageList.h>
#include <misc/SelectionControlFrame.h>

class SpriteEditorFrame;

class AnimationControlFrame : public SelectionControlFrame
{
public:
	AnimationControlFrame(SpriteEditorFrame* parent, ImageList* imglst);
	virtual ~AnimationControlFrame();

	void SetSprite(uint8_t sprite);
	void ResetSprite();
	virtual int GetMaxSelection() const;

	void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
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
	std::vector<wxString> m_anims;
	uint8_t m_sprite_id;
	std::shared_ptr<Landstalker::GameData> m_gd;

	SpriteEditorFrame* m_sef;
};

wxDECLARE_EVENT(EVT_ANIMATION_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_ANIMATION_MOVE_DOWN, wxCommandEvent);

#endif // _ANIMATION_CONTROL_FRAME_H_

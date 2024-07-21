#ifndef _SUBSPRITE_CONTROL_FRAME_H_
#define _SUBSPRITE_CONTROL_FRAME_H_

#include <wx/wx.h>
#include <vector>

#include <landstalker/main/include/GameData.h>
#include <user_interface/main/include/ImageList.h>
#include <user_interface/misc/include/SelectionControlFrame.h>

class SpriteEditorFrame;

class SubspriteControlFrame : public SelectionControlFrame
{
public:
	SubspriteControlFrame(SpriteEditorFrame* parent, ImageList* imglst);
	virtual ~SubspriteControlFrame();

	void SetSubsprites(const std::vector<SpriteFrame::SubSprite>& subsprites);
	void ResetSubsprites();
	virtual int GetMaxSelection() const;
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
	std::vector<SpriteFrame::SubSprite> m_subsprites;

	SpriteEditorFrame* m_sef;
};

wxDECLARE_EVENT(EVT_SUBSPRITE_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_SUBSPRITE_ADD, wxCommandEvent);
wxDECLARE_EVENT(EVT_SUBSPRITE_DELETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_SUBSPRITE_MOVE_UP, wxCommandEvent);
wxDECLARE_EVENT(EVT_SUBSPRITE_MOVE_DOWN, wxCommandEvent);
wxDECLARE_EVENT(EVT_SUBSPRITE_UPDATE, wxCommandEvent);

#endif // _SUBSPRITE_CONTROL_FRAME_H_

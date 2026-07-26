#ifndef _DAMAGE_CONSTANTS_FRAME_H_
#define _DAMAGE_CONSTANTS_FRAME_H_

#include <cstddef>
#include <vector>

#include <wx/aui/aui.h>

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>

class wxScrolledWindow;
class wxSpinCtrlDouble;

// Editor for the sword/armour damage-modifier constants (damage.inc / ChargedSwordBoost +
// ArmourDefence). Each constant is a 16-bit fixed-point multiplier (raw / 256); it is shown and
// edited here as a percentage (raw / 256 * 100) via a spin control that steps one raw unit at a
// time - a step of 1/256, i.e. 100/256 %.
class DamageConstantsFrame : public EditorFrame
{
public:
	DamageConstantsFrame(wxWindow* parent, ImageList* imglst);
	virtual ~DamageConstantsFrame();

	bool Open();
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
	// Flush every field into the game data before a save/build.
	virtual void CommitPendingEdits();

private:
	void BuildUI();
	void LoadValues();
	void CommitSpin(std::size_t index);

	wxAuiManager m_mgr;
	wxScrolledWindow* m_panel = nullptr;
	std::vector<wxSpinCtrlDouble*> m_spins; // aligned to the constant table, one per constant
};

#endif // _DAMAGE_CONSTANTS_FRAME_H_

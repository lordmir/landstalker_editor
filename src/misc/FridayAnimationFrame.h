#ifndef _FRIDAY_ANIMATION_FRAME_H_
#define _FRIDAY_ANIMATION_FRAME_H_

#include <cstdint>
#include <vector>

#include <wx/aui/aui.h>
#include <wx/panel.h>
#include <wx/timer.h>

#include <landstalker/main/GameData.h>
#include <landstalker/main/SpriteData.h>
#include <main/EditorFrame.h>

class wxListBox;
class wxScrolledWindow;
class wxSpinCtrl;
class wxButton;
class wxSlider;
class wxStaticText;

// A small canvas that draws a Friday waypoint path (start point + straight segments through every
// waypoint) auto-fitted to its size, with a dot animating along the path at the per-segment frame
// rate so the motion can be previewed.
class FridayPathPreview : public wxPanel
{
public:
	explicit FridayPathPreview(wxWindow* parent);
	~FridayPathPreview() override;

	void SetPath(const Landstalker::SpriteData::FridayAnimation& anim);
	void Clear();
	// Suspend/restore the timer when the editor is hidden/shown; leaves the user's play state intact.
	void Pause();
	void Resume();
	// User-facing playback controls.
	void SetPlaying(bool playing);
	bool IsPlaying() const { return m_playing; }
	void SetSpeed(double multiplier); // 1.0 == normal (~60 game frames per second)

private:
	void OnPaint(wxPaintEvent& evt);
	void OnTimer(wxTimerEvent& evt);
	void ComputeBounds();
	void UpdateTimerState();
	// The path points in order: the start position followed by each waypoint target.
	std::vector<std::pair<double, double>> PathPoints() const;
	wxPoint ToCanvas(double x, double y, const wxSize& size) const;

	Landstalker::SpriteData::FridayAnimation m_anim;
	wxTimer m_timer;
	double m_frame = 0.0;   // current position along the path, in frames
	int m_total_frames = 0; // sum of every segment's frame count (one full loop)
	double m_min_x = 0.0, m_min_y = 0.0, m_max_x = 0.0, m_max_y = 0.0;
	bool m_has_path = false;
	bool m_playing = true;  // user play/pause state
	bool m_shown = true;    // false while the editor is hidden
	double m_speed = 1.0;   // playback speed multiplier
};

// Editor for the 15 Friday overlay animations (fridayanimNN.bin). Laid out like the input-table
// editor: a left list picks the animation, the centre edits its start position and its waypoints
// as a growing list of rows (X / Y / frames, with an always-present blank row that only commits
// once changed), and a right-hand pane previews the path.
class FridayAnimationFrame : public EditorFrame
{
public:
	FridayAnimationFrame(wxWindow* parent, ImageList* imglst);
	virtual ~FridayAnimationFrame();

	bool Open();
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
	virtual bool Show(bool show = true);
	// Flushes any typed-but-not-blurred field, plus a changed trailing row, before a save/build.
	virtual void CommitPendingEdits();

private:
	using FridayAnimation = Landstalker::SpriteData::FridayAnimation;

	struct WaypointRow
	{
		wxStaticText* number = nullptr;
		wxSpinCtrl* x = nullptr;
		wxSpinCtrl* y = nullptr;
		wxSpinCtrl* frames = nullptr;
		wxButton* del = nullptr;   // null on the trailing blank row
		bool is_blank = false;
	};

	void PopulateAnimationList();
	void SelectAnimationInList(int index);
	void LoadSelected();
	void WriteBack();
	void RebuildRows();
	FridayAnimation::Waypoint ReadRow(const WaypointRow& row) const;
	bool RowIsBlank(const WaypointRow& row) const;
	void OnAnimationSelected();
	void OnStartChanged();
	void OnRowEdited(std::size_t row);
	void OnDeleteRow(std::size_t row);
	void OnPlayPause();
	void OnSpeedChanged();
	void UpdatePlayButton();

	FridayAnimation m_anim;             // working copy of the selected animation
	int m_selected = -1;
	std::vector<WaypointRow> m_rows;    // committed waypoint rows plus the blank
	bool m_populating = false;

	wxAuiManager m_mgr;
	wxListBox* m_list = nullptr;
	wxSpinCtrl* m_start_x = nullptr;
	wxSpinCtrl* m_start_y = nullptr;
	wxScrolledWindow* m_grid_panel = nullptr;
	FridayPathPreview* m_preview = nullptr;
	wxButton* m_play_btn = nullptr;
	wxSlider* m_speed_slider = nullptr;
	wxStaticText* m_speed_label = nullptr;
};

#endif // _FRIDAY_ANIMATION_FRAME_H_

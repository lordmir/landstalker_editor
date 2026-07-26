#include <misc/FridayAnimationFrame.h>

#include <algorithm>
#include <cmath>

#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/listbox.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

namespace
{
	constexpr int TIMER_INTERVAL_MS = 33;
	constexpr double FRAMES_PER_TICK = 2.0; // ~60 game frames per second
	constexpr int MAX_COORD = 0xFFFF;       // X/Y are stored as words
	constexpr int MIN_FRAMES = 1;
	constexpr int MAX_FRAMES = 0x8000;      // frames-1 must stay below the 0x8000 end marker
	// Speed slider maps integer ticks to a playback multiplier of ticks/100 (0.1x .. 4.0x).
	constexpr int SPEED_MIN = 10;
	constexpr int SPEED_MAX = 400;
	constexpr int SPEED_DEFAULT = 100;
}

// ---------------------------------------------------------------------------------------------
// FridayPathPreview
// ---------------------------------------------------------------------------------------------

FridayPathPreview::FridayPathPreview(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(200, 200)),
	  m_timer(this)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize(wxSize(180, 180));
	Bind(wxEVT_PAINT, &FridayPathPreview::OnPaint, this);
	Bind(wxEVT_TIMER, &FridayPathPreview::OnTimer, this);
}

FridayPathPreview::~FridayPathPreview()
{
	m_timer.Stop();
}

std::vector<std::pair<double, double>> FridayPathPreview::PathPoints() const
{
	std::vector<std::pair<double, double>> pts;
	pts.reserve(m_anim.waypoints.size() + 1);
	pts.emplace_back(m_anim.start_x, m_anim.start_y);
	for (const auto& wp : m_anim.waypoints)
	{
		pts.emplace_back(wp.x, wp.y);
	}
	return pts;
}

void FridayPathPreview::ComputeBounds()
{
	const auto pts = PathPoints();
	m_min_x = m_max_x = pts.front().first;
	m_min_y = m_max_y = pts.front().second;
	for (const auto& p : pts)
	{
		m_min_x = std::min(m_min_x, p.first);
		m_max_x = std::max(m_max_x, p.first);
		m_min_y = std::min(m_min_y, p.second);
		m_max_y = std::max(m_max_y, p.second);
	}
}

void FridayPathPreview::SetPath(const Landstalker::SpriteData::FridayAnimation& anim)
{
	m_anim = anim;
	m_has_path = true;
	m_total_frames = 0;
	for (const auto& wp : m_anim.waypoints)
	{
		m_total_frames += std::max<int>(0, wp.frames);
	}
	ComputeBounds();
	if (m_total_frames > 0)
	{
		m_frame = std::fmod(m_frame, static_cast<double>(m_total_frames));
	}
	else
	{
		m_frame = 0.0;
	}
	UpdateTimerState();
	Refresh();
}

void FridayPathPreview::Clear()
{
	m_has_path = false;
	m_timer.Stop();
	m_frame = 0.0;
	Refresh();
}

void FridayPathPreview::Pause()
{
	m_shown = false;
	UpdateTimerState();
}

void FridayPathPreview::Resume()
{
	m_shown = true;
	UpdateTimerState();
}

void FridayPathPreview::SetPlaying(bool playing)
{
	m_playing = playing;
	UpdateTimerState();
}

void FridayPathPreview::SetSpeed(double multiplier)
{
	m_speed = std::max(0.0, multiplier);
}

void FridayPathPreview::UpdateTimerState()
{
	// Run the timer only when the editor is visible, the user hasn't paused, and there is motion.
	const bool should_run = m_shown && m_playing && m_has_path && m_total_frames > 0;
	if (should_run && !m_timer.IsRunning())
	{
		m_timer.Start(TIMER_INTERVAL_MS);
	}
	else if (!should_run && m_timer.IsRunning())
	{
		m_timer.Stop();
	}
}

void FridayPathPreview::OnTimer(wxTimerEvent&)
{
	if (m_total_frames <= 0)
	{
		return;
	}
	m_frame += FRAMES_PER_TICK * m_speed;
	if (m_frame >= m_total_frames)
	{
		m_frame = std::fmod(m_frame, static_cast<double>(m_total_frames));
	}
	Refresh();
}

wxPoint FridayPathPreview::ToCanvas(double x, double y, const wxSize& size) const
{
	const double margin = 14.0;
	double bw = m_max_x - m_min_x;
	double bh = m_max_y - m_min_y;
	if (bw <= 0.0) bw = 1.0;
	if (bh <= 0.0) bh = 1.0;
	const double avail_w = std::max(1.0, size.GetWidth() - 2.0 * margin);
	const double avail_h = std::max(1.0, size.GetHeight() - 2.0 * margin);
	const double scale = std::min(avail_w / bw, avail_h / bh);
	// Centre the fitted path; VDP Y grows downward, matching canvas Y, so no flip is needed.
	const double ox = margin + (avail_w - bw * scale) / 2.0;
	const double oy = margin + (avail_h - bh * scale) / 2.0;
	return wxPoint(static_cast<int>(std::lround(ox + (x - m_min_x) * scale)),
		static_cast<int>(std::lround(oy + (y - m_min_y) * scale)));
}

void FridayPathPreview::OnPaint(wxPaintEvent&)
{
	wxAutoBufferedPaintDC dc(this);
	const wxSize size = GetClientSize();
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
	dc.Clear();
	if (!m_has_path)
	{
		return;
	}

	const wxColour fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	const auto pts = PathPoints();
	std::vector<wxPoint> canvas;
	canvas.reserve(pts.size());
	for (const auto& p : pts)
	{
		canvas.push_back(ToCanvas(p.first, p.second, size));
	}

	// Path segments.
	dc.SetPen(wxPen(fg, 1));
	for (std::size_t i = 1; i < canvas.size(); ++i)
	{
		dc.DrawLine(canvas[i - 1], canvas[i]);
	}
	// Waypoint markers (skip index 0, the start).
	dc.SetBrush(wxBrush(fg));
	for (std::size_t i = 1; i < canvas.size(); ++i)
	{
		dc.DrawCircle(canvas[i], 2);
	}
	// Start marker: a hollow green ring.
	dc.SetPen(wxPen(wxColour(0, 160, 0), 2));
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawCircle(canvas.front(), 4);

	// Animated dot.
	if (m_total_frames > 0 && canvas.size() >= 2)
	{
		double f = m_frame;
		std::size_t seg = 0;
		for (; seg < m_anim.waypoints.size(); ++seg)
		{
			const double d = std::max<int>(1, m_anim.waypoints[seg].frames);
			if (f < d || seg + 1 == m_anim.waypoints.size())
			{
				const double t = std::min(1.0, f / d);
				const wxPoint& a = canvas[seg];
				const wxPoint& b = canvas[seg + 1];
				const wxPoint dot(static_cast<int>(std::lround(a.x + (b.x - a.x) * t)),
					static_cast<int>(std::lround(a.y + (b.y - a.y) * t)));
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.SetBrush(wxBrush(wxColour(220, 40, 40)));
				dc.DrawCircle(dot, 4);
				break;
			}
			f -= d;
		}
	}
}

// ---------------------------------------------------------------------------------------------
// FridayAnimationFrame
// ---------------------------------------------------------------------------------------------

FridayAnimationFrame::FridayAnimationFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	// Left pane: the (fixed) list of animations.
	wxPanel* left = new wxPanel(this, wxID_ANY);
	wxBoxSizer* lv = new wxBoxSizer(wxVERTICAL);
	m_list = new wxListBox(left, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	lv->Add(m_list, 1, wxEXPAND | wxALL, 3);
	left->SetSizer(lv);
	m_list->Bind(wxEVT_LISTBOX, [this](wxCommandEvent&) { OnAnimationSelected(); });

	// Centre pane: start position over the scrolling waypoint grid.
	wxPanel* center = new wxPanel(this, wxID_ANY);
	wxBoxSizer* cv = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* startrow = new wxBoxSizer(wxHORIZONTAL);
	startrow->Add(new wxStaticText(center, wxID_ANY, "Start X:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
	m_start_x = new wxSpinCtrl(center, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1),
		wxSP_ARROW_KEYS, 0, MAX_COORD, 0);
	startrow->Add(m_start_x, 0, wxALL, 4);
	startrow->Add(new wxStaticText(center, wxID_ANY, "Start Y:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
	m_start_y = new wxSpinCtrl(center, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1),
		wxSP_ARROW_KEYS, 0, MAX_COORD, 0);
	startrow->Add(m_start_y, 0, wxALL, 4);
	cv->Add(startrow, 0, wxEXPAND);

	m_grid_panel = new wxScrolledWindow(center, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxVSCROLL | wxHSCROLL | wxTAB_TRAVERSAL);
	m_grid_panel->SetScrollRate(16, 16);
	cv->Add(m_grid_panel, 1, wxEXPAND);
	center->SetSizer(cv);

	for (auto* sp : { m_start_x, m_start_y })
	{
		sp->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { OnStartChanged(); });
		sp->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { OnStartChanged(); });
	}

	// Right pane: the preview canvas over a playback control bar.
	wxPanel* previewPane = new wxPanel(this, wxID_ANY);
	wxBoxSizer* pv = new wxBoxSizer(wxVERTICAL);
	m_preview = new FridayPathPreview(previewPane);
	pv->Add(m_preview, 1, wxEXPAND | wxALL, 2);

	wxBoxSizer* ctrl = new wxBoxSizer(wxHORIZONTAL);
	m_play_btn = new wxButton(previewPane, wxID_ANY, "Pause", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	ctrl->Add(m_play_btn, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
	ctrl->Add(new wxStaticText(previewPane, wxID_ANY, "Speed:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
	m_speed_slider = new wxSlider(previewPane, wxID_ANY, SPEED_DEFAULT, SPEED_MIN, SPEED_MAX);
	ctrl->Add(m_speed_slider, 1, wxALIGN_CENTER_VERTICAL | wxALL, 3);
	m_speed_label = new wxStaticText(previewPane, wxID_ANY, "1.0x", wxDefaultPosition, wxSize(40, -1));
	ctrl->Add(m_speed_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	pv->Add(ctrl, 0, wxEXPAND);
	previewPane->SetSizer(pv);

	m_play_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnPlayPause(); });
	m_speed_slider->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) { OnSpeedChanged(); });
	OnSpeedChanged();

	m_mgr.AddPane(left, wxAuiPaneInfo().Left().Caption("Animations").MinSize(wxSize(150, -1))
		.BestSize(wxSize(180, -1)).CloseButton(false).Floatable(false).Resizable());
	m_mgr.AddPane(previewPane, wxAuiPaneInfo().Right().Caption("Path Preview").MinSize(wxSize(220, -1))
		.BestSize(wxSize(260, -1)).CloseButton(false).Floatable(false).Resizable());
	m_mgr.AddPane(center, wxAuiPaneInfo().CenterPane());
	m_mgr.Update();
}

FridayAnimationFrame::~FridayAnimationFrame()
{
	m_mgr.UnInit();
}

bool FridayAnimationFrame::Open()
{
	if (!m_gd)
	{
		return false;
	}
	PopulateAnimationList();
	const std::size_t count = m_gd->GetSpriteData()->GetFridayAnimationCount();
	if (m_selected < 0 || m_selected >= static_cast<int>(count))
	{
		m_selected = (count == 0) ? -1 : 0;
	}
	SelectAnimationInList(m_selected);
	LoadSelected();
	return true;
}

void FridayAnimationFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_selected = -1;
	Open();
}

void FridayAnimationFrame::ClearGameData()
{
	m_gd.reset();
	m_anim = FridayAnimation();
	m_selected = -1;
	m_rows.clear();
	if (m_list) m_list->Clear();
	if (m_grid_panel)
	{
		m_grid_panel->DestroyChildren();
		m_grid_panel->SetSizer(nullptr);
	}
	if (m_preview) m_preview->Clear();
}

bool FridayAnimationFrame::Show(bool show)
{
	if (m_preview)
	{
		if (show) m_preview->Resume();
		else m_preview->Pause();
	}
	return EditorFrame::Show(show);
}

void FridayAnimationFrame::CommitPendingEdits()
{
	if (!m_gd || m_selected < 0)
	{
		return;
	}
	m_anim.start_x = static_cast<uint16_t>(m_start_x->GetValue());
	m_anim.start_y = static_cast<uint16_t>(m_start_y->GetValue());
	for (std::size_t r = 0; r < m_anim.waypoints.size() && r < m_rows.size(); ++r)
	{
		if (!m_rows[r].is_blank)
		{
			m_anim.waypoints[r] = ReadRow(m_rows[r]);
		}
	}
	if (!m_rows.empty() && m_rows.back().is_blank && !RowIsBlank(m_rows.back()))
	{
		m_anim.waypoints.push_back(ReadRow(m_rows.back()));
	}
	WriteBack();
}

void FridayAnimationFrame::PopulateAnimationList()
{
	if (!m_list || !m_gd)
	{
		return;
	}
	m_populating = true;
	m_list->Freeze();
	m_list->Clear();
	const std::size_t count = m_gd->GetSpriteData()->GetFridayAnimationCount();
	for (std::size_t i = 0; i < count; ++i)
	{
		m_list->Append(wxString::Format("Animation %u", static_cast<unsigned>(i + 1)));
	}
	m_list->Thaw();
	m_populating = false;
}

void FridayAnimationFrame::SelectAnimationInList(int index)
{
	if (!m_list)
	{
		return;
	}
	m_populating = true;
	if (index >= 0 && index < static_cast<int>(m_list->GetCount()))
	{
		m_list->SetSelection(index);
	}
	else
	{
		m_list->SetSelection(wxNOT_FOUND);
	}
	m_populating = false;
}

void FridayAnimationFrame::LoadSelected()
{
	if (m_gd && m_selected >= 0 && m_selected < static_cast<int>(m_gd->GetSpriteData()->GetFridayAnimationCount()))
	{
		m_anim = m_gd->GetSpriteData()->GetFridayAnimationPath(m_selected);
	}
	else
	{
		m_anim = FridayAnimation();
	}
	m_start_x->SetValue(m_anim.start_x);
	m_start_y->SetValue(m_anim.start_y);
	m_start_x->Enable(m_selected >= 0);
	m_start_y->Enable(m_selected >= 0);
	RebuildRows();
	if (m_selected >= 0) m_preview->SetPath(m_anim);
	else m_preview->Clear();
}

void FridayAnimationFrame::WriteBack()
{
	if (!m_gd || m_selected < 0)
	{
		return;
	}
	m_gd->GetSpriteData()->SetFridayAnimationPath(m_selected, m_anim);
	m_preview->SetPath(m_anim);
}

void FridayAnimationFrame::RebuildRows()
{
	if (!m_grid_panel)
	{
		return;
	}
	m_grid_panel->Freeze();
	m_rows.clear();
	m_grid_panel->DestroyChildren();

	if (m_selected < 0)
	{
		m_grid_panel->SetSizer(nullptr);
		m_grid_panel->Layout();
		m_grid_panel->Thaw();
		return;
	}

	auto* top = new wxBoxSizer(wxVERTICAL);
	auto* grid = new wxFlexGridSizer(5, wxSize(6, 4));
	grid->Add(new wxStaticText(m_grid_panel, wxID_ANY, "#"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_grid_panel, wxID_ANY, "X"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_grid_panel, wxID_ANY, "Y"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_grid_panel, wxID_ANY, "Frames"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_grid_panel, wxID_ANY, wxEmptyString), 0);

	const std::size_t total = m_anim.waypoints.size() + 1; // waypoints plus the trailing blank
	for (std::size_t r = 0; r < total; ++r)
	{
		const bool blank = (r == m_anim.waypoints.size());
		const FridayAnimation::Waypoint wp = blank ? FridayAnimation::Waypoint{} : m_anim.waypoints[r];

		WaypointRow lr;
		lr.is_blank = blank;

		lr.number = new wxStaticText(m_grid_panel, wxID_ANY,
			blank ? wxString("*") : wxString::Format("%u", static_cast<unsigned>(r)));
		grid->Add(lr.number, 0, wxALIGN_CENTER);

		lr.x = new wxSpinCtrl(m_grid_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1),
			wxSP_ARROW_KEYS, 0, MAX_COORD, wp.x);
		grid->Add(lr.x, 0, wxALIGN_CENTER);
		lr.y = new wxSpinCtrl(m_grid_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1),
			wxSP_ARROW_KEYS, 0, MAX_COORD, wp.y);
		grid->Add(lr.y, 0, wxALIGN_CENTER);
		lr.frames = new wxSpinCtrl(m_grid_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1),
			wxSP_ARROW_KEYS, MIN_FRAMES, MAX_FRAMES, std::max<int>(MIN_FRAMES, wp.frames));
		grid->Add(lr.frames, 0, wxALIGN_CENTER);

		if (blank)
		{
			// Commit only when a field settles (spin buttons / focus loss), never per keystroke -
			// a rebuild would otherwise destroy the control being edited.
			for (auto* sp : { lr.x, lr.y, lr.frames })
			{
				sp->Bind(wxEVT_SPINCTRL, [this, r](wxSpinEvent&) { OnRowEdited(r); });
				sp->Bind(wxEVT_KILL_FOCUS, [this, r](wxFocusEvent& e) { e.Skip(); OnRowEdited(r); });
			}
			grid->Add(new wxStaticText(m_grid_panel, wxID_ANY, wxEmptyString), 0);
		}
		else
		{
			for (auto* sp : { lr.x, lr.y, lr.frames })
			{
				sp->Bind(wxEVT_TEXT, [this, r](wxCommandEvent&) { OnRowEdited(r); });
				sp->Bind(wxEVT_SPINCTRL, [this, r](wxSpinEvent&) { OnRowEdited(r); });
			}
			lr.del = new wxButton(m_grid_panel, wxID_ANY, "Delete", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
			lr.del->Bind(wxEVT_BUTTON, [this, r](wxCommandEvent&) { OnDeleteRow(r); });
			grid->Add(lr.del, 0, wxALIGN_CENTER);
		}

		m_rows.push_back(lr);
	}

	top->Add(grid, 0, wxALL, 8);
	m_grid_panel->SetSizer(top);
	m_grid_panel->FitInside();
	m_grid_panel->Layout();
	m_grid_panel->Thaw();
}

FridayAnimationFrame::FridayAnimation::Waypoint FridayAnimationFrame::ReadRow(const WaypointRow& row) const
{
	FridayAnimation::Waypoint wp;
	wp.x = row.x ? static_cast<uint16_t>(row.x->GetValue()) : 0;
	wp.y = row.y ? static_cast<uint16_t>(row.y->GetValue()) : 0;
	wp.frames = row.frames ? static_cast<uint16_t>(row.frames->GetValue()) : MIN_FRAMES;
	return wp;
}

bool FridayAnimationFrame::RowIsBlank(const WaypointRow& row) const
{
	const FridayAnimation::Waypoint wp = ReadRow(row);
	return wp.x == 0 && wp.y == 0 && wp.frames == MIN_FRAMES;
}

void FridayAnimationFrame::OnAnimationSelected()
{
	if (m_populating)
	{
		return;
	}
	const int row = m_list->GetSelection();
	m_selected = (row == wxNOT_FOUND) ? -1 : row;
	LoadSelected();
}

void FridayAnimationFrame::OnStartChanged()
{
	if (!m_gd || m_selected < 0)
	{
		return;
	}
	m_anim.start_x = static_cast<uint16_t>(m_start_x->GetValue());
	m_anim.start_y = static_cast<uint16_t>(m_start_y->GetValue());
	WriteBack();
}

void FridayAnimationFrame::OnRowEdited(std::size_t row)
{
	if (!m_gd || m_selected < 0 || row >= m_rows.size())
	{
		return;
	}
	WaypointRow& lr = m_rows[row];
	if (lr.is_blank)
	{
		if (RowIsBlank(lr))
		{
			return; // untouched blank - never committed
		}
		m_anim.waypoints.push_back(ReadRow(lr));
		WriteBack();
		CallAfter([this] { RebuildRows(); });
		return;
	}
	if (row < m_anim.waypoints.size())
	{
		m_anim.waypoints[row] = ReadRow(lr);
		WriteBack();
	}
}

void FridayAnimationFrame::OnDeleteRow(std::size_t row)
{
	if (!m_gd || m_selected < 0 || row >= m_anim.waypoints.size())
	{
		return;
	}
	m_anim.waypoints.erase(m_anim.waypoints.begin() + row);
	WriteBack();
	CallAfter([this] { RebuildRows(); });
}

void FridayAnimationFrame::OnPlayPause()
{
	if (!m_preview)
	{
		return;
	}
	m_preview->SetPlaying(!m_preview->IsPlaying());
	UpdatePlayButton();
}

void FridayAnimationFrame::OnSpeedChanged()
{
	if (!m_speed_slider || !m_preview)
	{
		return;
	}
	const double mult = m_speed_slider->GetValue() / 100.0;
	m_preview->SetSpeed(mult);
	if (m_speed_label)
	{
		m_speed_label->SetLabel(wxString::Format("%.1fx", mult));
	}
}

void FridayAnimationFrame::UpdatePlayButton()
{
	if (m_play_btn && m_preview)
	{
		m_play_btn->SetLabel(m_preview->IsPlaying() ? "Pause" : "Play");
	}
}

#include <misc/InputTableFrame.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

namespace
{
	// The direction is the low nibble of the controller bitfield (controls.inc: UP=$01 DOWN=$02
	// LEFT=$04 RIGHT=$08). Only "none" or a diagonal is a valid scripted direction, so the picker
	// offers exactly those, in compass terms.
	const char* const DIRECTION_LABELS[5] = { "None", "NW", "NE", "SE", "SW" };
	constexpr uint8_t DIRECTION_BITS[5] = { 0x00, 0x05, 0x09, 0x0A, 0x06 };
	constexpr uint8_t DIRECTION_MASK = 0x0F;

	// A/B/C occupy bits 4-6 (controls.inc: B=$10 C=$20 A=$40); bit 7 (Start) is the sequence-end
	// marker and never a button here.
	const char* const BUTTON_LABELS[3] = { "A", "B", "C" };
	constexpr uint8_t BUTTON_BITS[3] = { 0x40, 0x10, 0x20 };

	constexpr uint8_t SEQUENCE_END = 0x80;
	constexpr uint8_t HOLD_DURATION = 0xFE;
	constexpr int MAX_DURATION = 0xFD;
	// A duration byte of 0x80 is indistinguishable from a sequence-end marker to the engine's
	// sequence seek, so it is not a permitted value.
	constexpr int RESERVED_DURATION = 0x80;

	int DirectionToSelection(uint8_t buttons)
	{
		const uint8_t dir = buttons & DIRECTION_MASK;
		for (int i = 0; i < 5; ++i)
		{
			if (DIRECTION_BITS[i] == dir)
			{
				return i;
			}
		}
		return 0; // any non-diagonal falls back to "None" in the picker
	}
}

InputTableFrame::InputTableFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	// Left pane: the sequence list over a grid of management buttons (as in the entity editor).
	wxPanel* left = new wxPanel(this, wxID_ANY);
	wxBoxSizer* lv = new wxBoxSizer(wxVERTICAL);
	m_seq_list = new wxListBox(left, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	lv->Add(m_seq_list, 1, wxEXPAND | wxALL, 3);

	wxGridSizer* buttons = new wxGridSizer(0, 2, 3, 3);
	m_add = new wxButton(left, wxID_ANY, "Add");
	m_remove = new wxButton(left, wxID_ANY, "Remove");
	m_move_up = new wxButton(left, wxID_ANY, "Move Up");
	m_move_down = new wxButton(left, wxID_ANY, "Move Down");
	for (auto* b : { m_add, m_remove, m_move_up, m_move_down })
	{
		buttons->Add(b, 0, wxEXPAND);
	}
	lv->Add(buttons, 0, wxEXPAND | wxALL, 3);
	left->SetSizer(lv);

	m_seq_list->Bind(wxEVT_LISTBOX, [this](wxCommandEvent&) { OnSequenceSelected(); });
	m_add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnAddSequence(); });
	m_remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRemoveSequence(); });
	m_move_up->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnMoveSequence(-1); });
	m_move_down->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnMoveSequence(1); });

	m_lines = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxVSCROLL | wxHSCROLL | wxTAB_TRAVERSAL);
	m_lines->SetScrollRate(8, 16);

	m_mgr.AddPane(left, wxAuiPaneInfo().Left().Caption("Sequences").MinSize(wxSize(180, -1))
		.BestSize(wxSize(220, -1)).CloseButton(false).Floatable(false).Resizable());
	m_mgr.AddPane(m_lines, wxAuiPaneInfo().CenterPane());
	m_mgr.Update();
	RefreshButtons();
}

InputTableFrame::~InputTableFrame()
{
	m_mgr.UnInit();
}

bool InputTableFrame::Open()
{
	if (!m_gd)
	{
		return false;
	}
	LoadFromData();
	PopulateSequenceList();
	if (m_selected < 0 || m_selected >= static_cast<int>(m_sequences.size()))
	{
		m_selected = m_sequences.empty() ? -1 : 0;
	}
	SelectSequenceInList(m_selected);
	RebuildLines();
	RefreshButtons();
	return true;
}

void InputTableFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_selected = -1;
	Open();
}

void InputTableFrame::ClearGameData()
{
	m_gd.reset();
	m_sequences.clear();
	m_rows.clear();
	m_selected = -1;
	if (m_seq_list)
	{
		m_seq_list->Clear();
	}
	if (m_lines)
	{
		m_lines->DestroyChildren();
		m_lines->SetSizer(nullptr);
	}
	RefreshButtons();
}

void InputTableFrame::CommitPendingEdits()
{
	if (!m_gd || m_selected < 0 || m_selected >= static_cast<int>(m_sequences.size()))
	{
		return;
	}
	// Pull any typed-but-not-blurred durations back out of the row controls, and fold in a
	// trailing blank line that was changed but not yet committed.
	auto& seq = m_sequences[m_selected];
	for (std::size_t r = 0; r < seq.size() && r < m_rows.size(); ++r)
	{
		if (!m_rows[r].is_blank)
		{
			seq[r] = ReadRow(m_rows[r]);
		}
	}
	if (!m_rows.empty() && m_rows.back().is_blank && !RowIsBlank(m_rows.back()))
	{
		seq.push_back(ReadRow(m_rows.back()));
	}
	CommitToData();
}

void InputTableFrame::LoadFromData()
{
	m_sequences.clear();
	if (!m_gd)
	{
		return;
	}
	const auto& bytes = m_gd->GetSpriteData()->GetInputPlayback();
	Sequence current;
	std::size_t i = 0;
	while (i < bytes.size())
	{
		if (bytes[i] == SEQUENCE_END)
		{
			m_sequences.push_back(current);
			current.clear();
			++i;
			continue;
		}
		Line line;
		line.buttons = bytes[i] & 0x7F;
		const uint8_t dur = (i + 1 < bytes.size()) ? bytes[i + 1] : 0;
		if (dur == HOLD_DURATION)
		{
			line.hold = true;
			line.duration = 0;
		}
		else
		{
			line.duration = dur;
		}
		current.push_back(line);
		i += 2;
	}
	// A well-formed table ends on a 0x80, so anything left here is a stray unterminated tail.
	if (!current.empty())
	{
		m_sequences.push_back(current);
	}
}

void InputTableFrame::CommitToData()
{
	if (!m_gd)
	{
		return;
	}
	std::vector<uint8_t> bytes;
	for (const auto& seq : m_sequences)
	{
		for (const auto& line : seq)
		{
			bytes.push_back(line.buttons);
			bytes.push_back(line.hold ? HOLD_DURATION : line.duration);
		}
		bytes.push_back(SEQUENCE_END);
	}
	m_gd->GetSpriteData()->SetInputPlayback(bytes);
}

void InputTableFrame::PopulateSequenceList()
{
	if (!m_seq_list)
	{
		return;
	}
	m_populating = true;
	m_seq_list->Freeze();
	m_seq_list->Clear();
	for (std::size_t i = 0; i < m_sequences.size(); ++i)
	{
		m_seq_list->Append(wxString::Format("Sequence %u", static_cast<unsigned>(i)));
	}
	m_seq_list->Thaw();
	m_populating = false;
}

void InputTableFrame::SelectSequenceInList(int index)
{
	if (!m_seq_list)
	{
		return;
	}
	m_populating = true;
	if (index >= 0 && index < static_cast<int>(m_seq_list->GetCount()))
	{
		m_seq_list->SetSelection(index);
		m_seq_list->EnsureVisible(index);
	}
	else
	{
		m_seq_list->SetSelection(wxNOT_FOUND);
	}
	m_populating = false;
}

void InputTableFrame::RefreshButtons()
{
	const bool has_data = m_gd != nullptr;
	const bool has_sel = has_data && m_selected >= 0 && m_selected < static_cast<int>(m_sequences.size());
	if (m_add) m_add->Enable(has_data);
	if (m_remove) m_remove->Enable(has_sel);
	if (m_move_up) m_move_up->Enable(has_sel && m_selected > 0);
	if (m_move_down) m_move_down->Enable(has_sel && m_selected < static_cast<int>(m_sequences.size()) - 1);
}

void InputTableFrame::OnSequenceSelected()
{
	if (m_populating)
	{
		return;
	}
	const int row = m_seq_list->GetSelection();
	m_selected = (row == wxNOT_FOUND) ? -1 : row;
	RebuildLines();
	RefreshButtons();
}

void InputTableFrame::OnAddSequence()
{
	if (!m_gd)
	{
		return;
	}
	m_sequences.emplace_back();
	CommitToData();
	m_selected = static_cast<int>(m_sequences.size()) - 1;
	PopulateSequenceList();
	SelectSequenceInList(m_selected);
	RebuildLines();
	RefreshButtons();
}

void InputTableFrame::OnRemoveSequence()
{
	if (!m_gd || m_selected < 0 || m_selected >= static_cast<int>(m_sequences.size()))
	{
		return;
	}
	m_sequences.erase(m_sequences.begin() + m_selected);
	CommitToData();
	if (m_selected >= static_cast<int>(m_sequences.size()))
	{
		m_selected = static_cast<int>(m_sequences.size()) - 1;
	}
	PopulateSequenceList();
	SelectSequenceInList(m_selected);
	RebuildLines();
	RefreshButtons();
}

void InputTableFrame::OnMoveSequence(int delta)
{
	if (!m_gd || m_selected < 0)
	{
		return;
	}
	const int target = m_selected + delta;
	if (target < 0 || target >= static_cast<int>(m_sequences.size()))
	{
		return;
	}
	std::swap(m_sequences[m_selected], m_sequences[target]);
	m_selected = target;
	CommitToData();
	PopulateSequenceList();
	SelectSequenceInList(m_selected);
	RebuildLines();
	RefreshButtons();
}

void InputTableFrame::RebuildLines()
{
	if (!m_lines)
	{
		return;
	}
	m_lines->Freeze();
	m_rows.clear();
	m_lines->DestroyChildren();

	if (m_selected < 0 || m_selected >= static_cast<int>(m_sequences.size()))
	{
		m_lines->SetSizer(nullptr);
		m_lines->Layout();
		m_lines->Thaw();
		return;
	}

	auto* top = new wxBoxSizer(wxVERTICAL);
	auto* grid = new wxFlexGridSizer(8, wxSize(6, 4));

	// Header row: #, Direction, A, B, C, Duration, Hold, (delete).
	grid->Add(new wxStaticText(m_lines, wxID_ANY, "#"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_lines, wxID_ANY, "Direction"), 0, wxALIGN_CENTER);
	for (int k = 0; k < 3; ++k)
	{
		grid->Add(new wxStaticText(m_lines, wxID_ANY, BUTTON_LABELS[k]), 0, wxALIGN_CENTER);
	}
	grid->Add(new wxStaticText(m_lines, wxID_ANY, "Duration"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_lines, wxID_ANY, "Hold"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_lines, wxID_ANY, wxEmptyString), 0);

	wxArrayString dir_choices;
	for (const char* label : DIRECTION_LABELS)
	{
		dir_choices.Add(label);
	}

	const Sequence& seq = m_sequences[m_selected];
	const std::size_t total = seq.size() + 1; // committed lines plus the trailing blank
	for (std::size_t r = 0; r < total; ++r)
	{
		const bool blank = (r == seq.size());
		const Line line = blank ? Line{} : seq[r];

		LineRow lr;
		lr.is_blank = blank;
		lr.prev_duration = line.duration;

		// An unmodified blank line is marked "*"; committed lines show their index.
		lr.number = new wxStaticText(m_lines, wxID_ANY,
			blank ? wxString("*") : wxString::Format("%u", static_cast<unsigned>(r)));
		grid->Add(lr.number, 0, wxALIGN_CENTER);

		auto* dir = new wxChoice(m_lines, wxID_ANY, wxDefaultPosition, wxDefaultSize, dir_choices);
		dir->SetSelection(DirectionToSelection(line.buttons));
		dir->Bind(wxEVT_CHOICE, [this, r](wxCommandEvent&) { OnLineEdited(r); });
		lr.direction = dir;
		grid->Add(dir, 0, wxALIGN_CENTER);

		for (int k = 0; k < 3; ++k)
		{
			auto* cb = new wxCheckBox(m_lines, wxID_ANY, wxEmptyString);
			cb->SetValue((line.buttons & BUTTON_BITS[k]) != 0);
			cb->Bind(wxEVT_CHECKBOX, [this, r](wxCommandEvent&) { OnLineEdited(r); });
			lr.buttons[k] = cb;
			grid->Add(cb, 0, wxALIGN_CENTER);
		}

		auto* sp = new wxSpinCtrl(m_lines, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(70, -1),
			wxSP_ARROW_KEYS, 0, MAX_DURATION, line.duration);
		sp->Enable(!line.hold);
		lr.duration = sp;
		grid->Add(sp, 0, wxALIGN_CENTER);

		auto* hb = new wxCheckBox(m_lines, wxID_ANY, wxEmptyString);
		hb->SetValue(line.hold);
		hb->Bind(wxEVT_CHECKBOX, [this, r](wxCommandEvent&) { OnLineEdited(r); });
		lr.hold = hb;
		grid->Add(hb, 0, wxALIGN_CENTER);

		if (blank)
		{
			// A blank line commits on a discrete change (a checkbox) or once the duration field
			// settles (spin buttons / focus loss) - never per keystroke, which would rebuild the
			// grid and destroy the control mid-edit.
			sp->Bind(wxEVT_SPINCTRL, [this, r](wxSpinEvent&) { OnLineEdited(r); });
			sp->Bind(wxEVT_KILL_FOCUS, [this, r](wxFocusEvent& e) { e.Skip(); OnLineEdited(r); });
			grid->Add(new wxStaticText(m_lines, wxID_ANY, wxEmptyString), 0);
		}
		else
		{
			// Committed rows never rebuild on edit, so it is safe to track every keystroke.
			sp->Bind(wxEVT_TEXT, [this, r](wxCommandEvent&) { OnLineEdited(r); });
			sp->Bind(wxEVT_SPINCTRL, [this, r](wxSpinEvent&) { OnLineEdited(r); });
			auto* db = new wxButton(m_lines, wxID_ANY, "Delete", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
			db->Bind(wxEVT_BUTTON, [this, r](wxCommandEvent&) { OnDeleteLine(r); });
			lr.del = db;
			grid->Add(db, 0, wxALIGN_CENTER);
		}

		m_rows.push_back(lr);
	}

	top->Add(grid, 0, wxALL, 8);
	m_lines->SetSizer(top);
	m_lines->FitInside();
	m_lines->Layout();
	m_lines->Thaw();
}

InputTableFrame::Line InputTableFrame::ReadRow(const LineRow& row) const
{
	Line line;
	if (row.direction)
	{
		const int sel = row.direction->GetSelection();
		line.buttons |= DIRECTION_BITS[(sel >= 0 && sel < 5) ? sel : 0];
	}
	for (int k = 0; k < 3; ++k)
	{
		if (row.buttons[k] && row.buttons[k]->GetValue())
		{
			line.buttons |= BUTTON_BITS[k];
		}
	}
	line.hold = row.hold && row.hold->GetValue();
	line.duration = row.duration ? static_cast<uint8_t>(row.duration->GetValue()) : 0;
	return line;
}

bool InputTableFrame::RowIsBlank(const LineRow& row) const
{
	const Line line = ReadRow(row);
	return line.buttons == 0 && !line.hold && line.duration == 0;
}

void InputTableFrame::OnLineEdited(std::size_t row)
{
	if (!m_gd || m_selected < 0 || row >= m_rows.size())
	{
		return;
	}
	LineRow& lr = m_rows[row];
	// A held line ignores its duration, so grey the spin out to match.
	if (lr.duration && lr.hold)
	{
		lr.duration->Enable(!lr.hold->GetValue());
	}
	// 0x80 is reserved (it reads as a sequence-end marker), so step over it in whichever
	// direction the value was moving.
	if (lr.duration)
	{
		int value = lr.duration->GetValue();
		if (value == RESERVED_DURATION)
		{
			value = (value >= lr.prev_duration) ? RESERVED_DURATION + 1 : RESERVED_DURATION - 1;
			lr.duration->SetValue(value);
		}
		lr.prev_duration = value;
	}

	if (lr.is_blank)
	{
		if (RowIsBlank(lr))
		{
			return; // untouched blank - never committed
		}
		m_sequences[m_selected].push_back(ReadRow(lr));
		CommitToData();
		CallAfter([this] { RebuildLines(); }); // append a fresh blank below the new line
		return;
	}

	if (row < m_sequences[m_selected].size())
	{
		m_sequences[m_selected][row] = ReadRow(lr);
		CommitToData();
	}
}

void InputTableFrame::OnDeleteLine(std::size_t row)
{
	if (!m_gd || m_selected < 0)
	{
		return;
	}
	auto& seq = m_sequences[m_selected];
	if (row >= seq.size())
	{
		return;
	}
	seq.erase(seq.begin() + row);
	CommitToData();
	CallAfter([this] { RebuildLines(); });
}

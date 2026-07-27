#ifndef _INPUT_TABLE_FRAME_H_
#define _INPUT_TABLE_FRAME_H_

#include <cstdint>
#include <vector>

#include <wx/aui/aui.h>

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>

class wxPanel;
class wxListBox;
class wxButton;
class wxTextCtrl;
class wxScrolledWindow;
class wxCheckBox;
class wxSpinCtrl;

// Editor for the scripted-input playback table (InputPlayback / playbackinputtable.bin). The table
// is a list of input sequences; each sequence is a run of (button-bitfield, duration) pairs ending
// at a 0x80 marker. A duration of 0xFE means "hold" (the input is held until the next sequence).
//
// Layout mirrors the entity editor: a left pane lists the sequences with add / remove / move
// buttons, and the centre pane edits the selected sequence as one row per line - seven button
// checkboxes (U/D/L/R/A/B/C), a duration spin (0..0xFD), a Hold checkbox and a Delete Line button.
// A blank line is always kept at the bottom for appending; it is only committed once edited.
class InputTableFrame : public EditorFrame
{
public:
	InputTableFrame(wxWindow* parent, ImageList* imglst);
	virtual ~InputTableFrame();

	// The display name for input sequence `index`: its C_INPUT_SCRIPT label, or a generic
	// "PlaybackScriptNN" when unnamed. Shared with the cutscene editor's <Playback> hints/picker.
	static wxString DisplayName(int index);

	bool Open();
	// Select and show a given input sequence (e.g. when opened from a <Playback>/PlaybackInput link).
	void GoToSequence(int sequence);
	// Hide/show the left "Sequences" list pane - hidden for the focused single-sequence popup.
	void ShowSequenceListPane(bool show);
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
	// Flushes any duration typed but not yet blurred, plus a changed trailing line, into the
	// data before the project is saved or built.
	virtual void CommitPendingEdits();

private:
	struct Line
	{
		// Controller bitfield: the low nibble is a direction (only "none" or a diagonal is valid;
		// see DIRECTION_BITS) and bits 4-6 are A/B/C (BUTTON_BITS). Bit 7 (Start) never appears
		// here - it is the end-of-sequence marker.
		uint8_t buttons = 0;
		uint8_t duration = 0;  // frame count, 0..0xFD excluding 0x80; ignored while hold is set
		bool hold = false;     // serialises the duration byte as 0xFE
	};
	using Sequence = std::vector<Line>;

	// Per-line widgets, kept parallel to the rendered rows so a control maps back to its line.
	struct LineRow
	{
		wxStaticText* number = nullptr;  // row index, or "*" for the uncommitted blank line
		wxChoice* direction = nullptr;   // None / NW / NE / SE / SW
		wxCheckBox* buttons[3] = {};     // A / B / C
		wxSpinCtrl* duration = nullptr;
		wxCheckBox* hold = nullptr;
		wxButton* del = nullptr;         // null on the trailing blank row
		int prev_duration = 0;           // last accepted duration, so 0x80 can be skipped either way
		bool is_blank = false;
	};

	// Model <-> raw bytes.
	void LoadFromData();
	void CommitToData();

	// Name field.
	void CommitName();       // write the field back to the C_INPUT_SCRIPT label for the selection
	void UpdateNameField();  // show the selected sequence's current name in the field

	// Left sequence list.
	void PopulateSequenceList();
	void SelectSequenceInList(int index);
	void RefreshButtons();
	void OnSequenceSelected();
	void OnAddSequence();
	void OnRemoveSequence();
	void OnMoveSequence(int delta);

	// Centre line grid.
	void RebuildLines();
	Line ReadRow(const LineRow& row) const;
	bool RowIsBlank(const LineRow& row) const;
	void OnLineEdited(std::size_t row);
	void OnDeleteLine(std::size_t row);

	std::vector<Sequence> m_sequences;
	int m_selected = -1;                // selected sequence, or -1
	std::vector<LineRow> m_rows;        // widgets for the selected sequence's lines + blank

	wxAuiManager m_mgr;
	wxPanel* m_left_pane = nullptr;     // the "Sequences" list pane, hidden in the focused popup
	wxListBox* m_seq_list = nullptr;
	wxButton* m_add = nullptr;
	wxButton* m_remove = nullptr;
	wxButton* m_move_up = nullptr;
	wxButton* m_move_down = nullptr;
	wxScrolledWindow* m_lines = nullptr;
	wxTextCtrl* m_name = nullptr;       // editable name for the selected sequence (C_INPUT_SCRIPT)
	bool m_populating = false;          // guards programmatic list changes from re-entry
};

#endif // _INPUT_TABLE_FRAME_H_

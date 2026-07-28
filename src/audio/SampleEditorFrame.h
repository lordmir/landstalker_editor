#ifndef _SAMPLE_EDITOR_FRAME_H_
#define _SAMPLE_EDITOR_FRAME_H_

#include <cstdint>
#include <memory>
#include <vector>

#include <wx/aui/aui.h>
#include <wx/panel.h>
#include <wx/string.h>
#include <wx/timer.h>

#include <audio/AlsaPlayer.h>
#include <landstalker/main/AudioData.h>
#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>

class wxScrolledWindow;
class wxButton;
class wxSpinCtrl;
class wxStaticText;
class wxSound;

// A horizontal bar rendering a mono 8bpp PCM sample buffer as a min/max envelope waveform, one
// pixel column at a time. Empty data just shows a flat centre line.
class WaveformPanel : public wxPanel
{
public:
	explicit WaveformPanel(wxWindow* parent);

	void SetData(const std::vector<uint8_t>& samples);
	void Clear();

private:
	void OnPaint(wxPaintEvent& evt);

	std::vector<uint8_t> m_samples;
};

// Editor for the raw PCM sample banks (pcmbank0.bin / pcmbank1.bin) used by the Cube/Iwadare
// sound driver for digital sample playback, and the sample directory that indexes into them.
// The top half lets each bank's mono 8bpp sample data be previewed as a waveform and
// imported/exported as a raw binary file. The bottom half edits the sample directory as one row
// per entry (rate, bank, length, start offset), with a Delete button per row and an always-
// present blank trailing row for appending - mirroring the input-table editor. The number of
// rows allowed depends on how the data was loaded (AudioData::GetMaxPcmSampleCount()): a ROM
// load's table is a fixed-size region within the driver binary, so growing it would corrupt
// adjacent driver data.
class SampleEditorFrame : public EditorFrame
{
public:
	SampleEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~SampleEditorFrame();

	bool Open();
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
	// Flushes any field typed but not yet blurred, plus a changed trailing row, before a save/build.
	virtual void CommitPendingEdits();

private:
	using PcmSample = Landstalker::AudioData::PcmSample;

	// Per-row widgets, kept parallel to the rendered rows so a control maps back to its entry.
	struct SampleRow
	{
		wxStaticText* number = nullptr;
		wxSpinCtrl* rate = nullptr;
		wxStaticText* rate_hz = nullptr;
		wxSpinCtrl* bank = nullptr;
		wxSpinCtrl* length = nullptr;
		wxSpinCtrl* start_offset = nullptr;
		wxStaticText* duration = nullptr; // derived readout: length / sample-rate, in seconds
		wxButton* play = nullptr;       // null on the trailing blank row
		wxButton* export_wav = nullptr; // null on the trailing blank row
		wxButton* del = nullptr;        // null on the trailing blank row
		bool is_blank = false;
	};

	void BuildUI();
	void LoadValues();
	// One PCM bank's box: waveform + size readout + import/export buttons. bank is 0 or 1.
	wxSizer* BuildBankBox(wxWindow* parent, const wxString& title, int bank);
	void UpdateBankSizeLabel(int bank);
	void OnImport(int bank);
	void OnExport(int bank);

	// Sample table grid.
	void RebuildRows();
	// Refreshes a row's derived readout labels (Hz sample rate and duration) from its current
	// Rate/Length control values. Touches no data, so it's safe to call at any time.
	void UpdateRowReadouts(const SampleRow& row) const;
	PcmSample ReadRow(const SampleRow& row) const;
	bool RowIsBlank(const SampleRow& row) const;
	void OnRowEdited(std::size_t row);
	void OnDeleteRow(std::size_t row);

	// Slices a row's PCM bytes out of its bank and looks up its playback rate; false if the row
	// or its bank/offset/length don't produce a valid slice (used by both playback and export).
	bool ExtractSamplePcm(std::size_t row, std::vector<uint8_t>& pcm, uint32_t& rate_hz) const;
	void OnExportSampleRow(std::size_t row);

	// Playback (one sample at a time; starting another stops it).
	//
	// On Linux, AlsaPlayer talks to ALSA directly - wxSound's only non-null Unix backend talks to
	// the legacy OSS /dev/dsp device, which current distros generally don't provide (they run
	// ALSA/PulseAudio/PipeWire instead), so wxSound silently "succeeds" via its no-op Null backend
	// there and nothing is ever heard. AlsaPlayer also keeps the output device open across plays
	// instead of reopening it each time (a fresh open/close reliably clicks as the DAC settles)
	// and reports real playback completion instead of guessing, so StartPlayback doesn't need the
	// timer-based approximation below for this path.
	//
	// On Windows (and macOS, whose wxSound backend has neither of the Unix problems above),
	// wxSound plays the in-memory WAV directly. It has no completion callback though, so a timer
	// sized to the sample's duration (plus a grace margin for device startup latency) is used to
	// reset the button afterwards - see the .cpp for why that path must NOT also interrupt
	// playback: killing it early is exactly what used to cause an audible pop at the end of every
	// sample (on the old Unix subprocess approach, which AlsaPlayer replaces).
	void OnPlayRow(std::size_t row);
	void OnPlayTimer(wxTimerEvent& evt);
	// Starts playing a sliced sample; returns false (and shows an error) if it couldn't be started
	// at all. `row` is only used to match up AlsaPlayer's completion callback on Linux.
	bool StartPlayback(const std::vector<uint8_t>& pcm, uint32_t rate_hz, std::size_t row);
	// Explicit interruption: about to play something else, or the user hit Stop.
	void StopPlayback();
	// Assumed-natural-completion path: tidies up bookkeeping/UI without sending any stop signal.
	void ResetPlaybackState();

	wxAuiManager m_mgr;
	wxPanel* m_panel = nullptr;
	WaveformPanel* m_waveform[2] = { nullptr, nullptr };
	wxStaticText* m_bank_size[2] = { nullptr, nullptr };
	wxButton* m_import_btn[2] = { nullptr, nullptr };
	wxButton* m_export_btn[2] = { nullptr, nullptr };

	wxScrolledWindow* m_table_panel = nullptr;
	std::vector<PcmSample> m_table;      // working copy of the sample directory
	std::vector<SampleRow> m_rows;       // widgets for the table's entries + blank
	// Guards OnRowEdited against firing while RebuildRows is programmatically constructing/
	// initialising row controls. wx (particularly GTK) can queue a spin control's "changed"
	// notification for delivery on the next event loop iteration rather than firing it inline,
	// so binding a handler right after setting a control's initial value is not enough by itself
	// - without this guard, populating the table on load spuriously "edits" every row once the
	// event loop turns, which round-trips identical values back into AudioData but still flips
	// its dirty flag (the write happens, even though nothing actually changed).
	bool m_populating = false;

	int m_playing_row = -1;
	wxTimer m_play_timer;
#if defined(__linux__)
	AlsaPlayer m_alsa;
#else
	std::unique_ptr<wxSound> m_sound;  // Windows/macOS playback
#endif
};

#endif // _SAMPLE_EDITOR_FRAME_H_

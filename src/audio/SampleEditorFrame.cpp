#include <audio/SampleEditorFrame.h>

#include <fstream>
#include <sstream>

#include <audio/AudioTablesYaml.h>
#include <audio/YamlIo.h>

#include <algorithm>
#include <cmath>
#include <filesystem>

#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/scrolwin.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#if !defined(__linux__)
#include <wx/sound.h>
#endif

#include <audio/AlsaPlayer.h>
#include <landstalker/main/AudioData.h>
#include <landstalker/misc/Utils.h>
#include <misc/SpinCtrlSize.h>

namespace
{
	constexpr int WAVEFORM_HEIGHT = 90;
	constexpr int MAX_RATE = 0xFF;
	constexpr int MAX_BANK = 1;
	constexpr int MAX_LENGTH = 0xFFFF;
	constexpr int MAX_START_OFFSET = 0xFFFF;
	// Extra time added to a sample's own playback duration before OnPlayTimer assumes it has
	// finished. Only used on the wxSound path (Windows/macOS), which has no completion callback
	// to wait on instead - Linux's AlsaPlayer reports real completion, so it doesn't need this.
	constexpr int PLAYBACK_GRACE_MS = 300;

	// Wraps raw PCM bytes (mono, 8-bit unsigned - the Genesis DAC's native format, which is also
	// exactly what 8-bit WAV PCM is) in a minimal canonical WAV header, for wxSound::Create.
	std::vector<uint8_t> BuildWavBytes(const std::vector<uint8_t>& pcm, uint32_t sample_rate)
	{
		std::vector<uint8_t> wav;
		wav.reserve(44 + pcm.size());
		auto push_u32 = [&](uint32_t v)
			{
				wav.push_back(static_cast<uint8_t>(v & 0xFF));
				wav.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
				wav.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
				wav.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
			};
		auto push_u16 = [&](uint16_t v)
			{
				wav.push_back(static_cast<uint8_t>(v & 0xFF));
				wav.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
			};
		auto push_tag = [&](const char* s)
			{
				wav.insert(wav.end(), s, s + 4);
			};

		constexpr uint16_t CHANNELS = 1;
		constexpr uint16_t BITS_PER_SAMPLE = 8;
		const uint32_t data_size = static_cast<uint32_t>(pcm.size());
		const uint32_t byte_rate = sample_rate * CHANNELS * BITS_PER_SAMPLE / 8;
		const uint16_t block_align = CHANNELS * BITS_PER_SAMPLE / 8;

		push_tag("RIFF");
		push_u32(36 + data_size);
		push_tag("WAVE");
		push_tag("fmt ");
		push_u32(16);              // fmt chunk size
		push_u16(1);                // PCM
		push_u16(CHANNELS);
		push_u32(sample_rate);
		push_u32(byte_rate);
		push_u16(block_align);
		push_u16(BITS_PER_SAMPLE);
		push_tag("data");
		push_u32(data_size);
		wav.insert(wav.end(), pcm.begin(), pcm.end());
		return wav;
	}

	// Samples per side to linearly ramp to/from 128 (silence, for unsigned 8-bit PCM) at the very
	// start/end of playback.
	constexpr std::size_t DECLICK_RAMP_SAMPLES = 48;

	// The DAC (or ALSA's/the OS's output stage) sits at the neutral/silence level before playback
	// starts and after it ends. Most of these samples already end close to that level - so the
	// pop some samples produce isn't from the raw data's own start/end value, but from the abrupt
	// jump between "neutral" and wherever the very first/last output sample actually was: even
	// a handful of levels away from 128 is an audible click once it happens as a single-sample
	// step rather than a transition. A short linear fade in each direction removes that step
	// regardless of the exact cause, without perceptibly changing the sample. Only applied to what
	// gets played, not to Export WAV - that should stay a faithful copy of the ROM data.
	std::vector<uint8_t> ApplyDeclickRamp(std::vector<uint8_t> pcm)
	{
		const std::size_t ramp = std::min(DECLICK_RAMP_SAMPLES, pcm.size() / 4);
		for (std::size_t i = 0; i < ramp; ++i)
		{
			const double gain = static_cast<double>(i) / static_cast<double>(ramp);

			const int v_start = 128 + static_cast<int>(std::lround((static_cast<int>(pcm[i]) - 128) * gain));
			pcm[i] = static_cast<uint8_t>(std::clamp(v_start, 0, 255));

			const std::size_t j = pcm.size() - 1 - i;
			const int v_end = 128 + static_cast<int>(std::lround((static_cast<int>(pcm[j]) - 128) * gain));
			pcm[j] = static_cast<uint8_t>(std::clamp(v_end, 0, 255));
		}
		return pcm;
	}
}

// ---------------------------------------------------------------------------------------------
// WaveformPanel
// ---------------------------------------------------------------------------------------------

WaveformPanel::WaveformPanel(wxWindow* parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, WAVEFORM_HEIGHT))
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize(wxSize(-1, WAVEFORM_HEIGHT));
	Bind(wxEVT_PAINT, &WaveformPanel::OnPaint, this);
}

void WaveformPanel::SetData(const std::vector<uint8_t>& samples)
{
	m_samples = samples;
	Refresh();
}

void WaveformPanel::Clear()
{
	m_samples.clear();
	Refresh();
}

void WaveformPanel::OnPaint(wxPaintEvent&)
{
	wxAutoBufferedPaintDC dc(this);
	const wxSize size = GetClientSize();
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
	dc.Clear();

	const int width = size.GetWidth();
	const int height = size.GetHeight();
	const int mid = height / 2;

	dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT), 1, wxPENSTYLE_SHORT_DASH));
	dc.DrawLine(0, mid, width, mid);

	if (m_samples.empty() || width <= 0)
	{
		return;
	}

	// Mono 8bpp samples are unsigned, centred on 128. Draw a min/max envelope per pixel column
	// so the whole (typically much larger than the panel is wide) buffer is represented.
	dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT), 1));
	const double samples_per_px = static_cast<double>(m_samples.size()) / width;
	for (int x = 0; x < width; ++x)
	{
		const std::size_t start = static_cast<std::size_t>(x * samples_per_px);
		const std::size_t end = std::max(start + 1, static_cast<std::size_t>((x + 1) * samples_per_px));
		uint8_t lo = 255;
		uint8_t hi = 0;
		for (std::size_t i = start; i < end && i < m_samples.size(); ++i)
		{
			lo = std::min(lo, m_samples[i]);
			hi = std::max(hi, m_samples[i]);
		}
		const int y_lo = mid - (static_cast<int>(lo) - 128) * mid / 128;
		const int y_hi = mid - (static_cast<int>(hi) - 128) * mid / 128;
		dc.DrawLine(x, y_hi, x, y_lo + 1);
	}
}

// ---------------------------------------------------------------------------------------------
// SampleEditorFrame
// ---------------------------------------------------------------------------------------------

SampleEditorFrame::SampleEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst),
	  m_play_timer(this)
{
	m_mgr.SetManagedWindow(this);

	m_panel = new wxPanel(this, wxID_ANY);

	m_mgr.AddPane(m_panel, wxAuiPaneInfo().CenterPane());
	m_mgr.Update();

	Bind(wxEVT_TIMER, &SampleEditorFrame::OnPlayTimer, this);

	BuildUI();
}

SampleEditorFrame::~SampleEditorFrame()
{
	StopPlayback();
	m_mgr.UnInit();
}

wxSizer* SampleEditorFrame::BuildBankBox(wxWindow* parent, const wxString& title, int bank)
{
	auto* box = new wxStaticBoxSizer(wxVERTICAL, parent, title);

	m_waveform[bank] = new WaveformPanel(box->GetStaticBox());
	box->Add(m_waveform[bank], 0, wxEXPAND | wxALL, 4);

	m_bank_size[bank] = new wxStaticText(box->GetStaticBox(), wxID_ANY, wxEmptyString);
	box->Add(m_bank_size[bank], 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);

	auto* buttons = new wxBoxSizer(wxHORIZONTAL);
	m_import_btn[bank] = new wxButton(box->GetStaticBox(), wxID_ANY, "Import...");
	m_export_btn[bank] = new wxButton(box->GetStaticBox(), wxID_ANY, "Export...");
	m_import_btn[bank]->Bind(wxEVT_BUTTON, [this, bank](wxCommandEvent&) { OnImport(bank); });
	m_export_btn[bank]->Bind(wxEVT_BUTTON, [this, bank](wxCommandEvent&) { OnExport(bank); });
	buttons->Add(m_import_btn[bank], 0, wxRIGHT, 6);
	buttons->Add(m_export_btn[bank], 0);
	box->Add(buttons, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);

	return box;
}

void SampleEditorFrame::UpdateBankSizeLabel(int bank)
{
	if (!m_bank_size[bank])
	{
		return;
	}
	if (!m_gd)
	{
		m_bank_size[bank]->SetLabel(wxEmptyString);
		return;
	}
	auto ad = m_gd->GetAudioData();
	const auto& bytes = (bank == 0) ? ad->GetPcmBank0() : ad->GetPcmBank1();
	const std::size_t capacity = Landstalker::AudioData::GetPcmBankCapacity();
	m_bank_size[bank]->SetLabel(wxString::Format("Size: %u / %u bytes",
		static_cast<unsigned>(bytes.size()), static_cast<unsigned>(capacity)));
	if (bytes.size() > capacity)
	{
		m_bank_size[bank]->SetForegroundColour(*wxRED);
	}
	else
	{
		m_bank_size[bank]->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	}
}

void SampleEditorFrame::BuildUI()
{
	m_panel->Freeze();

	auto* top = new wxBoxSizer(wxVERTICAL);
	top->Add(BuildBankBox(m_panel, "PCM Bank 0", 0), 0, wxEXPAND | wxALL, 8);
	top->Add(BuildBankBox(m_panel, "PCM Bank 1", 1), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	auto* table_box = new wxStaticBoxSizer(wxVERTICAL, m_panel, "Sample Directory");
	m_table_panel = new wxScrolledWindow(table_box->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxVSCROLL | wxHSCROLL | wxTAB_TRAVERSAL);
	m_table_panel->SetScrollRate(16, 16);
	// SetDoubleBuffered(true) trades one problem for another here: it stops the flicker but the
	// extra off-screen composite step is expensive enough with this many child widgets to make
	// scrolling noticeably slower, which matters more. Single-buffered (the default) it is.
	table_box->Add(m_table_panel, 1, wxEXPAND | wxALL, 4);
	top->Add(table_box, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	m_panel->SetSizer(top);
	m_panel->Layout();
	m_panel->Thaw();
}

namespace
{
	enum MENU_IDS
	{
		ID_FILE_EXPORT_YAML = 20000,
		ID_FILE_IMPORT_YAML
	};
}

void SampleEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YAML, "Export Sample Table as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YAML, "Import Sample Table from YAML...");
	RefreshMenuEnable();
}

void SampleEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_YAML:
		OnExportYaml();
		break;
	case ID_FILE_IMPORT_YAML:
		OnImportYaml();
		break;
	}
}

void SampleEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void SampleEditorFrame::RefreshMenuEnable() const
{
	EnableMenuItem(ID_FILE_EXPORT_YAML, static_cast<bool>(m_gd));
	EnableMenuItem(ID_FILE_IMPORT_YAML, static_cast<bool>(m_gd));
}

void SampleEditorFrame::OnExportYaml()
{
	if (!m_gd)
	{
		return;
	}
	CommitPendingEdits();
	ExportYamlWithDialog(this, "Export Sample Table as YAML", "samples.yaml",
		[&](YAML::Emitter& out) { EmitPcmSampleTableYaml(out, m_gd->GetAudioData()->GetPcmSampleTable()); });
}

void SampleEditorFrame::OnImportYaml()
{
	if (!m_gd)
	{
		return;
	}
	if (ImportYamlWithDialog(this, "Import Sample Table from YAML", [&](const YAML::Node& root)
		{
			const auto table = PcmSampleTableFromYaml(root);
			auto ad = m_gd->GetAudioData();
			if (table.size() > ad->GetMaxPcmSampleCount())
			{
				throw std::runtime_error("The file holds " + std::to_string(table.size())
					+ " samples, but this project only has room for " + std::to_string(ad->GetMaxPcmSampleCount()));
			}
			ad->SetPcmSampleTable(table);
		}))
	{
		LoadValues();
	}
}

bool SampleEditorFrame::Open()
{
	if (!m_gd)
	{
		return false;
	}
	LoadValues();
	return true;
}

void SampleEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	LoadValues();
}

void SampleEditorFrame::ClearGameData()
{
	StopPlayback();
	m_gd.reset();
	for (auto* waveform : m_waveform)
	{
		if (waveform)
		{
			waveform->Clear();
		}
	}
	for (auto* btn : m_import_btn)
	{
		if (btn) btn->Enable(false);
	}
	for (auto* btn : m_export_btn)
	{
		if (btn) btn->Enable(false);
	}
	UpdateBankSizeLabel(0);
	UpdateBankSizeLabel(1);
	m_table.clear();
	RebuildRows();
}

void SampleEditorFrame::LoadValues()
{
	const bool have_data = static_cast<bool>(m_gd);
	for (auto* btn : m_import_btn)
	{
		if (btn) btn->Enable(have_data);
	}
	for (auto* btn : m_export_btn)
	{
		if (btn) btn->Enable(have_data);
	}
	if (!have_data)
	{
		m_table.clear();
		UpdateBankSizeLabel(0);
		UpdateBankSizeLabel(1);
		RebuildRows();
		return;
	}
	auto ad = m_gd->GetAudioData();
	m_waveform[0]->SetData(ad->GetPcmBank0());
	m_waveform[1]->SetData(ad->GetPcmBank1());
	UpdateBankSizeLabel(0);
	UpdateBankSizeLabel(1);
	m_table = ad->GetPcmSampleTable();
	RebuildRows();
}

void SampleEditorFrame::OnImport(int bank)
{
	if (!m_gd)
	{
		return;
	}
	wxFileDialog fd(this, wxString::Format("Import PCM Bank %d", bank), "", "",
		"Raw PCM Sample Data (*.bin;*.pcm;*.raw)|*.bin;*.pcm;*.raw|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	const auto bytes = Landstalker::ReadBytes(chosen.string());
	if (bytes.empty())
	{
		wxMessageBox("Unable to read sample data from the selected file.",
			"Import PCM Bank", wxOK | wxICON_ERROR, this);
		return;
	}
	const std::size_t capacity = Landstalker::AudioData::GetPcmBankCapacity();
	if (bytes.size() > capacity)
	{
		wxMessageBox(wxString::Format(
			"This file is %u bytes, which is larger than the %u-byte PCM bank it would replace. "
			"It would not fit when the project is injected into a ROM, so it has not been imported.",
			static_cast<unsigned>(bytes.size()), static_cast<unsigned>(capacity)),
			"Import PCM Bank", wxOK | wxICON_ERROR, this);
		return;
	}
	auto ad = m_gd->GetAudioData();
	if (bank == 0)
	{
		ad->SetPcmBank0(bytes);
	}
	else
	{
		ad->SetPcmBank1(bytes);
	}
	m_waveform[bank]->SetData(bytes);
	UpdateBankSizeLabel(bank);
}

void SampleEditorFrame::OnExport(int bank)
{
	if (!m_gd)
	{
		return;
	}
	auto ad = m_gd->GetAudioData();
	const auto& bytes = (bank == 0) ? ad->GetPcmBank0() : ad->GetPcmBank1();

	wxFileDialog fd(this, wxString::Format("Export PCM Bank %d", bank), "",
		wxString::Format("pcmbank%d.bin", bank),
		"Raw PCM Sample Data (*.bin)|*.bin|All Files (*.*)|*.*",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const std::filesystem::path chosen(fd.GetPath().ToStdString());
	Landstalker::WriteBytes(bytes, chosen.string());
}

void SampleEditorFrame::RebuildRows()
{
	if (!m_table_panel)
	{
		return;
	}
	// The rows (and their Play/Stop buttons) are about to be destroyed; drop any playback state
	// that points at them first, rather than leaving m_playing_row referring to a button that no
	// longer exists (or, after the rebuild, a different row's).
	StopPlayback();
	m_populating = true;
	m_table_panel->Freeze();
	m_rows.clear();
	m_table_panel->DestroyChildren();

	if (!m_gd)
	{
		m_table_panel->SetSizer(nullptr);
		m_table_panel->Layout();
		m_table_panel->Thaw();
		m_populating = false;
		return;
	}

	const std::size_t max_count = m_gd->GetAudioData()->GetMaxPcmSampleCount();
	const bool can_add = m_table.size() < max_count;

	auto* top = new wxBoxSizer(wxVERTICAL);
	auto* grid = new wxFlexGridSizer(10, wxSize(18, 6));
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, "#"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, "Rate"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, "Sample Rate"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, "Bank"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, "Length"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, "Start Offset"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, "Duration"), 0, wxALIGN_CENTER);
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, wxEmptyString), 0);
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, wxEmptyString), 0);
	grid->Add(new wxStaticText(m_table_panel, wxID_ANY, wxEmptyString), 0);

	const std::size_t total = m_table.size() + (can_add ? 1 : 0);
	for (std::size_t r = 0; r < total; ++r)
	{
		const bool blank = (r == m_table.size());
		const PcmSample sample = blank ? PcmSample{} : m_table[r];

		SampleRow row;
		row.is_blank = blank;

		row.number = new wxStaticText(m_table_panel, wxID_ANY,
			blank ? wxString("*") : wxString::Format("%u", static_cast<unsigned>(r)));
		grid->Add(row.number, 0, wxALIGN_CENTER);

		row.rate = new wxSpinCtrl(m_table_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(70),
			wxSP_ARROW_KEYS, 0, MAX_RATE, sample.rate);
		grid->Add(row.rate, 0, wxALIGN_CENTER);

		row.rate_hz = new wxStaticText(m_table_panel, wxID_ANY,
			wxString::Format("%u Hz", Landstalker::AudioData::GetPcmSampleRateHz(sample.rate)),
			wxDefaultPosition, wxSize(80, -1), wxALIGN_RIGHT);
		grid->Add(row.rate_hz, 0, wxALIGN_CENTER_VERTICAL);

		row.bank = new wxSpinCtrl(m_table_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(60),
			wxSP_ARROW_KEYS, 0, MAX_BANK, sample.bank);
		grid->Add(row.bank, 0, wxALIGN_CENTER);

		row.length = new wxSpinCtrl(m_table_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(90),
			wxSP_ARROW_KEYS, 0, MAX_LENGTH, sample.length);
		grid->Add(row.length, 0, wxALIGN_CENTER);

		row.start_offset = new wxSpinCtrl(m_table_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(90),
			wxSP_ARROW_KEYS, 0, MAX_START_OFFSET, sample.start_offset);
		grid->Add(row.start_offset, 0, wxALIGN_CENTER);

		row.duration = new wxStaticText(m_table_panel, wxID_ANY, wxEmptyString,
			wxDefaultPosition, wxSize(70, -1), wxALIGN_RIGHT);
		grid->Add(row.duration, 0, wxALIGN_CENTER_VERTICAL);
		UpdateRowReadouts(row); // seed the derived Hz/duration labels from the initial values

		if (blank)
		{
			// Commit only when a field settles (spin buttons / focus loss), never per keystroke -
			// a rebuild would otherwise destroy the control being edited.
			for (auto* sp : { row.bank, row.length, row.start_offset })
			{
				sp->Bind(wxEVT_SPINCTRL, [this, r](wxSpinEvent&) { OnRowEdited(r); });
				sp->Bind(wxEVT_KILL_FOCUS, [this, r](wxFocusEvent& e) { e.Skip(); OnRowEdited(r); });
			}
			row.rate->Bind(wxEVT_SPINCTRL, [this, r](wxSpinEvent&) { OnRowEdited(r); });
			row.rate->Bind(wxEVT_KILL_FOCUS, [this, r](wxFocusEvent& e) { e.Skip(); OnRowEdited(r); });
			// The blank row doesn't commit per keystroke, but its Hz/duration readouts should still
			// track what's being typed - so update them live without touching the data.
			row.rate->Bind(wxEVT_TEXT, [this, row](wxCommandEvent&) { UpdateRowReadouts(row); });
			row.length->Bind(wxEVT_TEXT, [this, row](wxCommandEvent&) { UpdateRowReadouts(row); });
			grid->Add(new wxStaticText(m_table_panel, wxID_ANY, wxEmptyString), 0);
			grid->Add(new wxStaticText(m_table_panel, wxID_ANY, wxEmptyString), 0);
			grid->Add(new wxStaticText(m_table_panel, wxID_ANY, wxEmptyString), 0);
		}
		else
		{
			for (auto* sp : { row.rate, row.bank, row.length, row.start_offset })
			{
				sp->Bind(wxEVT_TEXT, [this, r](wxCommandEvent&) { OnRowEdited(r); });
				sp->Bind(wxEVT_SPINCTRL, [this, r](wxSpinEvent&) { OnRowEdited(r); });
			}
			// Fixed sizes, not wxBU_EXACTFIT: Play's label toggles to "Stop", and a size that
			// tracks the label would shift the sizer's column widths (and everything after it)
			// every time it's clicked.
			row.play = new wxButton(m_table_panel, wxID_ANY, "Play", wxDefaultPosition, wxSize(70, -1));
			row.play->Bind(wxEVT_BUTTON, [this, r](wxCommandEvent&) { OnPlayRow(r); });
			grid->Add(row.play, 0, wxALIGN_CENTER);
			row.export_wav = new wxButton(m_table_panel, wxID_ANY, "Export WAV...", wxDefaultPosition, wxSize(110, -1));
			row.export_wav->Bind(wxEVT_BUTTON, [this, r](wxCommandEvent&) { OnExportSampleRow(r); });
			grid->Add(row.export_wav, 0, wxALIGN_CENTER);
			row.del = new wxButton(m_table_panel, wxID_ANY, "Delete", wxDefaultPosition, wxSize(70, -1));
			row.del->Bind(wxEVT_BUTTON, [this, r](wxCommandEvent&) { OnDeleteRow(r); });
			grid->Add(row.del, 0, wxALIGN_CENTER);
		}

		m_rows.push_back(row);
	}

	top->Add(grid, 0, wxALL, 6);
	if (!can_add)
	{
		top->Add(new wxStaticText(m_table_panel, wxID_ANY,
			wxString::Format("Maximum of %u entries reached.", static_cast<unsigned>(max_count))),
			0, wxLEFT | wxBOTTOM, 6);
	}
	m_table_panel->SetSizer(top);
	m_table_panel->FitInside();
	m_table_panel->Layout();
	m_table_panel->Thaw();
	m_populating = false;
}

void SampleEditorFrame::UpdateRowReadouts(const SampleRow& row) const
{
	if (!row.rate)
	{
		return;
	}
	const uint32_t hz = Landstalker::AudioData::GetPcmSampleRateHz(static_cast<uint8_t>(row.rate->GetValue()));
	if (row.rate_hz)
	{
		row.rate_hz->SetLabel(wxString::Format("%u Hz", hz));
	}
	if (row.duration)
	{
		// length is the sample count (mono 8bpp = one byte per sample); hz is always > 0.
		const unsigned length = row.length ? static_cast<unsigned>(row.length->GetValue()) : 0;
		row.duration->SetLabel(wxString::Format("%.3f s", static_cast<double>(length) / hz));
	}
}

SampleEditorFrame::PcmSample SampleEditorFrame::ReadRow(const SampleRow& row) const
{
	PcmSample s;
	s.rate = row.rate ? static_cast<uint8_t>(row.rate->GetValue()) : 0;
	s.bank = row.bank ? static_cast<uint8_t>(row.bank->GetValue()) : 0;
	s.length = row.length ? static_cast<uint16_t>(row.length->GetValue()) : 0;
	s.start_offset = row.start_offset ? static_cast<uint16_t>(row.start_offset->GetValue()) : 0;
	return s;
}

bool SampleEditorFrame::RowIsBlank(const SampleRow& row) const
{
	return ReadRow(row).length == 0;
}

void SampleEditorFrame::OnRowEdited(std::size_t row)
{
	if (m_populating || !m_gd || row >= m_rows.size())
	{
		return;
	}
	SampleRow& r = m_rows[row];
	// Keep the derived Hz/duration readouts in sync with whatever was just edited, regardless of
	// whether the change ends up committing below.
	UpdateRowReadouts(r);
	if (r.is_blank)
	{
		if (RowIsBlank(r))
		{
			return; // untouched blank - never committed
		}
		if (m_table.size() >= m_gd->GetAudioData()->GetMaxPcmSampleCount())
		{
			return; // at cap - the blank row shouldn't be shown, but guard anyway
		}
		m_table.push_back(ReadRow(r));
		m_gd->GetAudioData()->SetPcmSampleTable(m_table);
		CallAfter([this] { RebuildRows(); });
		return;
	}
	if (row < m_table.size())
	{
		auto entry = ReadRow(r);
		entry.reserved = m_table[row].reserved;
		entry.reserved2 = m_table[row].reserved2;
		m_table[row] = entry;
		m_gd->GetAudioData()->SetPcmSampleTable(m_table);
	}
}

void SampleEditorFrame::OnDeleteRow(std::size_t row)
{
	if (!m_gd || row >= m_table.size())
	{
		return;
	}
	m_table.erase(m_table.begin() + row);
	m_gd->GetAudioData()->SetPcmSampleTable(m_table);
	CallAfter([this] { RebuildRows(); });
}

void SampleEditorFrame::CommitPendingEdits()
{
	if (!m_gd)
	{
		return;
	}
	for (std::size_t r = 0; r < m_table.size() && r < m_rows.size(); ++r)
	{
		if (!m_rows[r].is_blank)
		{
			auto entry = ReadRow(m_rows[r]);
			entry.reserved = m_table[r].reserved;
			entry.reserved2 = m_table[r].reserved2;
			m_table[r] = entry;
		}
	}
	if (!m_rows.empty() && m_rows.back().is_blank && !RowIsBlank(m_rows.back()) &&
		m_table.size() < m_gd->GetAudioData()->GetMaxPcmSampleCount())
	{
		m_table.push_back(ReadRow(m_rows.back()));
	}
	m_gd->GetAudioData()->SetPcmSampleTable(m_table);
}

bool SampleEditorFrame::ExtractSamplePcm(std::size_t row, std::vector<uint8_t>& pcm, uint32_t& rate_hz) const
{
	if (!m_gd || row >= m_table.size())
	{
		return false;
	}
	const auto& sample = m_table[row];
	auto ad = m_gd->GetAudioData();
	const auto& bank = (sample.bank == 0) ? ad->GetPcmBank0() : ad->GetPcmBank1();
	if (sample.start_offset >= bank.size())
	{
		return false;
	}
	const std::size_t end = std::min(bank.size(), static_cast<std::size_t>(sample.start_offset) + sample.length);
	if (end <= sample.start_offset)
	{
		return false;
	}
	pcm.assign(bank.begin() + sample.start_offset, bank.begin() + end);
	rate_hz = Landstalker::AudioData::GetPcmSampleRateHz(sample.rate);
	return true;
}

void SampleEditorFrame::OnExportSampleRow(std::size_t row)
{
	std::vector<uint8_t> pcm;
	uint32_t rate_hz = 0;
	if (!ExtractSamplePcm(row, pcm, rate_hz))
	{
		wxMessageBox("This sample has no valid data to export.", "Export Sample", wxOK | wxICON_WARNING, this);
		return;
	}
	const auto wav = BuildWavBytes(pcm, rate_hz);

	wxFileDialog fd(this, wxString::Format("Export Sample %u", static_cast<unsigned>(row)), "",
		wxString::Format("sample_%02u.wav", static_cast<unsigned>(row)),
		"WAV Audio (*.wav)|*.wav|All Files (*.*)|*.*",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	Landstalker::WriteBytes(wav, fd.GetPath().ToStdString());
}

bool SampleEditorFrame::StartPlayback(const std::vector<uint8_t>& pcm, uint32_t rate_hz, std::size_t row)
{
#if defined(__linux__)
	// Real completion detection: AlsaPlayer's engine thread fires this once the sample has
	// actually played out of the device (it tracks the device delay through the trailing
	// silence), so the button flips back exactly when the audio ends. The row check
	// guards against a stale completion for a row that isn't the current one anymore - e.g. it
	// finished naturally at (almost) the same moment the user clicked Play on another row.
	return m_alsa.Play(pcm, rate_hz, [this, row]()
		{
			CallAfter([this, row]()
				{
					if (m_playing_row == static_cast<int>(row))
					{
						ResetPlaybackState();
					}
				});
		});
#else
	const auto wav = BuildWavBytes(pcm, rate_hz);
	auto sound = std::make_unique<wxSound>();
	if (!sound->Create(wav.size(), wav.data()) || !sound->IsOk())
	{
		wxMessageBox("Unable to play this sample on this system.", "Play Sample", wxOK | wxICON_WARNING, this);
		return false;
	}
	sound->Play(wxSOUND_ASYNC);
	m_sound = std::move(sound);

	// wxSound has no completion callback, so approximate it: the sample's own duration plus a
	// grace margin for device startup latency, past which we assume playback has finished.
	const double duration_ms = pcm.size() * 1000.0 / rate_hz;
	m_play_timer.StartOnce(std::max(50, static_cast<int>(duration_ms)) + PLAYBACK_GRACE_MS);
	return true;
#endif
}

void SampleEditorFrame::OnPlayRow(std::size_t row)
{
	if (!m_gd || row >= m_table.size())
	{
		return;
	}
	if (m_playing_row == static_cast<int>(row))
	{
		StopPlayback();
		return;
	}
	StopPlayback(); // only one sample plays at a time

	std::vector<uint8_t> pcm;
	uint32_t rate_hz = 0;
	if (!ExtractSamplePcm(row, pcm, rate_hz))
	{
		return;
	}
	if (!StartPlayback(ApplyDeclickRamp(std::move(pcm)), rate_hz, row))
	{
		return;
	}
	m_playing_row = static_cast<int>(row);
	if (row < m_rows.size() && m_rows[row].play)
	{
		m_rows[row].play->SetLabel("Stop");
	}
}

void SampleEditorFrame::OnPlayTimer(wxTimerEvent&)
{
	// wxSound path only (see StartPlayback) - assumed to have finished naturally. Do NOT
	// interrupt it here (that's what StopPlayback does): this used to unconditionally tear down
	// playback as soon as the timer fired, which on the old Unix subprocess path cut the tail off
	// mid-stream and produced an audible pop/click on every single sample.
	ResetPlaybackState();
}

void SampleEditorFrame::StopPlayback()
{
#if defined(__linux__)
	m_alsa.Stop();
#else
	wxSound::Stop();
#endif
	ResetPlaybackState();
}

void SampleEditorFrame::ResetPlaybackState()
{
	m_play_timer.Stop();
#if !defined(__linux__)
	m_sound.reset();
#endif
	if (m_playing_row >= 0 && static_cast<std::size_t>(m_playing_row) < m_rows.size() && m_rows[m_playing_row].play)
	{
		m_rows[m_playing_row].play->SetLabel("Play");
	}
	m_playing_row = -1;
}

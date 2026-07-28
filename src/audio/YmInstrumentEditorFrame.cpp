#include <audio/YmInstrumentEditorFrame.h>

#include <fstream>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <audio/YmInstrumentYaml.h>
#include <audio/YamlIo.h>
#include <landstalker/misc/Labels.h>
#include <misc/SpinCtrlSize.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_YAML = 20000,
	ID_FILE_IMPORT_YAML
};

namespace
{
	using Landstalker::MusicData;

	// UI operator column (OP1-OP4) -> byte position within a four-byte register group. The bytes
	// are stored in YM2612 slot order S1, S3, S2, S4, so OP2 and OP3 swap.
	constexpr std::size_t OP_STORAGE_INDEX[4] = { 0, 2, 1, 3 };

	constexpr const char* OP_LABELS[4] = { "OP1", "OP2", "OP3", "OP4" };
}

std::size_t YmInstrumentEditorFrame::ByteIndex(std::size_t group_offset, std::size_t op)
{
	return group_offset + OP_STORAGE_INDEX[op];
}

YmInstrumentEditorFrame::YmInstrumentEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	BuildUI();
}

YmInstrumentEditorFrame::~YmInstrumentEditorFrame()
{
}

void YmInstrumentEditorFrame::BuildUI()
{
	Freeze();
	m_panel = new wxPanel(this);
	auto* top = new wxBoxSizer(wxHORIZONTAL);

	// See SoundEventListColumn for why the list gets its own wrapper panel: a wxListCtrl parented
	// directly under a wxStaticBoxSizer's box computes a wildly inflated minimum width on
	// wx/GTK, and an intermediate panel's normal min-size computation sidesteps that.
	auto* list_box = new wxStaticBoxSizer(wxVERTICAL, m_panel, "Instruments");
	auto* list_wrap = new wxPanel(list_box->GetStaticBox());
	auto* list_wrap_sizer = new wxBoxSizer(wxVERTICAL);
	m_list = new wxListView(list_wrap, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	m_list->AppendColumn("#", wxLIST_FORMAT_RIGHT, 45);
	m_list->AppendColumn("Name", wxLIST_FORMAT_LEFT, 150);
	m_list->AppendColumn("Summary", wxLIST_FORMAT_LEFT, 95);
	m_list->SetMinSize(wxSize(45 + 150 + 95 + 24, -1));
	m_list->Bind(wxEVT_LIST_ITEM_SELECTED, &YmInstrumentEditorFrame::OnListSelection, this);
	list_wrap_sizer->Add(m_list, 1, wxEXPAND);
	list_wrap->SetSizer(list_wrap_sizer);
	list_box->Add(list_wrap, 1, wxEXPAND | wxALL, 3);
	top->Add(list_box, 0, wxEXPAND | wxALL, 8);

	auto* detail = new wxBoxSizer(wxVERTICAL);

	auto* header_box = new wxStaticBoxSizer(wxHORIZONTAL, m_panel, "Instrument");
	wxWindow* header_parent = header_box->GetStaticBox();
	m_name_label = new wxStaticText(header_parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1));
	header_box->Add(m_name_label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 6);
	m_rename_btn = new wxButton(header_parent, wxID_ANY, "Rename...");
	m_rename_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRename(); });
	header_box->Add(m_rename_btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
	header_box->Add(new wxStaticText(header_parent, wxID_ANY, "Algorithm"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
	m_algorithm = new wxSpinCtrl(header_parent, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(70),
		wxSP_ARROW_KEYS, 0, 7, 0);
	header_box->Add(m_algorithm, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
	header_box->Add(new wxStaticText(header_parent, wxID_ANY, "Feedback"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
	m_feedback = new wxSpinCtrl(header_parent, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(70),
		wxSP_ARROW_KEYS, 0, 7, 0);
	header_box->Add(m_feedback, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
	detail->Add(header_box, 0, wxEXPAND | wxTOP | wxRIGHT, 8);

	auto* ops_box = new wxStaticBoxSizer(wxVERTICAL, m_panel, "Operators");
	wxWindow* ops_parent = ops_box->GetStaticBox();
	auto* grid = new wxFlexGridSizer(5, wxSize(10, 4));

	const auto add_row_label = [&](const wxString& text)
	{
		grid->Add(new wxStaticText(ops_parent, wxID_ANY, text), 0, wxALIGN_CENTER_VERTICAL);
	};
	const auto add_spin = [&](wxSpinCtrl*& ctrl, int max)
	{
		ctrl = new wxSpinCtrl(ops_parent, wxID_ANY, wxEmptyString, wxDefaultPosition, SpinCtrlSize(75),
			wxSP_ARROW_KEYS, 0, max, 0);
		ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { CommitDetail(); });
		ctrl->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { e.Skip(); CommitDetail(); });
		grid->Add(ctrl, 0);
	};

	grid->Add(new wxStaticText(ops_parent, wxID_ANY, wxEmptyString), 0);
	for (std::size_t op = 0; op < 4; ++op)
	{
		grid->Add(new wxStaticText(ops_parent, wxID_ANY, OP_LABELS[op]), 0, wxALIGN_CENTER_HORIZONTAL);
	}

	add_row_label("Detune (DT)");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].detune, 7);
	add_row_label("Multiple (MUL)");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].multiple, 15);
	add_row_label("Total Level (TL)");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].total_level, 127);
	add_row_label("Rate Scaling (RS)");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].rate_scaling, 3);
	add_row_label("Attack Rate (AR)");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].attack_rate, 31);
	add_row_label("LFO AM Enable");
	for (std::size_t op = 0; op < 4; ++op)
	{
		m_ops[op].am_enable = new wxCheckBox(ops_parent, wxID_ANY, wxEmptyString);
		m_ops[op].am_enable->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { CommitDetail(); });
		grid->Add(m_ops[op].am_enable, 0, wxALIGN_CENTER_HORIZONTAL);
	}
	add_row_label("Decay Rate (D1R)");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].decay_rate, 31);
	add_row_label("Sustain Rate (D2R)");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].sustain_rate, 31);
	add_row_label("Sustain Level (SL)");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].sustain_level, 15);
	add_row_label("Release Rate (RR)");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].release_rate, 15);
	add_row_label("SSG-EG");
	for (std::size_t op = 0; op < 4; ++op) add_spin(m_ops[op].ssg_eg, 15);

	ops_box->Add(grid, 0, wxALL, 6);
	detail->Add(ops_box, 0, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 8);

	m_algorithm->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { CommitDetail(); });
	m_algorithm->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { e.Skip(); CommitDetail(); });
	m_feedback->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { CommitDetail(); });
	m_feedback->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { e.Skip(); CommitDetail(); });

	top->Add(detail, 1, wxEXPAND);
	m_panel->SetSizer(top);

	auto* outer = new wxBoxSizer(wxVERTICAL);
	outer->Add(m_panel, 1, wxEXPAND);
	SetSizer(outer);

	LoadDetail();
	Layout();
	Thaw();
}

wxString YmInstrumentEditorFrame::NameFor(std::size_t index) const
{
	// Names live in the shared Labels store (persisted in the project's *_labels.yaml), with a
	// generated "Instrument NN" default pre-seeding any patch that hasn't been named yet.
	const auto label = Landstalker::Labels::Get(Landstalker::Labels::C_YM_INSTRUMENTS, static_cast<int>(index));
	return label ? wxString(*label) : wxString::Format("Instrument %02zX", index);
}

void YmInstrumentEditorFrame::OnRename()
{
	if (!m_gd || m_index >= MusicData::YM_INSTRUMENT_COUNT)
	{
		return;
	}
	wxTextEntryDialog dlg(this, wxString::Format("New name for instrument %02zXh:", m_index),
		"Rename Instrument", NameFor(m_index));
	if (dlg.ShowModal() != wxID_OK)
	{
		return;
	}
	const std::wstring name = dlg.GetValue().ToStdWstring();
	if (!Landstalker::Labels::Update(Landstalker::Labels::C_YM_INSTRUMENTS, static_cast<int>(m_index), name))
	{
		wxMessageBox("That name is invalid or already in use by another label.", "Rename Instrument",
			wxOK | wxICON_ERROR, this);
		return;
	}
	m_list->SetItem(static_cast<long>(m_index), 1, NameFor(m_index));
	LoadDetail();
}

wxString YmInstrumentEditorFrame::SummaryFor(const YmInstrument& instrument) const
{
	const uint8_t fb_alg = instrument[28];
	return wxString::Format("Alg %u, FB %u", fb_alg & 0x07, (fb_alg >> 3) & 0x07);
}

void YmInstrumentEditorFrame::RefreshList()
{
	m_list->Freeze();
	m_list->DeleteAllItems();
	if (m_gd)
	{
		const auto& instruments = m_gd->GetMusicData()->GetYmInstruments();
		for (std::size_t i = 0; i < instruments.size(); ++i)
		{
			const long row = m_list->InsertItem(static_cast<long>(i), wxString::Format("%02zXh", i));
			m_list->SetItem(row, 1, NameFor(i));
			m_list->SetItem(row, 2, SummaryFor(instruments[i]));
		}
	}
	m_list->Thaw();
}

void YmInstrumentEditorFrame::RefreshListRow(long row)
{
	if (!m_gd || row < 0 || static_cast<std::size_t>(row) >= MusicData::YM_INSTRUMENT_COUNT)
	{
		return;
	}
	m_list->SetItem(row, 2, SummaryFor(m_gd->GetMusicData()->GetYmInstruments()[row]));
}

void YmInstrumentEditorFrame::LoadDetail()
{
	m_populating = true;
	const bool have = m_gd && m_index < MusicData::YM_INSTRUMENT_COUNT;
	m_name_label->SetLabel(have ? wxString::Format("[%02zX] ", m_index) + NameFor(m_index) : wxString(wxEmptyString));
	m_rename_btn->Enable(have);
	m_feedback->Enable(have);
	m_algorithm->Enable(have);
	for (auto& op : m_ops)
	{
		op.detune->Enable(have);
		op.multiple->Enable(have);
		op.total_level->Enable(have);
		op.rate_scaling->Enable(have);
		op.attack_rate->Enable(have);
		op.am_enable->Enable(have);
		op.decay_rate->Enable(have);
		op.sustain_rate->Enable(have);
		op.sustain_level->Enable(have);
		op.release_rate->Enable(have);
		op.ssg_eg->Enable(have);
	}
	if (!have)
	{
		m_populating = false;
		return;
	}
	const auto& inst = m_gd->GetMusicData()->GetYmInstruments()[m_index];
	m_algorithm->SetValue(inst[28] & 0x07);
	m_feedback->SetValue((inst[28] >> 3) & 0x07);
	for (std::size_t op = 0; op < 4; ++op)
	{
		auto& c = m_ops[op];
		c.detune->SetValue((inst[ByteIndex(0, op)] >> 4) & 0x07);
		c.multiple->SetValue(inst[ByteIndex(0, op)] & 0x0F);
		c.total_level->SetValue(inst[ByteIndex(4, op)] & 0x7F);
		c.rate_scaling->SetValue((inst[ByteIndex(8, op)] >> 6) & 0x03);
		c.attack_rate->SetValue(inst[ByteIndex(8, op)] & 0x1F);
		c.am_enable->SetValue((inst[ByteIndex(12, op)] & 0x80) != 0);
		c.decay_rate->SetValue(inst[ByteIndex(12, op)] & 0x1F);
		c.sustain_rate->SetValue(inst[ByteIndex(16, op)] & 0x1F);
		c.sustain_level->SetValue((inst[ByteIndex(20, op)] >> 4) & 0x0F);
		c.release_rate->SetValue(inst[ByteIndex(20, op)] & 0x0F);
		c.ssg_eg->SetValue(inst[ByteIndex(24, op)] & 0x0F);
	}
	m_populating = false;
}

void YmInstrumentEditorFrame::CommitDetail()
{
	if (m_populating || !m_gd || m_index >= MusicData::YM_INSTRUMENT_COUNT)
	{
		return;
	}
	auto md = m_gd->GetMusicData();
	auto instruments = md->GetYmInstruments();
	auto& inst = instruments[m_index];

	// Each write is a read-modify-write of only that field's bits, so bits outside the documented
	// fields survive editing (see the class comment).
	inst[28] = static_cast<uint8_t>((inst[28] & 0xC0)
		| ((m_feedback->GetValue() & 0x07) << 3) | (m_algorithm->GetValue() & 0x07));
	for (std::size_t op = 0; op < 4; ++op)
	{
		const auto& c = m_ops[op];
		auto& dt_mul = inst[ByteIndex(0, op)];
		dt_mul = static_cast<uint8_t>((dt_mul & 0x80) | ((c.detune->GetValue() & 0x07) << 4) | (c.multiple->GetValue() & 0x0F));
		auto& tl = inst[ByteIndex(4, op)];
		tl = static_cast<uint8_t>((tl & 0x80) | (c.total_level->GetValue() & 0x7F));
		auto& rs_ar = inst[ByteIndex(8, op)];
		rs_ar = static_cast<uint8_t>((rs_ar & 0x20) | ((c.rate_scaling->GetValue() & 0x03) << 6) | (c.attack_rate->GetValue() & 0x1F));
		auto& am_dr = inst[ByteIndex(12, op)];
		am_dr = static_cast<uint8_t>((am_dr & 0x60) | (c.am_enable->GetValue() ? 0x80 : 0) | (c.decay_rate->GetValue() & 0x1F));
		auto& sr = inst[ByteIndex(16, op)];
		sr = static_cast<uint8_t>((sr & 0xE0) | (c.sustain_rate->GetValue() & 0x1F));
		inst[ByteIndex(20, op)] = static_cast<uint8_t>(((c.sustain_level->GetValue() & 0x0F) << 4) | (c.release_rate->GetValue() & 0x0F));
		auto& ssg = inst[ByteIndex(24, op)];
		ssg = static_cast<uint8_t>((ssg & 0xF0) | (c.ssg_eg->GetValue() & 0x0F));
	}
	md->SetYmInstruments(instruments);
	RefreshListRow(static_cast<long>(m_index));
}

void YmInstrumentEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YAML, "Export Instruments as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YAML, "Import Instruments from YAML...");
	RefreshMenuEnable();
}

void YmInstrumentEditorFrame::OnMenuClick(wxMenuEvent& evt)
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

void YmInstrumentEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void YmInstrumentEditorFrame::RefreshMenuEnable() const
{
	EnableMenuItem(ID_FILE_EXPORT_YAML, static_cast<bool>(m_gd));
	EnableMenuItem(ID_FILE_IMPORT_YAML, static_cast<bool>(m_gd));
}

void YmInstrumentEditorFrame::OnExportYaml()
{
	if (!m_gd)
	{
		return;
	}
	ExportYamlWithDialog(this, "Export Instruments as YAML", "instruments.yaml",
		[&](YAML::Emitter& out) { EmitYmInstrumentsYaml(out, m_gd->GetMusicData()->GetYmInstruments()); });
}

void YmInstrumentEditorFrame::OnImportYaml()
{
	if (!m_gd)
	{
		return;
	}
	const bool ok = ImportYamlWithDialog(this, "Import Instruments from YAML", [&](const YAML::Node& root)
	{
		auto md = m_gd->GetMusicData();
		auto instruments = md->GetYmInstruments();
		ApplyYmInstrumentsFromYaml(root, instruments);
		md->SetYmInstruments(instruments);
	});
	if (ok)
	{
		RefreshList();
		if (m_list->GetItemCount() > 0)
		{
			m_list->Select(static_cast<long>(m_index));
		}
		LoadDetail();
	}
}

void YmInstrumentEditorFrame::OnListSelection(wxListEvent& evt)
{
	m_index = static_cast<std::size_t>(evt.GetIndex());
	LoadDetail();
}

bool YmInstrumentEditorFrame::Open()
{
	if (!m_gd)
	{
		return false;
	}
	RefreshList();
	if (m_list->GetFirstSelected() < 0 && m_list->GetItemCount() > 0)
	{
		m_list->Select(static_cast<long>(std::min(m_index, MusicData::YM_INSTRUMENT_COUNT - 1)));
	}
	LoadDetail();
	return true;
}

void YmInstrumentEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_index = 0;
	RefreshList();
	LoadDetail();
}

void YmInstrumentEditorFrame::ClearGameData()
{
	m_gd.reset();
	RefreshList();
	LoadDetail();
}

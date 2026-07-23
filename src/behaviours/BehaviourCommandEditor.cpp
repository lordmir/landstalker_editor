#include <behaviours/BehaviourCommandEditor.h>
#include <behaviours/BehaviourCommandDataViewRenderer.h>

#include <algorithm>
#include <functional>

#include <wx/settings.h>
#include <wx/spinctrl.h>

#include <landstalker/main/GameData.h>
#include <landstalker/misc/Labels.h>
#include <misc/DataViewEditorKeys.h>
#include <misc/LookupChoiceControl.h>

namespace
{

// GTK places the cell editor a fixed, unrequested distance to the left of the column's real
// on-screen start (see ScriptEntryEditors.cpp's CONTENT_LEFT_FUDGE_PX); the generic dataview
// (MSW) places it exactly over the cell, so any leading offset there just misaligns the editor's
// content relative to the rendered row underneath it.
#ifdef __WXGTK__
constexpr int CONTENT_LEFT_FUDGE_PX = 36;
#else
constexpr int CONTENT_LEFT_FUDGE_PX = 0;
#endif
// Sized so the first parameter field starts where the renderer draws it: the renderer's command
// bubble reserves LABEL_WIDTH (180) before the first parameter, and this control gets a 4px
// leading margin in the sizer.
constexpr int COMMAND_CHOICE_WIDTH = 176;
constexpr int LOOKUP_COMBO_WIDTH = 240;

wxArrayString BuildCommandChoices()
{
	using namespace Landstalker;
	wxArrayString choices;
	const int count = static_cast<int>(Behaviours::CommandType::NULL_COMMAND) + 1;
	choices.Alloc(count);
	// Command ids are contiguous (0 .. NULL_COMMAND), so the choice index doubles as the id.
	// Hex prefixes match how the commands are documented (the disassembly's $NN numbering).
	for (int i = 0; i < count; ++i)
	{
		choices.Add(wxString::Format("[%02X] %s", i, Behaviours::GetCommandById(i).aliases.front()));
	}
	return choices;
}

wxArrayString BuildLookupChoices(int base, int count, const std::function<std::wstring(int)>& name_of)
{
	wxArrayString choices;
	choices.Alloc(count);
	// Bracketed prefix carries the true id (base-offset for HIGH_CUTSCENE, whose ids start at
	// 256) - the control's selection is the *list index*, so collectors re-add `base`.
	for (int i = 0; i < count; ++i)
	{
		choices.Add(wxString::Format("[%d] %s", base + i, wxString(name_of(base + i))));
	}
	return choices;
}

} // namespace

BehaviourCommandEditorCtrl::BehaviourCommandEditorCtrl(wxWindow* parent, const wxRect& rect,
	const Landstalker::Behaviours::Command& command, int max_label_target)
	: wxPanel(parent, wxID_ANY, rect.GetPosition(), rect.GetSize()),
	  m_max_label_target(std::max(max_label_target, 1)),
	  m_cell_rect(rect)
{
	// Same opaque-background/click-consumption groundwork as ScriptEntryEditorCtrl - this floats
	// directly over the row's still-rendered graphics; see that class's constructor comments.
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Bind(wxEVT_PAINT, &BehaviourCommandEditorCtrl::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, [](wxMouseEvent&) {});
	Bind(wxEVT_LEFT_UP, [](wxMouseEvent&) {});
	Bind(wxEVT_LEFT_DCLICK, [](wxMouseEvent&) {});

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->AddSpacer(CONTENT_LEFT_FUDGE_PX);

	m_command_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, BuildCommandChoices());
	m_command_choice->SetMinSize(wxSize(COMMAND_CHOICE_WIDTH, -1));
	m_command_choice->SetSelection(static_cast<int>(command.command));
	m_command_choice->Bind(wxEVT_CHOICE, &BehaviourCommandEditorCtrl::OnCommandChange, this);
	sizer->Add(m_command_choice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);

	m_params_sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(m_params_sizer, 0, wxALIGN_CENTER_VERTICAL);

	// Trailing filler, as in ScriptEntryEditorCtrl::FinishContentSizer() - soaks up the leftover
	// width so the fields trail into blank space rather than stretching.
	sizer->AddStretchSpacer(1);
	sizer->Add(new wxStaticText(this, wxID_ANY, wxEmptyString), 1, wxEXPAND | wxLEFT, 8);
	SetSizer(sizer);

	BuildParamFields(command);
	FitToContent();
}

void BehaviourCommandEditorCtrl::FitToContent()
{
	wxSizer* sizer = GetSizer();
	if (!sizer)
	{
		return;
	}
#ifdef __WXGTK__
	// GTK force-stretches the editor window to its own idea of the cell area afterwards (see
	// ScriptDataViewRenderer::GetSize()'s comment), so shrinking to the natural minimum here is
	// safe and keeps the initial layout tidy - same as CreateScriptEntryEditor().
	sizer->SetSizeHints(this);
	sizer->Fit(this);
#else
	// The generic dataview (MSW) places the editor exactly over the cell and never resizes it -
	// keep the full cell width so the editor covers the whole row (and so parameter fields added
	// by a mid-edit command change have room to appear), only growing the height if the controls
	// need more than the row provides. Fit()-shrinking here (tried first) both uncovered the
	// stale row rendering to the right of the editor and clipped away any field the rebuild
	// added beyond the original content's width.
	const wxSize best = sizer->CalcMin();
	SetSize(m_cell_rect.GetWidth(), std::max(m_cell_rect.GetHeight(), best.GetHeight()));
#endif
	Layout();
}

void BehaviourCommandEditorCtrl::SetRenderer(wxDataViewRenderer* renderer)
{
	m_renderer = renderer;
}

void BehaviourCommandEditorCtrl::SetFocus()
{
	m_command_choice->SetFocus();
}

void BehaviourCommandEditorCtrl::BuildParamFields(const Landstalker::Behaviours::Command& command)
{
	using namespace Landstalker;
	m_fields.clear();
	// Clear(true) destroys the previous fields' windows along with removing their sizer items.
	m_params_sizer->Clear(true);

	for (const auto& param : command.params)
	{
		const auto& name = std::get<0>(param);
		const auto type = std::get<2>(param);
		const int int_value = std::holds_alternative<int>(std::get<1>(param)) ? std::get<int>(std::get<1>(param)) : 0;
		const double dbl_value = std::holds_alternative<double>(std::get<1>(param)) ? std::get<double>(std::get<1>(param)) : 0.0;

		auto* label = new wxStaticText(this, wxID_ANY, wxString(name) + ":");
		m_params_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

		ParamField field{ name, type, nullptr, 0, int_value };
		switch (type)
		{
		case Behaviours::ParamType::COORDINATE:
		case Behaviours::ParamType::LONG_COORDINATE:
		{
			auto* spin = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS | wxWANTS_CHARS);
			if (type == Behaviours::ParamType::COORDINATE)
			{
				// One byte, in 1/16ths (see Behaviours::Unpack()) - 4 decimal digits shows a
				// sixteenth (0.0625) exactly, so an untouched value never changes on commit.
				spin->SetRange(0.0, 255.0 / 16.0);
				spin->SetDigits(4);
			}
			else
			{
				// Two bytes, in 1/256ths - 8 digits shows 0.00390625 exactly (same reasoning).
				spin->SetRange(0.0, 65535.0 / 256.0);
				spin->SetDigits(8);
			}
			spin->SetIncrement(1.0 / 16.0);
			spin->SetValue(dbl_value);
			field.ctrl = spin;
			break;
		}
		case Behaviours::ParamType::LABEL:
		{
			auto* spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS | wxWANTS_CHARS, 1, m_max_label_target);
			spin->SetValue(std::clamp(int_value, 1, m_max_label_target));
			field.ctrl = spin;
			break;
		}
		case Behaviours::ParamType::FLAG:
		case Behaviours::ParamType::SOUND:
		case Behaviours::ParamType::LOW_CUTSCENE:
		case Behaviours::ParamType::HIGH_CUTSCENE:
		{
			wxArrayString choices;
			switch (type)
			{
			case Behaviours::ParamType::FLAG:
				// 0-2047, the full (byte << 3) | bit encodable range - see Behaviours::Unpack().
				choices = BuildLookupChoices(0, 2048, [](int i) { return ScriptData::GetFlagDisplayName(i); });
				break;
			case Behaviours::ParamType::SOUND:
				choices = BuildLookupChoices(0, 256, [](int i)
				{
					const auto label_name = Labels::Get(Labels::C_SOUNDS, i);
					return label_name ? *label_name : StrWPrintf(L"Sound%d", i);
				});
				break;
			default:
				// HIGH_CUTSCENE ids are 256-511 on the wire (one byte + 256) - the list still
				// runs 0-255, with `base` restoring the true id on collect.
				field.base = (type == Behaviours::ParamType::HIGH_CUTSCENE) ? 256 : 0;
				choices = BuildLookupChoices(field.base, 256, [](int i) { return ScriptData::GetCutsceneDisplayName(i); });
				break;
			}
			auto* combo = new LookupChoiceControl(this, wxID_ANY, wxEmptyString, choices,
				wxDefaultPosition, wxSize(LOOKUP_COMBO_WIDTH, -1));
			const int index = int_value - field.base;
			if (index >= 0 && index < static_cast<int>(choices.GetCount()))
			{
				combo->SetSelection(index);
			}
			else
			{
				// Out-of-range (corrupt) value: show it, and let GetSelection()'s wxNOT_FOUND
				// fall back to it unchanged on collect.
				combo->ChangeValue(wxString::Format("[%d] ???", int_value));
			}
			field.ctrl = combo;
			break;
		}
		case Behaviours::ParamType::INT8:
		case Behaviours::ParamType::UINT8:
		case Behaviours::ParamType::UINT16:
		default:
		{
			int min = 0;
			int max = 255;
			if (type == Behaviours::ParamType::INT8)
			{
				min = -128;
				max = 127;
			}
			else if (type == Behaviours::ParamType::UINT16)
			{
				max = 65535;
			}
			auto* spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxSP_ARROW_KEYS | wxWANTS_CHARS, min, max);
			spin->SetValue(std::clamp(int_value, min, max));
			field.ctrl = spin;
			break;
		}
		}
		m_params_sizer->Add(field.ctrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
		m_fields.push_back(field);

		// Fields created by a mid-edit command change need the same Escape/Enter/focus-loss
		// wiring CreateEditorCtrl()'s initial recursive bind gave the originals. m_renderer is
		// only set after construction, so this is skipped (correctly) for the initial build.
		if (m_renderer)
		{
			BindDataViewEditorEscapeEnter(label, m_renderer);
			BindDataViewEditorEscapeEnter(field.ctrl, m_renderer);
		}
	}
}

Landstalker::Behaviours::Command BehaviourCommandEditorCtrl::CollectCommand() const
{
	using namespace Landstalker;
	Behaviours::Command cmd;
	cmd.command = static_cast<Behaviours::CommandType>(m_command_choice->GetSelection());
	for (const auto& field : m_fields)
	{
		Behaviours::ParameterValue value;
		switch (field.type)
		{
		case Behaviours::ParamType::COORDINATE:
		case Behaviours::ParamType::LONG_COORDINATE:
			value = static_cast<wxSpinCtrlDouble*>(field.ctrl)->GetValue();
			break;
		case Behaviours::ParamType::FLAG:
		case Behaviours::ParamType::SOUND:
		case Behaviours::ParamType::LOW_CUTSCENE:
		case Behaviours::ParamType::HIGH_CUTSCENE:
		{
			auto* combo = static_cast<LookupChoiceControl*>(field.ctrl);
			combo->CommitPendingSelection();
			const int selection = combo->GetSelection();
			value = selection != wxNOT_FOUND ? field.base + selection : field.fallback;
			break;
		}
		default:
			value = static_cast<wxSpinCtrl*>(field.ctrl)->GetValue();
			break;
		}
		cmd.params.push_back({ field.name, value, field.type });
	}
	return cmd;
}

Landstalker::Behaviours::Command BehaviourCommandEditorCtrl::GetValue() const
{
	return CollectCommand();
}

void BehaviourCommandEditorCtrl::OnCommandChange(wxCommandEvent& evt)
{
	using namespace Landstalker;
	const auto new_type = static_cast<Behaviours::CommandType>(m_command_choice->GetSelection());
	// Carry over existing parameter values where the new command's parameter list matches by
	// position and type (e.g. switching MoveTimed -> MoveUpTimed keeps Ticks), defaulting the rest.
	const Behaviours::Command old_cmd = CollectCommand();
	Behaviours::Command new_cmd = BehaviourCommand::MakeDefault(new_type);
	for (std::size_t i = 0; i < new_cmd.params.size() && i < old_cmd.params.size(); ++i)
	{
		if (std::get<2>(new_cmd.params[i]) == std::get<2>(old_cmd.params[i]))
		{
			std::get<1>(new_cmd.params[i]) = std::get<1>(old_cmd.params[i]);
		}
	}
	BuildParamFields(new_cmd);
	FitToContent();
	evt.Skip();
}

void BehaviourCommandEditorCtrl::SetIndexCellReference(wxDataViewCtrl* dvc, const wxDataViewItem& item,
	wxDataViewColumn* col, const wxString& index_text)
{
	m_index_cell_dvc = dvc;
	m_index_cell_item = item;
	m_index_cell_col = col;
	m_index_text = index_text;
	Refresh();
}

void BehaviourCommandEditorCtrl::OnPaint(wxPaintEvent&)
{
	// Verbatim ScriptEntryEditorCtrl::OnPaint() - opaque fill only from the Index cell's right
	// edge onward, plus a self-drawn copy of the index number in the overlapped strip. See that
	// class's constructor/OnPaint comments for the full rationale. On MSW the editor never
	// overlaps the Index cell (margin computes to ~0), so this degenerates to a plain background
	// fill with the index text clipped away - correct there too.
	wxPaintDC dc(this);
	const wxSize size = GetSize();

	int margin = 0;
	int local_left = 0;
	if (m_index_cell_dvc)
	{
		const wxRect cell_rect = m_index_cell_dvc->GetItemRect(m_index_cell_item, m_index_cell_col);
		const wxPoint cell_right_screen = m_index_cell_dvc->ClientToScreen(wxPoint(cell_rect.x + cell_rect.width, cell_rect.y));
		const wxPoint cell_left_screen = m_index_cell_dvc->ClientToScreen(cell_rect.GetPosition());
		margin = std::max(0, cell_right_screen.x - GetScreenPosition().x);
		local_left = cell_left_screen.x - GetScreenPosition().x + 2;
	}

	if (margin < size.GetWidth())
	{
		dc.SetBrush(wxBrush(GetBackgroundColour()));
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(margin, 0, size.GetWidth() - margin, size.GetHeight());
	}

	if (m_index_cell_dvc && !m_index_text.empty())
	{
		const wxSize text_size = dc.GetTextExtent(m_index_text);
		dc.DrawText(m_index_text, local_left, std::max((size.GetHeight() - text_size.GetHeight()) / 2, 0));
	}
}

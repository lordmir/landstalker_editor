#include <behaviours/BehaviourCommandDataViewRenderer.h>
#include <behaviours/BehaviourCommandEditor.h>

#include <algorithm>

#include <wx/dc.h>
#include <wx/settings.h>
#include <wx/time.h>

#include <landstalker/behaviours/BehaviourYamlConverter.h>
#include <landstalker/main/GameData.h>
#include <landstalker/misc/Labels.h>
#include <misc/DataViewEditorKeys.h>

static const int LABEL_WIDTH = 180;
static const int PARAM_NAME_WIDTH = 70;
static const int PARAM_VALUE_WIDTH = 60;

namespace BehaviourCommand
{

std::string Serialise(const Landstalker::Behaviours::Command& cmd)
{
	return Landstalker::BehaviourYamlConverter::ToYaml({ cmd });
}

std::optional<Landstalker::Behaviours::Command> Deserialise(const std::string& yaml)
{
	try
	{
		const auto cmds = Landstalker::BehaviourYamlConverter::FromYaml(yaml);
		if (cmds.size() == 1)
		{
			return cmds.front();
		}
	}
	catch (const std::exception&)
	{
	}
	return std::nullopt;
}

Landstalker::Behaviours::Command MakeDefault(Landstalker::Behaviours::CommandType type)
{
	using namespace Landstalker;
	Behaviours::Command cmd;
	cmd.command = type;
	for (const auto& param : Behaviours::GetCommand(type).params)
	{
		Behaviours::ParameterValue value;
		switch (param.second)
		{
		case Behaviours::ParamType::COORDINATE:
		case Behaviours::ParamType::LONG_COORDINATE:
			value = 0.0;
			break;
		case Behaviours::ParamType::LABEL:
			value = 1;
			break;
		case Behaviours::ParamType::HIGH_CUTSCENE:
			value = 256;
			break;
		default:
			value = 0;
			break;
		}
		cmd.params.push_back({ param.first, value, param.second });
	}
	return cmd;
}

Style GetStyle(Landstalker::Behaviours::CommandType type)
{
	using namespace Landstalker;
	// Categorised by the command's primary alias rather than enumerating all ~105 enum values -
	// the aliases follow consistent naming (Turn*/SetDir*/Move*/...), and this only ever picks a
	// bubble colour, so an imperfect match for a new command is cosmetic.
	const std::string& name = Behaviours::GetCommand(type).aliases.front();
	auto starts_with = [&name](const char* prefix)
	{
		return name.rfind(prefix, 0) == 0;
	};
	auto contains = [&name](const char* text)
	{
		return name.find(text) != std::string::npos;
	};
	if (starts_with("Turn") || starts_with("SetDir") || name == "RotatePlayer" || name == "UpdateSpriteFacing")
	{
		return { *wxCYAN, *wxBLACK };
	}
	if (contains("Flag") || contains("Switch") || contains("TileSwap"))
	{
		return { wxColour("MEDIUM SPRING GREEN"), *wxBLACK };
	}
	if (contains("Cutscene") || name == "PrintText" || name == "PlaySound")
	{
		return { wxColour("DARK ORCHID"), *wxWHITE };
	}
	if (contains("Visible") || contains("Hide") || contains("Show") || contains("Flash") ||
		contains("Despawn") || name == "SpecialAnimation")
	{
		return { wxColour("GOLD"), *wxBLACK };
	}
	if (starts_with("Move") || starts_with("Follow") || starts_with("Projectile") ||
		starts_with("Flee") || name == "Jump")
	{
		return { *wxBLUE, *wxWHITE };
	}
	if (starts_with("SetSpeed") || contains("Gravity") || contains("Rotation") || contains("Hostile") ||
		contains("Backwards") || name == "SetEntitySpeed" || name == "SetTargetPosition" ||
		name == "ResetToInitParams")
	{
		return { wxColour("ORANGE"), *wxBLACK };
	}
	if (starts_with("Pause") || starts_with("Wait") || starts_with("Goto") || starts_with("Repeat") ||
		name == "Freeze" || name == "Null")
	{
		return { wxColour("MAROON"), *wxWHITE };
	}
	return { wxColour("DARK SLATE BLUE"), *wxWHITE };
}

} // namespace BehaviourCommand

BehaviourCommandDataViewRenderer::BehaviourCommandDataViewRenderer(wxDataViewCellMode mode, std::function<unsigned int()> row_count)
	: wxDataViewCustomRenderer("string", mode, wxALIGN_LEFT),
	  m_row_count(std::move(row_count))
{
}

bool BehaviourCommandDataViewRenderer::Render(wxRect rect, wxDC* dc, int state)
{
	rect.y += 2;
	wxFont bold = dc->GetFont().Bold();
	if (!m_value)
	{
		InsertRenderBubble(rect, dc, state, "INVALID", *wxRED, LABEL_WIDTH, &bold, wxWHITE);
		return true;
	}
	const auto& def = Landstalker::Behaviours::GetCommand(m_value->command);
	const auto style = BehaviourCommand::GetStyle(m_value->command);
	InsertRenderBubble(rect, dc, state, def.aliases.front(), style.bg, LABEL_WIDTH, &bold, &style.fg);
	for (const auto& param : m_value->params)
	{
		RenderParameter(rect, dc, state, param);
	}
	return true;
}

void BehaviourCommandDataViewRenderer::RenderParameter(wxRect& rect, wxDC* dc, int state, const Landstalker::Behaviours::Parameter& param)
{
	using namespace Landstalker;
	wxFont bold = dc->GetFont().Bold();
	wxFont italic_tt = dc->GetFont().Italic();
	italic_tt.SetFamily(wxFONTFAMILY_TELETYPE);

	const auto& name = std::get<0>(param);
	const auto& value = std::get<1>(param);
	const auto type = std::get<2>(param);
	const int int_value = std::holds_alternative<int>(value) ? std::get<int>(value) : 0;

	InsertRenderLabel(rect, dc, state, wxString(name) + ":", PARAM_NAME_WIDTH, &bold);
	switch (type)
	{
	case Behaviours::ParamType::COORDINATE:
	case Behaviours::ParamType::LONG_COORDINATE:
		InsertRenderLabel(rect, dc, state,
			StrPrintf("%g", std::holds_alternative<double>(value) ? std::get<double>(value) : 0.0),
			PARAM_VALUE_WIDTH);
		break;
	case Behaviours::ParamType::LABEL:
	{
		// A dangling target would trip Behaviours::Pack()'s label lookup at ROM build time -
		// flag it in red the moment it becomes dangling (e.g. after deleting the target row).
		const bool valid = int_value >= 1 && int_value <= static_cast<int>(m_row_count ? m_row_count() : 0);
		InsertRenderLabel(rect, dc, state, StrPrintf("%d", int_value), PARAM_VALUE_WIDTH,
			nullptr, valid ? nullptr : wxRED);
		break;
	}
	case Behaviours::ParamType::FLAG:
		InsertRenderLabel(rect, dc, state, StrPrintf("%04d", int_value), PARAM_VALUE_WIDTH);
		InsertRenderLabel(rect, dc, state, ScriptData::GetFlagDisplayName(int_value), 100, &italic_tt);
		break;
	case Behaviours::ParamType::SOUND:
		InsertRenderLabel(rect, dc, state, StrPrintf("%d", int_value), PARAM_VALUE_WIDTH);
		if (Labels::Exists(Labels::C_SOUNDS, int_value))
		{
			InsertRenderLabel(rect, dc, state, *Labels::Get(Labels::C_SOUNDS, int_value), 100, &italic_tt);
		}
		break;
	case Behaviours::ParamType::LOW_CUTSCENE:
	case Behaviours::ParamType::HIGH_CUTSCENE:
		InsertRenderLabel(rect, dc, state, StrPrintf("%d", int_value), PARAM_VALUE_WIDTH);
		InsertRenderLabel(rect, dc, state, ScriptData::GetCutsceneDisplayName(int_value), 100, &italic_tt);
		break;
	default:
		InsertRenderLabel(rect, dc, state, StrPrintf("%d", int_value), PARAM_VALUE_WIDTH);
		break;
	}
}

void BehaviourCommandDataViewRenderer::InsertRenderLabel(wxRect& rect, wxDC* dc, int state, const wxString& text, int min_width, const wxFont* font, const wxColour* text_colour)
{
	wxFont orig_font;
	if (font)
	{
		orig_font = dc->GetFont();
		dc->SetFont(*font);
	}
	auto extent = dc->GetTextExtent(text);
	int y_offset = std::max((rect.GetHeight() - extent.GetHeight()) / 2, 0);
	rect.y += y_offset;

	// See ScriptDataViewRenderer::InsertRenderBubble()'s comment: RenderText() on GTK ignores
	// `dc`, so forcing a colour needs the attribute override, not just dc->SetTextForeground().
	const wxDataViewItemAttr orig_attr = GetAttr();
	wxColour orig_dc_colour;
	if (text_colour)
	{
		wxDataViewItemAttr attr = orig_attr;
		attr.SetColour(*text_colour);
		SetAttr(attr);
		orig_dc_colour = dc->GetTextForeground();
		dc->SetTextForeground(*text_colour);
	}
	RenderText(text, 2, rect, dc, state);
	if (text_colour)
	{
		SetAttr(orig_attr);
		dc->SetTextForeground(orig_dc_colour);
	}

	rect.y -= y_offset;
	int new_width = std::max(extent.GetWidth() + 2, min_width);
	rect.x += new_width;
	rect.width -= new_width;
	if (font)
	{
		dc->SetFont(orig_font);
	}
}

void BehaviourCommandDataViewRenderer::InsertRenderBubble(wxRect& rect, wxDC* dc, int state, const wxString& text, const wxColour& colour, int min_width, const wxFont* font, const wxColour* text_colour)
{
	wxFont orig_font = dc->GetFont();
	if (font)
	{
		dc->SetFont(*font);
	}
	auto extent = dc->GetTextExtent(text);
	wxRect draw_rect = rect;

	draw_rect.width = extent.GetWidth() + 4;
	draw_rect.x += 2;

	// Skip the bubble fill while selected so the native selection highlight stays legible -
	// see ScriptDataViewRenderer::InsertRenderBubble()'s comment for the GTK rationale.
	const bool selected = (state & wxDATAVIEW_CELL_SELECTED) != 0;
	if (!selected)
	{
		dc->SetBrush(wxBrush(colour));
		dc->SetPen(*wxTRANSPARENT_PEN);
		dc->DrawRoundedRectangle(draw_rect, 3);
	}

	const wxDataViewItemAttr orig_attr = GetAttr();
	wxColour orig_dc_colour;
	if (text_colour && !selected)
	{
		wxDataViewItemAttr attr = orig_attr;
		attr.SetColour(*text_colour);
		SetAttr(attr);
		orig_dc_colour = dc->GetTextForeground();
		dc->SetTextForeground(*text_colour);
	}
	RenderText(text, 2, draw_rect, dc, state);
	if (text_colour && !selected)
	{
		SetAttr(orig_attr);
		dc->SetTextForeground(orig_dc_colour);
	}

	int new_width = std::max(draw_rect.GetWidth() + 2, min_width);
	rect.x += new_width;
	rect.width -= new_width;
	if (font)
	{
		dc->SetFont(orig_font);
	}
}

bool BehaviourCommandDataViewRenderer::ActivateCell(const wxRect& /*cell*/, wxDataViewModel* /*model*/, const wxDataViewItem& /*item*/,
	unsigned int /*col*/, const wxMouseEvent* /*mouseEvent*/)
{
	return false;
}

wxSize BehaviourCommandDataViewRenderer::GetSize() const
{
	// Same cached-live-width scheme as ScriptDataViewRenderer::GetSize() - see that function's
	// comment for why neither the live column width nor a fixed constant works on its own.
	static int s_lastKnownWidth = 800; // BehaviourScriptDataViewModel::InitControl()'s SetMinWidth() floor
	const int live_width = GetOwner() ? GetOwner()->GetWidth() : 0;
	if (live_width > 200)
	{
		s_lastKnownWidth = live_width;
	}
	return { s_lastKnownWidth, GetTextExtent("Xy").GetHeight() };
}

bool BehaviourCommandDataViewRenderer::SetValue(const wxVariant& value)
{
	// SetValue runs on every repaint of every visible cell (hover and selection changes
	// included), so skip the YAML re-parse when the value hasn't changed since last time.
	const wxString raw = value.GetString();
	if (raw != m_raw)
	{
		m_raw = raw;
		m_value = BehaviourCommand::Deserialise(std::string(raw.utf8_str()));
	}
	return true;
}

bool BehaviourCommandDataViewRenderer::GetValue(wxVariant& value) const
{
	if (m_value)
	{
		// FromUTF8 - see BehaviourScriptDataViewModel::GetValueByRow()'s comment.
		value = wxString::FromUTF8(BehaviourCommand::Serialise(*m_value));
		return true;
	}
	return false;
}

bool BehaviourCommandDataViewRenderer::HasEditorCtrl() const
{
	return true;
}

wxWindow* BehaviourCommandDataViewRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& value)
{
	auto cmd = BehaviourCommand::Deserialise(std::string(value.GetString().utf8_str()));
	if (!cmd)
	{
		// An unparseable row still needs a functional editor - hand it a default command so the
		// user can rebuild the row rather than being locked out of it.
		cmd = BehaviourCommand::MakeDefault(Landstalker::Behaviours::CommandType::PAUSE);
	}
	auto* editor = new BehaviourCommandEditorCtrl(parent, labelRect, *cmd,
		static_cast<int>(m_row_count ? m_row_count() : 0));
	editor->SetRenderer(this);
	BindDataViewEditorEscapeEnter(editor, this);

	// Same Index-cell handling as ScriptDataViewRenderer::CreateEditorCtrl() (self-drawn index
	// copy in the strip the editor overlaps + a deferred explicit repaint) - see the comments
	// there and in ScriptEntryEditorCtrl.
	wxDataViewColumn* command_column = GetOwner();
	wxDataViewCtrl* dvc = command_column->GetOwner();
	wxDataViewColumn* index_column = dvc->GetColumn(0);
	// The virtual list model's item ID is row + 1, which is exactly the 1-based instruction
	// number the Index column displays.
	editor->SetIndexCellReference(dvc, m_editingItem, index_column,
		wxString::Format("%ld", static_cast<long>(reinterpret_cast<std::intptr_t>(m_editingItem.GetID()))));
	if (index_column)
	{
		wxDataViewItem editing_item = m_editingItem;
		editor->CallAfter([dvc, editing_item, index_column]()
		{
			dvc->RefreshRect(dvc->GetItemRect(editing_item, index_column));
		});
	}
	return editor;
}

bool BehaviourCommandDataViewRenderer::StartEditing(const wxDataViewItem& item, wxRect labelRect)
{
	// See ScriptDataViewRenderer::StartEditing()'s comment - debounce a rapid finish/restart
	// cascade that otherwise races editor teardown and segfaults on GTK.
	if (m_lastEditEndTimeMs > 0 && (wxGetLocalTimeMillis() - m_lastEditEndTimeMs) < 250)
	{
		return false;
	}
	m_editingItem = item;
	return wxDataViewCustomRenderer::StartEditing(item, labelRect);
}

bool BehaviourCommandDataViewRenderer::FinishEditing()
{
	m_lastEditEndTimeMs = wxGetLocalTimeMillis();
	return wxDataViewCustomRenderer::FinishEditing();
}

void BehaviourCommandDataViewRenderer::CancelEditing()
{
	m_lastEditEndTimeMs = wxGetLocalTimeMillis();
	wxDataViewCustomRenderer::CancelEditing();
}

bool BehaviourCommandDataViewRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
	auto* editor = dynamic_cast<BehaviourCommandEditorCtrl*>(ctrl);
	if (!editor)
	{
		return false;
	}
	// FromUTF8 - see BehaviourScriptDataViewModel::GetValueByRow()'s comment.
	value = wxString::FromUTF8(BehaviourCommand::Serialise(editor->GetValue()));
	return true;
}

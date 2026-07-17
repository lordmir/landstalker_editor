#include <script/ScriptTreeActionEditors.h>

#include <landstalker/misc/Utils.h>
#include <misc/DataViewEditorKeys.h>
#include <misc/LookupDataViewRenderer.h>

using namespace Landstalker;

namespace
{
    // The Script Action editor's commit payload needs its own mode tag - "function reference"
    // vs "new function" vs "raw script ID" can't be told apart from the node type alone.
    // ScriptTreeDataViewModel::SetValue() parses these back; every other editor commits a plain value whose
    // meaning follows from the node's type.
    const std::string FUNC_PREFIX = "Function: ";
    const std::string ID_PREFIX = "Script ID: ";
    const std::string NEW_FUNC_PREFIX = "New Function: ";
}

ScriptActionEditorCtrl::ScriptActionEditorCtrl(wxWindow* parent, const wxRect& rect, bool is_function, uint16_t script_id, const wxString& function_name, ScriptTreeDataViewModel* model)
    : wxPanel(parent, wxID_ANY, rect.GetPosition(), rect.GetSize()),
      m_model(model)
{
    wxArrayString choices;
    if (m_model)
    {
        for (const auto& name : m_model->GetFunctionNameChoices())
        {
            choices.Add(name);
        }
    }

    m_mode = new wxChoice(this, wxID_ANY);
    m_mode->Append("Script ID");
    m_mode->Append("Function Name");
    m_mode->Append("New Function");
    m_mode->SetSelection(is_function ? 1 : 0);

    const wxSize id_text_size(GetTextExtent("0000").GetWidth() + 16, -1);
    m_id_text = new wxTextCtrl(this, wxID_ANY, StrPrintf("%04X", script_id), wxDefaultPosition, id_text_size, wxTE_PROCESS_ENTER);
    m_id_text->SetMaxLength(4);
    {
        wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);
        wxArrayString hex_chars;
        const std::string digits = "0123456789ABCDEFabcdef";
        for (char c : digits)
        {
            hex_chars.Add(wxString(c));
        }
        validator.SetIncludes(hex_chars);
        m_id_text->SetValidator(validator);
    }

    m_id_spin = new wxSpinButton(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_VERTICAL | wxSP_ARROW_KEYS);
    m_id_spin->SetRange(0, 0xFFFF);
    m_id_spin->SetValue(script_id);

    m_function_combo = LookupEditor::Create(this, wxRect(wxPoint(0, 0), rect.GetSize()), function_name, choices, nullptr);

    m_new_function_text = new wxTextCtrl(this, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_new_function_text->SetMaxLength(50);
    {
        // Restrict to the allowed character set as they type; the remaining rules (can't start
        // with a digit, can't already exist) are only checkable in context, so those are
        // enforced via the red/black validity colouring instead.
        wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);
        wxArrayString allowed_chars;
        const std::string chars = "_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        for (char c : chars)
        {
            allowed_chars.Add(wxString(c));
        }
        validator.SetIncludes(allowed_chars);
        m_new_function_text->SetValidator(validator);
    }

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_mode, 0, wxEXPAND);
    sizer->Add(m_id_text, 0, wxEXPAND);
    sizer->Add(m_id_spin, 0, wxEXPAND);
    sizer->Add(m_function_combo, 1, wxEXPAND);
    sizer->Add(m_new_function_text, 1, wxEXPAND);
    SetSizer(sizer);

    m_mode->Bind(wxEVT_CHOICE, &ScriptActionEditorCtrl::OnModeChanged, this);
    m_id_text->Bind(wxEVT_TEXT, &ScriptActionEditorCtrl::OnIdTextChanged, this);
    m_id_spin->Bind(wxEVT_SPIN_UP, &ScriptActionEditorCtrl::OnSpinUp, this);
    m_id_spin->Bind(wxEVT_SPIN_DOWN, &ScriptActionEditorCtrl::OnSpinDown, this);
    m_function_combo->Bind(wxEVT_TEXT, &ScriptActionEditorCtrl::OnFunctionTextChanged, this);
    m_new_function_text->Bind(wxEVT_TEXT, &ScriptActionEditorCtrl::OnNewFunctionTextChanged, this);
    m_preview_hide_timer.Bind(wxEVT_TIMER, &ScriptActionEditorCtrl::OnPreviewHideTimer, this);

    UpdateLayout();
    UpdateIdColour();
    UpdateFunctionColour();
    UpdateNewFunctionColour();
}

uint16_t ScriptActionEditorCtrl::ParseId() const
{
    try
    {
        return static_cast<uint16_t>(std::stoul(m_id_text->GetValue().ToStdString(), nullptr, 16));
    }
    catch (...)
    {
        return 0;
    }
}

void ScriptActionEditorCtrl::OnModeChanged(wxCommandEvent&)
{
    UpdateLayout();
}

void ScriptActionEditorCtrl::OnIdTextChanged(wxCommandEvent&)
{
    UpdateIdColour();
    ShowPreviewPopup();
}

void ScriptActionEditorCtrl::OnFunctionTextChanged(wxCommandEvent&)
{
    UpdateFunctionColour();
    ShowPreviewPopup();
}

void ScriptActionEditorCtrl::OnNewFunctionTextChanged(wxCommandEvent&)
{
    UpdateNewFunctionColour();
    ShowPreviewPopup();
}

void ScriptActionEditorCtrl::OnSpinUp(wxSpinEvent& event)
{
    uint16_t value = ParseId();
    if (value != 0xFFFF)
    {
        ++value;
    }
    m_id_text->ChangeValue(StrPrintf("%04X", value));
    UpdateIdColour();
    ShowPreviewPopup();
    event.Veto();
}

void ScriptActionEditorCtrl::OnSpinDown(wxSpinEvent& event)
{
    uint16_t value = ParseId();
    if (value != 0)
    {
        --value;
    }
    m_id_text->ChangeValue(StrPrintf("%04X", value));
    UpdateIdColour();
    ShowPreviewPopup();
    event.Veto();
}

void ScriptActionEditorCtrl::OnPreviewHideTimer(wxTimerEvent&)
{
    HidePreviewPopup();
}

void ScriptActionEditorCtrl::UpdateLayout()
{
    const int mode = m_mode->GetSelection();
    m_id_text->Show(mode == 0);
    m_id_spin->Show(mode == 0);
    m_function_combo->Show(mode == 1);
    m_new_function_text->Show(mode == 2);
    Layout();
    ShowPreviewPopup();
}

void ScriptActionEditorCtrl::ShowPreviewPopup()
{
    if (!m_preview_popup)
    {
        m_preview_popup = new wxPopupWindow(this, wxBORDER_SIMPLE);
        wxBoxSizer* popup_sizer = new wxBoxSizer(wxVERTICAL);
        m_preview_text = new wxStaticText(m_preview_popup, wxID_ANY, wxString());
        popup_sizer->Add(m_preview_text, 0, wxALL, 4);
        m_preview_popup->SetSizer(popup_sizer);
    }

    UpdatePreviewPopup();

    wxWindow* anchor = IsNewFunctionMode() ? static_cast<wxWindow*>(m_new_function_text)
        : IsFunctionMode() ? static_cast<wxWindow*>(m_function_combo)
        : static_cast<wxWindow*>(m_id_text);
    const wxPoint pos = anchor->ClientToScreen(wxPoint(0, anchor->GetSize().GetHeight()));
    m_preview_popup->Position(pos, wxSize(0, 0));
    m_preview_popup->Show();

    m_preview_hide_timer.Start(1500, wxTIMER_ONE_SHOT);
}

void ScriptActionEditorCtrl::HidePreviewPopup()
{
    m_preview_hide_timer.Stop();
    if (m_preview_popup)
    {
        m_preview_popup->Hide();
    }
}

void ScriptActionEditorCtrl::UpdatePreviewPopup()
{
    if (!m_preview_popup || !m_model)
    {
        return;
    }

    wxString preview;
    wxString empty_message;
    if (IsNewFunctionMode())
    {
        const std::string name = GetNewFunctionName().ToStdString();
        if (m_model->IsValidNewFunctionName(name))
        {
            preview = "Will create a new, empty function named \"" + wxString(name) + "\"";
        }
        empty_message = "(invalid or already-used function name)";
    }
    else if (IsFunctionMode())
    {
        preview = m_model->GetFunctionPreview(GetFunctionName().ToStdString());
        empty_message = "(unknown function)";
    }
    else
    {
        preview = m_model->GetScriptIdPreview(ParseId());
        empty_message = "(out of range)";
    }
    m_preview_text->SetLabel(preview.empty() ? empty_message : preview);
    m_preview_popup->GetSizer()->Fit(m_preview_popup);
}

void ScriptActionEditorCtrl::UpdateIdColour()
{
    const bool valid = m_model && m_model->IsValidScriptId(ParseId());
    m_id_text->SetForegroundColour(valid ? wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
        : wxSystemSettings::SelectLightDark(wxColour(200, 0, 0), wxColour(255, 90, 90)));
    m_id_text->Refresh();
}

void ScriptActionEditorCtrl::UpdateFunctionColour()
{
    const bool valid = m_model && m_model->HasFunction(Trim(GetFunctionName().ToStdString()));
    LookupEditor::SetTextColour(m_function_combo, valid ? wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
        : wxSystemSettings::SelectLightDark(wxColour(200, 0, 0), wxColour(255, 90, 90)));
}

void ScriptActionEditorCtrl::UpdateNewFunctionColour()
{
    const bool valid = m_model && m_model->IsValidNewFunctionName(Trim(m_new_function_text->GetValue().ToStdString()));
    m_new_function_text->SetForegroundColour(valid ? wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
        : wxSystemSettings::SelectLightDark(wxColour(200, 0, 0), wxColour(255, 90, 90)));
    m_new_function_text->Refresh();
}

bool ScriptActionEditorCtrl::IsFunctionMode() const
{
    return m_mode->GetSelection() == 1;
}

bool ScriptActionEditorCtrl::IsNewFunctionMode() const
{
    return m_mode->GetSelection() == 2;
}

wxString ScriptActionEditorCtrl::GetFunctionName() const
{
    return wxString(Trim(LookupEditor::GetValueText(m_function_combo).ToStdString()));
}

void ScriptActionEditorCtrl::CommitFunctionSelection()
{
    LookupEditor::CommitPendingSelection(m_function_combo);
}

wxString ScriptActionEditorCtrl::GetNewFunctionName() const
{
    return wxString(Trim(m_new_function_text->GetValue().ToStdString()));
}

uint16_t ScriptActionEditorCtrl::GetScriptId() const
{
    return ParseId();
}

ScriptActionRenderer::ScriptActionRenderer()
    : wxDataViewCustomRenderer(wxDataViewTextRenderer::GetDefaultType(), wxDATAVIEW_CELL_EDITABLE)
{
}

ScriptTreeDataViewModel* ScriptActionRenderer::GetTreeModel() const
{
    wxDataViewCtrl* view = GetView();
    return view ? static_cast<ScriptTreeDataViewModel*>(view->GetModel()) : nullptr;
}

ScriptTreeNode* ScriptActionRenderer::GetEditedNode() const
{
    // m_item is set by wxDataViewRendererBase::StartEditing() before CreateEditorCtrl() is
    // called and stays valid until FinishEditing() has retrieved the editor's value.
    return reinterpret_cast<ScriptTreeNode*>(m_item.GetID());
}

bool ScriptActionRenderer::SetValue(const wxVariant& value)
{
    m_value = value.GetString();
    return true;
}

bool ScriptActionRenderer::GetValue(wxVariant& value) const
{
    value = m_value;
    return true;
}

bool ScriptActionRenderer::Render(wxRect rect, wxDC* dc, int state)
{
    if (!GetView())
    {
        return false;
    }

    const bool selected = (state & wxDATAVIEW_CELL_SELECTED) != 0;

    // Colour/font come entirely from ScriptTreeDataViewModel::GetAttr() - the model classifies by the row's
    // actual ScriptTreeNodeType and typed payload (including the red invalid-value override), which the
    // renderer can't see (it only ever receives the display text). Selection takes priority,
    // for legibility against the highlight.
    const wxDataViewItemAttr& attr = GetAttr();
    wxFont font = GetView()->GetFont();
    if (attr.GetBold())
    {
        font = font.Bold();
    }
    if (attr.GetItalic())
    {
        font = font.Italic();
    }
    wxColour colour = attr.HasColour() ? attr.GetColour() : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    if (selected)
    {
        colour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
    }

    dc->SetTextForeground(colour);
    dc->SetFont(font);
    const wxSize extent = dc->GetTextExtent(m_value);
    dc->DrawText(m_value, rect.x + 2, rect.y + (rect.height - extent.GetHeight()) / 2);
    return true;
}

wxSize ScriptActionRenderer::GetSize() const
{
    if (!GetView())
    {
        return wxSize(80, 20);
    }
    wxClientDC dc(GetView());
    dc.SetFont(GetView()->GetFont());
    const wxSize extent = dc.GetTextExtent(m_value);
    return wxSize(extent.GetWidth() + 10, extent.GetHeight() + 6);
}

bool ScriptActionRenderer::HasEditorCtrl() const
{
    return true;
}

wxWindow* ScriptActionRenderer::CreateEditorCtrl(wxWindow* parent, wxRect labelRect, const wxVariant& /*value*/)
{
    // The editor is chosen by the row's ScriptTreeNodeType and pre-filled from its typed payload - the
    // display text (in `value`) is presentation only and is never parsed.
    ScriptTreeNode* node = GetEditedNode();
    if (!node)
    {
        return nullptr;
    }

    switch (node->type)
    {
    case ScriptTreeNodeType::FUNCTION:
    {
        // A bare function header isn't a script action - editing it renames the function, via
        // a plain text field holding just the name.
        wxTextCtrl* rename = new wxTextCtrl(parent, wxID_ANY, wxString(node->text_value),
            labelRect.GetPosition(), labelRect.GetSize(), wxTE_PROCESS_ENTER);
        rename->SetMaxLength(50);
        {
            wxTextValidator validator(wxFILTER_INCLUDE_CHAR_LIST);
            wxArrayString allowed_chars;
            const std::string chars = "_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
            for (char c : chars)
            {
                allowed_chars.Add(wxString(c));
            }
            validator.SetIncludes(allowed_chars);
            rename->SetValidator(validator);
        }
        rename->SelectAll();
        BindDataViewEditorEscapeEnter(rename, this);
        return rename;
    }

    case ScriptTreeNodeType::BRANCH:
    {
        // A Branch statement's label in a text field, plus a checkbox for the wide-branch
        // flag. The label is coloured red while it doesn't name an existing function,
        // mirroring how the row itself is drawn.
        wxPanel* editor = new wxPanel(parent, wxID_ANY, labelRect.GetPosition(), labelRect.GetSize());
        editor->SetName("BranchEditor");
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        wxTextCtrl* label = new wxTextCtrl(editor, wxID_ANY, wxString(node->text_value),
            wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
        label->SetName("BranchLabel");
        ScriptTreeDataViewModel* model = GetTreeModel();
        auto update_colour = [model, label]()
        {
            const bool valid = model && model->HasFunction(Trim(label->GetValue().ToStdString()));
            label->SetForegroundColour(valid ? wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
                : wxSystemSettings::SelectLightDark(wxColour(200, 0, 0), wxColour(255, 90, 90)));
            label->Refresh();
        };
        label->Bind(wxEVT_TEXT, [update_colour](wxCommandEvent&) { update_colour(); });
        update_colour();
        sizer->Add(label, 1, wxEXPAND | wxRIGHT, 8);
        wxCheckBox* wide = new wxCheckBox(editor, wxID_ANY, "Wide");
        wide->SetName("BranchWide");
        wide->SetValue(node->bool_value);
        sizer->Add(wide, 0, wxALIGN_CENTER_VERTICAL);
        editor->SetSizer(sizer);
        editor->Layout();
        label->SelectAll();
        BindDataViewEditorEscapeEnter(editor, this);
        return editor;
    }

    case ScriptTreeNodeType::PROG_DEP_ACTION:
    {
        // A Progress Entry gets a pair of spin controls, one per value. Needs an explicit opaque
        // background - a bare wxPanel here left gaps (around the static labels) showing the
        // underlying row's own text through the editor.
        wxPanel* editor = new wxPanel(parent, wxID_ANY, labelRect.GetPosition(), labelRect.GetSize());
        editor->SetName("QuestProgressEditor");
        editor->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(new wxStaticText(editor, wxID_ANY, "Quest"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        wxSpinCtrl* quest_spin = new wxSpinCtrl(editor, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, std::min<int>(node->numeric_value, 255));
        quest_spin->SetName("QuestSpin");
        sizer->Add(quest_spin, 1, wxEXPAND | wxRIGHT, 8);
        sizer->Add(new wxStaticText(editor, wxID_ANY, "Progress"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        wxSpinCtrl* progress_spin = new wxSpinCtrl(editor, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, std::min<int>(node->numeric_value2, 255));
        progress_spin->SetName("ProgressSpin");
        sizer->Add(progress_spin, 1, wxEXPAND);
        editor->SetSizer(sizer);
        editor->Layout();
        BindDataViewEditorEscapeEnter(editor, this);
        return editor;
    }

    // Numeric-parameter rows get a spin control, in hex if that's how the label shows the value.
    case ScriptTreeNodeType::PLAY_SOUND:
    case ScriptTreeNodeType::SLEEP:
    case ScriptTreeNodeType::CUSTOM_SHOP_ACTION:
    case ScriptTreeNodeType::SET_FLAG:
    case ScriptTreeNodeType::CHECK_FLAG:
    {
        wxSpinCtrl* spin = new wxSpinCtrl(parent, wxID_ANY, wxEmptyString, labelRect.GetPosition(),
            labelRect.GetSize(), wxSP_ARROW_KEYS, 0, 0xFFFF, node->numeric_value);
        if (node->type == ScriptTreeNodeType::SET_FLAG || node->type == ScriptTreeNodeType::CHECK_FLAG)
        {
            spin->SetBase(16);
        }
        BindDataViewEditorEscapeEnter(spin, this);
        return spin;
    }

    default:
        break;
    }

    // Everything else that's editable is an action row - anything built with an action_label
    // (top-level slots, Script Actions, a question's OnYes/OnNo, table entries, ...).
    if (node->action_label.empty())
    {
        return nullptr;
    }
    const std::string function_name = node->embedded_function_name.empty()
        ? node->text_value : node->embedded_function_name;
    wxWindow* editor = new ScriptActionEditorCtrl(parent, labelRect, !function_name.empty(),
        node->numeric_value, wxString(function_name), GetTreeModel());
    BindDataViewEditorEscapeEnter(editor, this);
    return editor;
}

bool ScriptActionRenderer::GetValueFromEditorCtrl(wxWindow* editorCtrl, wxVariant& value)
{
    // Every editor commits a plain payload; ScriptTreeDataViewModel::SetValue() interprets it from the
    // node's own type (rename for FUNCTION, label for BRANCH, number(s) for the spin rows).

    // The Progress Entry editor is a panel holding two named spin controls (see CreateEditorCtrl).
    if (editorCtrl->GetName() == "QuestProgressEditor")
    {
        wxSpinCtrl* quest = wxDynamicCast(editorCtrl->FindWindow(wxString("QuestSpin")), wxSpinCtrl);
        wxSpinCtrl* progress = wxDynamicCast(editorCtrl->FindWindow(wxString("ProgressSpin")), wxSpinCtrl);
        if (!quest || !progress)
        {
            return false;
        }
        value = wxString(StrPrintf("%d %d", quest->GetValue(), progress->GetValue()));
        return true;
    }

    // The Branch editor is a panel holding the label field and the "Wide" checkbox (see
    // CreateEditorCtrl) - committed as "<label>[ W]", the convention ApplyBranchLabel() reads.
    if (editorCtrl->GetName() == "BranchEditor")
    {
        wxTextCtrl* label = wxDynamicCast(editorCtrl->FindWindow(wxString("BranchLabel")), wxTextCtrl);
        wxCheckBox* wide = wxDynamicCast(editorCtrl->FindWindow(wxString("BranchWide")), wxCheckBox);
        if (!label || !wide)
        {
            return false;
        }
        value = wxString(Trim(label->GetValue().ToStdString()) + (wide->GetValue() ? " W" : ""));
        return true;
    }

    // A numeric-parameter row's editor is a wxSpinCtrl (see CreateEditorCtrl).
    if (wxSpinCtrl* spin = wxDynamicCast(editorCtrl, wxSpinCtrl))
    {
        value = wxString(StrPrintf("%d", spin->GetValue()));
        return true;
    }

    // The remaining plain wxTextCtrl editor is the bare function header rename.
    if (wxTextCtrl* text_editor = wxDynamicCast(editorCtrl, wxTextCtrl))
    {
        value = wxString(Trim(text_editor->GetValue().ToStdString()));
        return true;
    }

    // The Script Action editor - the one payload that carries its own mode tag, since the
    // three commit modes can't be told apart from the node type.
    auto* editor = static_cast<ScriptActionEditorCtrl*>(editorCtrl);
    if (editor->IsNewFunctionMode())
    {
        value = wxString(NEW_FUNC_PREFIX + editor->GetNewFunctionName().ToStdString());
    }
    else if (editor->IsFunctionMode())
    {
        editor->CommitFunctionSelection();
        value = wxString(FUNC_PREFIX + editor->GetFunctionName().ToStdString());
    }
    else
    {
        value = wxString(ID_PREFIX + StrPrintf("%04X", editor->GetScriptId()));
    }
    return true;
}

#include <rooms/EntityPropertiesWindow.h>

#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <landstalker/misc/Utils.h>
#include <landstalker/main/SpriteData.h>
#include <landstalker/main/GameData.h>
#include <landstalker/behaviours/BehaviourYamlConverter.h>
#include <script/ScriptTreeActionEditors.h>
#include <cmath>
#include <algorithm>
#include <vector>

namespace
{
int ComboSelectionOrParsed(const LookupChoiceControl* combo, int fallback)
{
    const int sel = combo->GetSelection();
    if (sel != wxNOT_FOUND)
    {
        return sel;
    }
    return fallback;
}
}

enum ID
{
    ID_HEADER = 20001,
    ID_TYPE,
    ID_X,
    ID_Y,
    ID_Z,
    ID_ROT,
    ID_PAL,
    ID_SPD,
    ID_BEHAV,
    ID_DLG,
    ID_FH,
    ID_FR,
    ID_FP,
    ID_FD,
    ID_FV,
    ID_FS,
    ID_FG,
    ID_FF,
    ID_FX,
    ID_FT,
    ID_CPYSRC,
    ID_CHEST_PREV,
    ID_CHEST_IDX,
    ID_CHEST_CONTENT,
    ID_BEHAV_NAME_APPLY,
    ID_BEHAV_NAME_CANCEL,
    ID_DLG_MAP_DIALOGUE,
    ID_DLG_MAP_SCRIPT
};

#if defined(__WXMSW__)
#define DLG_SIZE wxSize(560, 550)
#else
#define DLG_SIZE wxSize(560, 700)
#endif


EntityPropertiesWindow::EntityPropertiesWindow(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd)
    : wxDialog(parent, wxID_ANY, "Edit Entity", wxDefaultPosition, DLG_SIZE),
      m_entities(nullptr),
      m_gd_shared(gd),
      m_gd(gd.get()),
      m_id(0),
      m_room(0)
{
    m_chest_id = 0;
    m_disabled_for_room = false;
    m_chest_contents = 0;
    m_orig_chest_contents = 0;
    m_chest_flag = 0;
    m_prev_chest_flag = 0;
    m_prev_chest_contents = 0;

    wxArrayString entity_types;
    for (std::size_t i = 0; i < 256; ++i)
    {
        std::wstring entity_name = Landstalker::SpriteData::GetEntityDisplayName(i);
        entity_types.Add(Landstalker::StrWPrintf(L"[%02X] %ls", i, entity_name.c_str()));
    }
    wxArrayString behaviours;
    for (std::size_t i = 0; i < 1024; ++i)
    {
        std::wstring behaviour_name = Landstalker::SpriteData::GetBehaviourDisplayName(i);
        behaviours.Add(Landstalker::StrWPrintf(L"[%04d] %ls", i, behaviour_name.c_str()));
    }
    // Placeholder labels: SetEntity()'s RefreshDialogueNames() fills in the room's mapped
    // character names.
    wxArrayString dialogues;
    for (std::size_t i = 0; i < 64; ++i)
    {
        dialogues.Add(Landstalker::StrWPrintf(L"[%02d] ???", i));
    }
    wxArrayString items;
    for (std::size_t i = 0; i < 64; ++i)
    {
        auto item_name = gd->GetStringData()->GetItemName(i);
        items.Add(Landstalker::StrWPrintf(L"[%02X] %ls", i, item_name.c_str()));
    }

    wxBoxSizer* root_sizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(root_sizer);

    m_notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    wxNotebook* notebook = m_notebook;
    root_sizer->Add(notebook, 1, wxALL | wxEXPAND, 5);

    wxPanel* properties_page = new wxPanel(notebook, wxID_ANY);
    wxBoxSizer* szr1 = new wxBoxSizer(wxVERTICAL);
    properties_page->SetSizer(szr1);
    notebook->AddPage(properties_page, _("Properties"), true);

    wxPanel* behaviour_page = new wxPanel(notebook, wxID_ANY);
    wxBoxSizer* behaviour_sizer = new wxBoxSizer(wxVERTICAL);
    behaviour_page->SetSizer(behaviour_sizer);
    notebook->AddPage(behaviour_page, _("Behaviour Script"), false);

    wxBoxSizer* behaviour_select_sizer = new wxBoxSizer(wxHORIZONTAL);
    behaviour_sizer->Add(behaviour_select_sizer, 0, wxALL | wxEXPAND, 5);
    behaviour_select_sizer->Add(new wxStaticText(behaviour_page, wxID_ANY, "Behaviour:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_behaviour_tab = new LookupChoiceControl(behaviour_page, ID_BEHAV, wxEmptyString, behaviours,
        wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)));
    behaviour_select_sizer->Add(m_ctrl_behaviour_tab, 1, wxALL | wxEXPAND, 5);

    wxBoxSizer* behaviour_name_sizer = new wxBoxSizer(wxHORIZONTAL);
    behaviour_sizer->Add(behaviour_name_sizer, 0, wxALL | wxEXPAND, 5);
    behaviour_name_sizer->Add(new wxStaticText(behaviour_page, wxID_ANY, "Name:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_behaviour_name = new wxTextCtrl(behaviour_page, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDLG_UNIT(this, wxSize(-1, -1)));
    behaviour_name_sizer->Add(m_ctrl_behaviour_name, 1, wxALL | wxEXPAND, 5);
    m_ctrl_behaviour_name_apply = new wxBitmapButton(behaviour_page, ID_BEHAV_NAME_APPLY,
        wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_BUTTON, wxSize(16, 16)),
        wxDefaultPosition,
        wxDLG_UNIT(this, wxSize(-1, -1)),
        wxBU_AUTODRAW);
    behaviour_name_sizer->Add(m_ctrl_behaviour_name_apply, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_behaviour_name_cancel = new wxBitmapButton(behaviour_page, ID_BEHAV_NAME_CANCEL,
        wxArtProvider::GetBitmap(wxART_CROSS_MARK, wxART_BUTTON, wxSize(16, 16)),
        wxDefaultPosition,
        wxDLG_UNIT(this, wxSize(-1, -1)),
        wxBU_AUTODRAW);
    behaviour_name_sizer->Add(m_ctrl_behaviour_name_cancel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    m_ctrl_dialog_header = new wxStaticText(properties_page, ID_HEADER, Landstalker::StrPrintf("Edit Entity %d", id), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    wxFont m_ctrl_dialog_header_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    m_ctrl_dialog_header_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_ctrl_dialog_header->SetFont(m_ctrl_dialog_header_font);
    szr1->Add(m_ctrl_dialog_header, 0, wxALL, 5);

    wxBoxSizer* szr2a = new wxBoxSizer(wxHORIZONTAL);

    szr1->Add(szr2a, 0, wxEXPAND, 0);
    szr2a->Add(new wxStaticText(properties_page, wxID_ANY, "Entity Type:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_entity_type = new LookupChoiceControl(properties_page, ID_TYPE, wxEmptyString, entity_types,
        wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)));
    szr2a->Add(m_ctrl_entity_type, 1, wxALL | wxEXPAND, 5);

    szr1->Add(new wxStaticLine(properties_page), 0, wxALL | wxEXPAND, 0);
    wxBoxSizer* szr2b = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2b, 0, wxEXPAND, 0);
    szr2b->Add(new wxStaticText(properties_page, wxID_ANY, "X:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_x = new wxSpinCtrlDouble(properties_page, ID_X, wxT("32.0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_x->SetRange(0.5, 64.0);
    m_ctrl_x->SetIncrement(0.5);
    m_ctrl_x->SetSnapToTicks(true);
    m_ctrl_x->SetDigits(1);
    szr2b->Add(m_ctrl_x, 1, wxALL | wxEXPAND, 5);
    szr2b->Add(new wxStaticText(properties_page, wxID_ANY, "Y:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_y = new wxSpinCtrlDouble(properties_page, ID_Y, wxT("32.0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_y->SetRange(0.5, 64.0);
    m_ctrl_y->SetIncrement(0.5);
    m_ctrl_y->SetSnapToTicks(true);
    m_ctrl_y->SetDigits(1);
    szr2b->Add(m_ctrl_y, 1, wxALL | wxEXPAND, 5);
    szr2b->Add(new wxStaticText(properties_page, wxID_ANY, "Z:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_z = new wxSpinCtrlDouble(properties_page, ID_Z, wxT("0.0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_z->SetRange(0.0, 15.5);
    m_ctrl_z->SetIncrement(0.5);
    m_ctrl_z->SetSnapToTicks(true);
    m_ctrl_z->SetDigits(1);
    szr2b->Add(m_ctrl_z, 1, wxALL | wxEXPAND, 5);

    wxBoxSizer* szr2c = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2c, 0, wxALL | wxEXPAND, 0);
    szr2c->Add(new wxStaticText(properties_page, wxID_ANY, "Orientation:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString orientation_choices;
    orientation_choices.Add(wxT("[0] North East"));
    orientation_choices.Add(wxT("[1] South East"));
    orientation_choices.Add(wxT("[2] South West"));
    orientation_choices.Add(wxT("[3] North West"));
    m_ctrl_orientation = new wxChoice(properties_page, ID_ROT, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), orientation_choices, 0);
    szr2c->Add(m_ctrl_orientation, 2, wxALL | wxEXPAND, 5);

    szr2c->Add(new wxStaticText(properties_page, wxID_ANY, "Speed:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_speed = new wxSpinCtrl(properties_page, ID_SPD, wxT("0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_speed->SetRange(0, 7);
    m_ctrl_speed->SetIncrement(1);
    szr2c->Add(m_ctrl_speed, 1, wxALL | wxEXPAND, 5);

    wxBoxSizer* szr2d = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2d, 0, wxALL | wxEXPAND, 0);

    szr2d->Add(new wxStaticText(properties_page, wxID_ANY, "Palette:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString palette_choices;
    palette_choices.Add(wxT("[0] Room"));
    palette_choices.Add(wxT("[1] Sprite Lo/Hi"));
    palette_choices.Add(wxT("[2] Player"));
    palette_choices.Add(wxT("[3] Sprite Lo, HUD"));
    m_ctrl_palette = new wxChoice(properties_page, ID_PAL, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), palette_choices, 0);
    szr2d->Add(m_ctrl_palette, 1, wxALL | wxEXPAND, 5);

    szr2d->Add(new wxStaticText(properties_page, wxID_ANY, "Dialogue:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_dialogue = new LookupChoiceControl(properties_page, ID_DLG, wxEmptyString, dialogues,
        wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)));
    szr2d->Add(m_ctrl_dialogue, 1, wxALL | wxEXPAND, 5);

    wxBoxSizer* szr2e = new wxBoxSizer(wxHORIZONTAL);

    szr2e->Add(new wxStaticText(properties_page, wxID_ANY, "Behaviour:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_behaviour = new LookupChoiceControl(properties_page, ID_BEHAV, wxEmptyString, behaviours,
        wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)));
    szr2e->Add(m_ctrl_behaviour, 1, wxALL | wxEXPAND, 5);
    szr1->Add(szr2e, 0, wxALL | wxEXPAND, 0);
    szr1->Add(new wxStaticLine(properties_page), 0, wxALL | wxEXPAND, 0);
    wxGridSizer* szr2f = new wxGridSizer(3, 3, 0, 0);
    szr1->Add(szr2f, 2, wxALL | wxEXPAND, 5);

    m_ctrl_hostile = new wxCheckBox(properties_page, ID_FH, _("Hostile"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2f->Add(m_ctrl_hostile, 0, wxALL, 5);

    m_ctrl_no_rotate = new wxCheckBox(properties_page, ID_FR, _("No Rotate"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2f->Add(m_ctrl_no_rotate, 0, wxALL, 5);

    m_ctrl_no_pickup = new wxCheckBox(properties_page, ID_FP, _("No Pickup"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2f->Add(m_ctrl_no_pickup, 0, wxALL, 5);

    m_ctrl_has_dialogue = new wxCheckBox(properties_page, ID_FD, _("Has Dialogue"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2f->Add(m_ctrl_has_dialogue, 0, wxALL, 5);

    m_ctrl_visible = new wxCheckBox(properties_page, ID_FV, _("Visible"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2f->Add(m_ctrl_visible, 0, wxALL, 5);

    m_ctrl_solid = new wxCheckBox(properties_page, ID_FS, _("Solid"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2f->Add(m_ctrl_solid, 0, wxALL, 5);

    m_ctrl_has_gravity = new wxCheckBox(properties_page, ID_FG, _("Gravity"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2f->Add(m_ctrl_has_gravity, 0, wxALL, 5);

    m_ctrl_has_friction = new wxCheckBox(properties_page, ID_FF, _("Friction"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2f->Add(m_ctrl_has_friction, 0, wxALL, 5);

    m_ctrl_reserved = new wxCheckBox(properties_page, ID_FX, _("Reserved"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2f->Add(m_ctrl_reserved, 0, wxALL, 5);

    szr1->Add(new wxStaticLine(properties_page), 0, wxALL | wxEXPAND, 0);
    wxBoxSizer* szr2g = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2g, 1, wxALL | wxEXPAND, 5);

    m_ctrl_copy_tiles = new wxCheckBox(properties_page, ID_FT, _("Copy Tiles"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2g->Add(m_ctrl_copy_tiles, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    szr2g->Add(new wxStaticText(properties_page, wxID_ANY, _("Tile Source:")), 0, wxALL | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_copy_source = new wxSpinCtrl(properties_page, ID_CPYSRC, wxT("0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(60, -1)), wxSP_ARROW_KEYS);
    m_ctrl_copy_source->SetRange(0, 15);
    m_ctrl_copy_source->SetIncrement(1);
    szr2g->Add(m_ctrl_copy_source, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    
    szr1->Add(new wxStaticLine(properties_page), 0, wxALL | wxEXPAND, 0);
    m_chest_label = new wxStaticText(properties_page, wxID_ANY, "Chest Options (Chest ID: N/A)", wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    wxFont font = m_chest_label->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    m_chest_label->SetFont(font);
    szr1->Add(m_chest_label, 0, wxALL | wxALIGN_LEFT, 5);
    wxBoxSizer* szr2h = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2h, 0, wxALL | wxEXPAND, 5);
    szr2h->Add(new wxStaticText(properties_page, wxID_ANY, _("Copy from previous room:"), wxDefaultPosition, wxDefaultSize), 0, wxALIGN_CENTER_VERTICAL);
    m_ctrl_chest_prev = new wxCheckBox(properties_page, ID_CHEST_PREV, _(" - Setting applies to whole room!"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    szr2h->Add(m_ctrl_chest_prev, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    wxBoxSizer* szr2i = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2i, 0, wxALL | wxEXPAND, 5);
    szr2i->Add(new wxStaticText(properties_page, wxID_ANY, "Chest Flag ID:", wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0), 0, wxALL | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_chest_idx = new wxTextCtrl(properties_page, ID_CHEST_IDX, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(60, -1)), wxSP_ARROW_KEYS);
    szr2i->Add(m_ctrl_chest_idx, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    szr2i->Add(new wxStaticText(properties_page, wxID_ANY, "Chest Contents:", wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0), 0, wxALL | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_chest_content = new LookupChoiceControl(properties_page, ID_CHEST_CONTENT, "", items, wxDefaultPosition, wxDLG_UNIT(this, wxSize(150, -1)));
    szr2i->Add(m_ctrl_chest_content, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    m_ctrl_behaviour_script = new wxTextCtrl(
        behaviour_page,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxDLG_UNIT(this, wxSize(-1, -1)),
        wxTE_MULTILINE | wxTE_RICH2);
    auto script_font = m_ctrl_behaviour_script->GetFont();
    script_font.SetFamily(wxFONTFAMILY_TELETYPE);
    script_font.SetPointSize(10);
    m_ctrl_behaviour_script->SetFont(script_font);
    behaviour_sizer->Add(m_ctrl_behaviour_script, 1, wxALL | wxEXPAND, 5);

    behaviour_sizer->Add(new wxStaticText(behaviour_page, wxID_ANY, "Also Used By:"), 0, wxLEFT | wxRIGHT | wxTOP, 10);
    m_ctrl_behaviour_usage = new wxTextCtrl(
        behaviour_page,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxDLG_UNIT(this, wxSize(-1, 35)),
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_DONTWRAP);
    behaviour_sizer->Add(m_ctrl_behaviour_usage, 0, wxALL | wxEXPAND, 5);

    root_sizer->Add(new wxStaticLine(this), 0, wxALL | wxEXPAND, 0);
    m_sizer_btn = new wxStdDialogButtonSizer();
    root_sizer->Add(m_sizer_btn, 0, wxALL | wxEXPAND, 5);
    m_btn_ok = new wxButton(this, wxID_OK, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_btn_ok->SetDefault();
    m_sizer_btn->AddButton(m_btn_ok);
    m_btn_cancel = new wxButton(this, wxID_CANCEL, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_sizer_btn->AddButton(m_btn_cancel);
    m_sizer_btn->Realize();

    CentreOnParent(wxBOTH);
    SetAffirmativeId(wxID_OK);
    SetEscapeId(wxID_CANCEL);

    m_btn_ok->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnClickOK), NULL, this);
    m_btn_cancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnClickCancel), NULL, this);
    m_ctrl_entity_type->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_behaviour->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_behaviour_tab->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_behaviour_name_apply->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnApplyBehaviourName), NULL, this);
    m_ctrl_behaviour_name_cancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnCancelBehaviourName), NULL, this);
    m_ctrl_behaviour_tab->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_behaviour_name_apply->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnApplyBehaviourName), NULL, this);
    m_ctrl_behaviour_name_cancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnCancelBehaviourName), NULL, this);
    m_ctrl_chest_prev->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_chest_content->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_dlg_map_dialogue->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnDialogueMapDialogueChange), NULL, this);
    m_ctrl_dlg_map_script->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnDialogueMapScriptChange), NULL, this);
}

void EntityPropertiesWindow::SetEntity(int id, uint16_t room, std::vector<Landstalker::Entity>& entities)
{
    m_id = id;
    m_room = room;
    m_entities = &entities;

    m_chest_id = 0;
    for (int i = 0; i < m_id - 1; ++i)
    {
        if ((*m_entities)[i].IsChest())
        {
            m_chest_id++;
        }
    }
    m_disabled_for_room = m_gd->GetRoomData()->GetNoChestFlagForRoom(m_room);
    auto chests = m_gd->GetRoomData()->GetChestsForRoom(m_room);
    m_chest_contents = m_chest_id < static_cast<int>(chests.size()) ? chests[m_chest_id] : 0;
    m_orig_chest_contents = m_chest_contents;
    m_chest_flag = m_gd->GetRoomData()->GetChestFlagBaseForRoom(m_room) + m_chest_id;
    m_prev_chest_flag = (m_room == 0 ? 0 : m_gd->GetRoomData()->GetChestFlagBaseForRoom(m_room - 1)) + m_chest_id;
    m_prev_chest_contents = m_gd->GetRoomData()->GetChestContentsFromFlag(m_prev_chest_flag);

    const auto entity = &entities[id - 1];
    m_ctrl_dialog_header->SetLabel(Landstalker::StrPrintf("Edit Entity %d", id));
    m_ctrl_entity_type->SetSelection(entity->GetType());
    m_ctrl_x->SetValue(entity->GetXDbl());
    m_ctrl_y->SetValue(entity->GetYDbl());
    m_ctrl_z->SetValue(entity->GetZDbl());
    m_ctrl_orientation->SetSelection(static_cast<int>(entity->GetOrientation()));
    m_ctrl_speed->SetValue(entity->GetSpeed());
    m_ctrl_palette->SetSelection(entity->GetPalette());
    m_ctrl_dialogue->SetSelection(entity->GetDialogue());
    m_ctrl_hostile->SetValue(entity->IsHostile());
    m_ctrl_no_rotate->SetValue(entity->NoRotate());
    m_ctrl_no_pickup->SetValue(entity->NoPickup());
    m_ctrl_has_dialogue->SetValue(entity->HasDialogue());
    m_ctrl_visible->SetValue(entity->IsVisible());
    m_ctrl_solid->SetValue(entity->IsSolid());
    m_ctrl_has_gravity->SetValue(entity->HasGravity());
    m_ctrl_has_friction->SetValue(entity->HasFriction());
    m_ctrl_reserved->SetValue(entity->IsReservedSet());
    m_ctrl_copy_tiles->SetValue(entity->IsTileCopySet());
    m_ctrl_copy_source->SetValue(entity->GetCopySource());
    m_ctrl_chest_prev->SetValue(false);

    // Force the behaviour name/script/usage panels to repopulate for the new entity.
    m_current_behaviour_id = wxNOT_FOUND;
    m_ctrl_behaviour->SetSelection(entity->GetBehaviour());
    m_ctrl_behaviour_tab->SetSelection(entity->GetBehaviour());

    // Room dialogue mapping: reload and refresh the slot names in both dialogue dropdowns.
    // Scripts may have changed since the last open, so the character tree is stale; it is
    // rebuilt when the Dialogue tab is next shown.
    m_dialogue_map = m_gd->GetStringData()->GetRoomCharacters(m_room);
    m_dialogue_map_orig = m_dialogue_map;
    m_char_tree_stale = true;
    RefreshDialogueNames();
    m_ctrl_dlg_map_dialogue->SetSelection(entity->GetDialogue());
    m_notebook->ChangeSelection(0);
    RefreshDialogueMapScript();

    UpdateUI();
    m_btn_ok->SetFocus();
}

EntityPropertiesWindow::~EntityPropertiesWindow()
{
    m_btn_ok->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnClickOK), NULL, this);
    m_btn_cancel->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnClickCancel), NULL, this);
    m_ctrl_entity_type->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_behaviour->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_behaviour_tab->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_behaviour_name_apply->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnApplyBehaviourName), NULL, this);
    m_ctrl_behaviour_name_cancel->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnCancelBehaviourName), NULL, this);
    m_ctrl_chest_prev->Disconnect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
    m_ctrl_chest_content->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(EntityPropertiesWindow::OnChange), NULL, this);
}

void EntityPropertiesWindow::UpdateUI()
{
    int behaviour_id = ComboSelectionOrParsed(m_ctrl_behaviour, (*m_entities)[m_id - 1].GetBehaviour());
    const auto* event_focus = wxWindow::FindFocus();
    if (event_focus == m_ctrl_behaviour_tab)
    {
        behaviour_id = ComboSelectionOrParsed(m_ctrl_behaviour_tab, behaviour_id);
    }
    UpdateBehaviourControls(behaviour_id);
    UpdateBehaviourScript();
    UpdateBehaviourUsage(behaviour_id);

    if (ComboSelectionOrParsed(m_ctrl_entity_type, (*m_entities)[m_id - 1].GetType()) == 0x12) // Chest
    {
        m_chest_label->SetLabel(Landstalker::StrPrintf("Chest Options (Chest ID %d)", m_chest_id));
        if (m_ctrl_chest_prev->GetValue())
        {
            if (m_disabled_for_room == false)
            {
                m_orig_chest_contents = m_chest_contents;
                m_disabled_for_room = true;
            }
            m_ctrl_chest_idx->SetValue(Landstalker::StrPrintf("0x%02X", m_prev_chest_flag));
            m_ctrl_chest_content->SetSelection(m_prev_chest_contents);
        }
        else
        {
            if (m_disabled_for_room == true)
            {
                m_chest_contents = m_orig_chest_contents;
                m_disabled_for_room = false;
            }
            else
            {
                m_chest_contents = ComboSelectionOrParsed(m_ctrl_chest_content, m_chest_contents);
            }
            m_ctrl_chest_idx->SetValue(Landstalker::StrPrintf("0x%02X", m_chest_flag));
            m_ctrl_chest_content->SetSelection(m_chest_contents);
        }
        m_ctrl_chest_prev->SetValue(m_disabled_for_room);
        m_ctrl_chest_prev->Enable(true);
        m_ctrl_chest_idx->Enable(false);
        m_ctrl_chest_content->Enable(!m_disabled_for_room);
    }
    else
    {
        m_chest_label->SetLabel(_("Chest Options (N/A)"));
        m_ctrl_chest_prev->SetValue(false);
        m_ctrl_chest_idx->SetValue("");
        m_ctrl_chest_content->ChangeValue("");
        m_ctrl_chest_prev->Enable(false);
        m_ctrl_chest_idx->Enable(false);
        m_ctrl_chest_content->Enable(false);
    }
}

void EntityPropertiesWindow::UpdateBehaviourControls(int behaviour_id)
{
    if (behaviour_id == wxNOT_FOUND)
    {
        return;
    }

    if (m_ctrl_behaviour->GetSelection() != behaviour_id)
    {
        m_ctrl_behaviour->SetSelection(behaviour_id);
    }
    if (m_ctrl_behaviour_tab->GetSelection() != behaviour_id)
    {
        m_ctrl_behaviour_tab->SetSelection(behaviour_id);
    }

    if (m_current_behaviour_id != behaviour_id)
    {
        m_current_behaviour_id = behaviour_id;
        m_ctrl_behaviour_name->ChangeValue(Landstalker::SpriteData::GetBehaviourDisplayName(behaviour_id));
    }
}

void EntityPropertiesWindow::UpdateBehaviourScript()
{
    if (!m_gd || !m_gd->GetSpriteData())
    {
        m_ctrl_behaviour_script->ChangeValue(_("No game data loaded."));
        return;
    }

    const int behaviour_id = ComboSelectionOrParsed(m_ctrl_behaviour, (*m_entities)[m_id - 1].GetBehaviour());
    try
    {
        const auto script = m_gd->GetSpriteData()->GetScript(behaviour_id);
        const std::wstring script_name = Landstalker::SpriteData::GetBehaviourDisplayName(behaviour_id);
        std::wstring text = Landstalker::StrWPrintf(L"# [%04d] %ls\n\n", behaviour_id, script_name.c_str());
        text += Landstalker::utf8_to_wstr(Landstalker::BehaviourYamlConverter::ToYaml(script.second));
        m_ctrl_behaviour_script->ChangeValue(text);
    }
    catch (const std::exception& e)
    {
        m_ctrl_behaviour_script->ChangeValue(Landstalker::StrPrintf("Unable to load behaviour script %d:\n%s", behaviour_id, e.what()));
    }
}

void EntityPropertiesWindow::UpdateBehaviourUsage(int behaviour_id)
{
    if (!m_gd || !m_gd->GetRoomData() || !m_gd->GetSpriteData())
    {
        m_ctrl_behaviour_usage->ChangeValue(_("No game data loaded."));
        return;
    }

    wxString usage;
    int use_count = 0;
    constexpr int max_visible_usage = 10;
    for (std::size_t room = 0; room < m_gd->GetRoomData()->GetRoomCount(); ++room)
    {
        const auto entities = m_gd->GetSpriteData()->GetRoomEntities(static_cast<uint16_t>(room));
        for (std::size_t entity_idx = 0; entity_idx < entities.size(); ++entity_idx)
        {
            const auto& entity = entities[entity_idx];
            if (entity.GetBehaviour() != behaviour_id)
            {
                continue;
            }

            if (use_count < max_visible_usage)
            {
                if (!usage.IsEmpty())
                {
                    usage += "\n";
                }
                usage += Landstalker::StrWPrintf(L"[%03d] %ls - Entity %d, %ls",
                    room,
                    m_gd->GetRoomData()->GetRoomDisplayName(static_cast<uint16_t>(room)).c_str(),
                    entity_idx + 1,
                    Landstalker::SpriteData::GetEntityDisplayName(entity.GetType()).c_str());
            }
            ++use_count;
        }
    }

    if (use_count == 0)
    {
        usage = _("No room entities use this behaviour.");
    }
    else if (use_count > max_visible_usage)
    {
        usage += Landstalker::StrPrintf("\n...and %d more", use_count - max_visible_usage);
    }
    m_ctrl_behaviour_usage->ChangeValue(usage);
}

bool EntityPropertiesWindow::ApplyBehaviourNameChange()
{
    const int behaviour_id = ComboSelectionOrParsed(m_ctrl_behaviour, (*m_entities)[m_id - 1].GetBehaviour());
    const std::wstring behaviour_name = m_ctrl_behaviour_name->GetValue().ToStdWstring();
    if (!Landstalker::Labels::Update(Landstalker::Labels::C_BEHAVIOURS, behaviour_id, behaviour_name))
    {
        wxMessageBox(_("Behaviour name must be unique and valid."), _("Invalid Behaviour Name"), wxOK | wxICON_ERROR, this);
        return false;
    }

    const wxString display_name = Landstalker::StrWPrintf(L"[%04d] %ls", behaviour_id, behaviour_name.c_str());
    m_ctrl_behaviour->SetString(behaviour_id, display_name);
    m_ctrl_behaviour_tab->SetString(behaviour_id, display_name);
    return true;
}

void EntityPropertiesWindow::RevertBehaviourNameChange()
{
    const int behaviour_id = ComboSelectionOrParsed(m_ctrl_behaviour, (*m_entities)[m_id - 1].GetBehaviour());
    m_ctrl_behaviour_name->ChangeValue(Landstalker::SpriteData::GetBehaviourDisplayName(behaviour_id));
}

bool EntityPropertiesWindow::CommitBehaviourScript()
{
    if (!m_gd || !m_gd->GetSpriteData())
    {
        return true;
    }

    const int behaviour_id = ComboSelectionOrParsed(m_ctrl_behaviour, (*m_entities)[m_id - 1].GetBehaviour());
    try
    {
        const auto commands = Landstalker::BehaviourYamlConverter::FromYaml(
            Landstalker::wstr_to_utf8(m_ctrl_behaviour_script->GetValue().ToStdWstring()));
        m_gd->GetSpriteData()->SetScript(behaviour_id, commands);
    }
    catch (const std::exception& e)
    {
        wxMessageBox(e.what(), _("Error parsing behaviour YAML"), wxOK | wxICON_ERROR, this);
        return false;
    }
    return true;
}

void EntityPropertiesWindow::OnChange(wxCommandEvent& e)
{
    const auto* event_object = e.GetEventObject();
    if (event_object == m_ctrl_behaviour_tab)
    {
        m_ctrl_behaviour->SetSelection(ComboSelectionOrParsed(m_ctrl_behaviour_tab, (*m_entities)[m_id - 1].GetBehaviour()));
    }
    else if (event_object == m_ctrl_behaviour)
    {
        m_ctrl_behaviour_tab->SetSelection(ComboSelectionOrParsed(m_ctrl_behaviour, (*m_entities)[m_id - 1].GetBehaviour()));
    }

    UpdateUI();
    e.Skip();
}

void EntityPropertiesWindow::OnApplyBehaviourName(wxCommandEvent& e)
{
    ApplyBehaviourNameChange();
    e.Skip();
}

void EntityPropertiesWindow::OnCancelBehaviourName(wxCommandEvent& e)
{
    RevertBehaviourNameChange();
    e.Skip();
}

void EntityPropertiesWindow::RefreshDialogueMapScript()
{
    const int dialogue = m_ctrl_dlg_map_dialogue->GetSelection();
    const bool hint_was_shown = m_ctrl_dlg_map_hint->IsShown();
    if (dialogue < 0)
    {
        m_ctrl_dlg_map_script->SetSelection(0);
        m_ctrl_dlg_map_script->Disable();
        m_ctrl_dlg_map_hint->Hide();
    }
    else if (static_cast<std::size_t>(dialogue) > m_dialogue_map.size())
    {
        // The mapping list is contiguous (dialogue slot = list index), so a slot past the
        // next unmapped one can't be mapped without inventing the entries in between.
        m_ctrl_dlg_map_script->SetSelection(0);
        m_ctrl_dlg_map_script->Disable();
        m_ctrl_dlg_map_hint->SetLabel(Landstalker::StrPrintf(
            "This dialogue cannot be mapped yet: dialogues must be mapped in order, and the "
            "next unmapped dialogue is [%02d].", static_cast<int>(m_dialogue_map.size())));
        m_ctrl_dlg_map_hint->Wrap(m_ctrl_dlg_map_hint->GetParent()->GetClientSize().GetWidth() - 20);
        m_ctrl_dlg_map_hint->Show();
    }
    else
    {
        m_ctrl_dlg_map_script->Enable();
        m_ctrl_dlg_map_hint->Hide();
        // Index 0 is "<None>": the first slot past the end of the room's mapping list.
        if (static_cast<std::size_t>(dialogue) < m_dialogue_map.size())
        {
            m_ctrl_dlg_map_script->SetSelection(m_dialogue_map[dialogue] + 1);
        }
        else
        {
            m_ctrl_dlg_map_script->SetSelection(0);
        }
    }
    if (m_ctrl_dlg_map_hint->IsShown() != hint_was_shown)
    {
        m_ctrl_dlg_map_hint->GetParent()->Layout();
    }
    UpdateCharScriptTree();
}

void EntityPropertiesWindow::CommitCharScriptEditing()
{
    if (!m_ctrl_char_script)
    {
        return;
    }
    wxDataViewColumn* column = m_ctrl_char_script->GetColumn(0);
    wxDataViewRenderer* renderer = column ? column->GetRenderer() : nullptr;
    if (renderer && renderer->GetEditorCtrl())
    {
        // Pulls the editor's value through the model (SetValue -> SyncToScriptTable) and
        // closes the editor; an invalid value is simply dropped, as if editing were escaped.
        renderer->FinishEditing();
    }
}

void EntityPropertiesWindow::UpdateCharScriptTree()
{
    if (!m_ctrl_char_script)
    {
        return;
    }
    // Commit (rather than discard) any in-place edit before the model is swapped out from
    // under it - switching character or tab is navigation, not a request to abandon the edit.
    CommitCharScriptEditing();

    // Building the character category tree is the expensive part of this dialog; don't pay
    // for it until the Dialogue tab is actually shown (OnPageChanged re-runs this then).
    if (m_notebook->GetCurrentPage() != m_dialogue_page)
    {
        m_ctrl_char_script->UnselectAll();
        m_ctrl_char_script->AssociateModel(nullptr);
        m_ctrl_char_script->Disable();
        return;
    }
    if (m_char_tree_stale)
    {
        m_ctrl_char_script->UnselectAll();
        m_ctrl_char_script->AssociateModel(nullptr);
        m_char_models.clear();
        m_char_tree = ScriptTreeNode::BuildCategoryTree(m_gd_shared, 2);
        m_char_tree.SetParent();
        m_char_models.resize(m_char_tree.children.size());
        m_char_tree_stale = false;
    }

    const int selection = m_ctrl_dlg_map_script->IsEnabled() ? m_ctrl_dlg_map_script->GetSelection() : 0;
    const int char_id = selection - 1;
    // The last child is the synthesised "Other Functions" entry, not a character.
    const int char_count = static_cast<int>(m_char_tree.children.size()) - 1;
    if (selection <= 0 || char_id >= char_count)
    {
        m_ctrl_char_script->UnselectAll();
        m_ctrl_char_script->AssociateModel(nullptr);
        m_ctrl_char_script->Disable();
        return;
    }

    m_ctrl_char_script->Enable();
    if (!m_char_models[char_id])
    {
        auto it = m_char_tree.children.begin();
        std::advance(it, char_id);
        m_char_models[char_id] = wxObjectDataPtr<ScriptTreeDataViewModel>(new ScriptTreeDataViewModel(
            *it, m_gd_shared->GetScriptData()->GetCharFuncs(), m_gd_shared,
            { m_gd_shared->GetScriptData()->GetCharFuncs(), m_gd_shared->GetScriptData()->GetCutsceneFuncs(),
              m_gd_shared->GetScriptData()->GetShopFuncs(), m_gd_shared->GetScriptData()->GetItemFuncs() }));
    }
    m_ctrl_char_script->UnselectAll();
    m_ctrl_char_script->Freeze();
    m_ctrl_char_script->AssociateModel(m_char_models[char_id].get());
    wxDataViewItemArray children;
    m_char_models[char_id]->GetChildren(wxDataViewItem(), children);
    for (const auto& child : children)
    {
        m_ctrl_char_script->ExpandChildren(child);
    }
    m_ctrl_char_script->Thaw();
}

void EntityPropertiesWindow::RebuildCharScriptTree()
{
    m_char_tree_stale = true;
    UpdateCharScriptTree();
}

void EntityPropertiesWindow::OnPageChanged(wxBookCtrlEvent& e)
{
    // The character tree defers its (expensive) build until the Dialogue tab is shown.
    if (m_notebook->GetCurrentPage() == m_dialogue_page)
    {
        UpdateCharScriptTree();
    }
    e.Skip();
}

ScriptTreeDataViewModel* EntityPropertiesWindow::GetCharScriptModel() const
{
    if (!m_ctrl_char_script || !m_ctrl_char_script->IsEnabled())
    {
        return nullptr;
    }
    return static_cast<ScriptTreeDataViewModel*>(m_ctrl_char_script->GetModel());
}

void EntityPropertiesWindow::UpdateCharScriptButtons()
{
    ScriptTreeDataViewModel* model = GetCharScriptModel();
    const wxDataViewItem selected = model ? m_ctrl_char_script->GetSelection() : wxDataViewItem();
    m_btn_char_add_child->Enable(model && !model->GetAddChildOptions(selected).empty());
    m_btn_char_add_sibling->Enable(model && !model->GetAddSiblingOptions(selected).empty());
    m_btn_char_remove->Enable(model && model->CanRemoveItem(selected));
    m_btn_char_move_up->Enable(model && model->CanMoveItemUp(selected));
    m_btn_char_move_down->Enable(model && model->CanMoveItemDown(selected));
}

void EntityPropertiesWindow::ShowCharScriptAddMenu(bool child)
{
    constexpr int ADD_MENU_BASE = 21000;
    ScriptTreeDataViewModel* model = GetCharScriptModel();
    if (!model)
    {
        return;
    }

    wxDataViewItem selected = m_ctrl_char_script->GetSelection();
    std::vector<ScriptTreeAddOption> options = child ? model->GetAddChildOptions(selected) : model->GetAddSiblingOptions(selected);
    if (options.empty())
    {
        UpdateCharScriptButtons();
        return;
    }

    wxMenu menu;
    for (std::size_t i = 0; i < options.size(); ++i)
    {
        menu.Append(ADD_MENU_BASE + static_cast<int>(i), options.at(i).label);
    }

    const int selection = GetPopupMenuSelectionFromUser(menu);
    if (selection < ADD_MENU_BASE || static_cast<std::size_t>(selection - ADD_MENU_BASE) >= options.size())
    {
        return;
    }

    const ScriptTreeAddOption& option = options.at(selection - ADD_MENU_BASE);
    wxDataViewItem added = child ? model->AddChild(selected, option) : model->AddSibling(selected, option);
    if (added.IsOk())
    {
        if (child)
        {
            m_ctrl_char_script->Expand(selected);
        }
        m_ctrl_char_script->Expand(added);
        m_ctrl_char_script->Select(added);
        m_ctrl_char_script->EnsureVisible(added);
    }
    UpdateCharScriptButtons();
}

void EntityPropertiesWindow::RemoveCharScriptItem()
{
    ScriptTreeDataViewModel* model = GetCharScriptModel();
    if (!model)
    {
        return;
    }

    wxDataViewItem selected = m_ctrl_char_script->GetSelection();
    if (!model->CanRemoveItem(selected))
    {
        UpdateCharScriptButtons();
        return;
    }

    if (wxMessageBox("Remove the selected script item?", "Remove", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES)
    {
        return;
    }

    // Same policy as the Characters script editor: removing a subtree can drop the only
    // reference to an embedded function, so rebuild the category unless the removal is
    // reference-free and inside a standalone function. Both checks walk the subtree, so
    // they must run before the node is destroyed.
    const bool need_rebuild = model->SubtreeAffectsReferences(selected)
        || !model->IsInsideStandaloneFunction(selected);
    model->RemoveItem(selected);
    if (need_rebuild)
    {
        RebuildCharScriptTree();
    }
    else
    {
        UpdateCharScriptButtons();
    }
}

void EntityPropertiesWindow::MoveCharScriptItem(bool up)
{
    ScriptTreeDataViewModel* model = GetCharScriptModel();
    if (!model)
    {
        return;
    }

    const wxDataViewItem selected = m_ctrl_char_script->GetSelection();
    const wxDataViewItem moved = up ? model->MoveItemUp(selected) : model->MoveItemDown(selected);
    if (moved.IsOk())
    {
        m_ctrl_char_script->Select(moved);
        m_ctrl_char_script->EnsureVisible(moved);
    }
    UpdateCharScriptButtons();
}

void EntityPropertiesWindow::OnCharScriptSelectionChanged(wxDataViewEvent& e)
{
    UpdateCharScriptButtons();
    e.Skip();
}

void EntityPropertiesWindow::OnCharScriptItemActivated(wxDataViewEvent& e)
{
    ScriptTreeDataViewModel* model = GetCharScriptModel();

    // A Custom ASM block is edited as a whole in a multi-line dialog rather than in place -
    // activating either the block itself or any of its instruction lines opens it.
    const wxDataViewItem asm_block = (model && e.GetItem().IsOk())
        ? model->GetCustomAsmBlock(e.GetItem()) : wxDataViewItem();
    if (model && asm_block.IsOk())
    {
        wxTextEntryDialog dialog(this,
                                 "Edit the custom assembler block, one instruction per line:",
                                 "Custom ASM",
                                 model->GetCustomAsmText(asm_block),
                                 wxTextEntryDialogStyle | wxTE_MULTILINE);
        if (dialog.ShowModal() == wxID_OK)
        {
            if (model->ApplyCustomAsm(asm_block, dialog.GetValue()))
            {
                // ASM lines can mention function labels, which feeds the reference analysis.
                RebuildCharScriptTree();
            }
            else
            {
                wxMessageBox("The assembler block was not changed - check that every line is a single, valid instruction, "
                             "that at least one instruction remains, and that no line is a script-structure instruction "
                             "(rts, bra, trap #0-#2) - those have their own statement types.",
                             "Custom ASM", wxOK | wxICON_WARNING, this);
            }
        }
        e.Skip();
        return;
    }

    if (m_ctrl_char_script && e.GetItem().IsOk())
    {
        m_ctrl_char_script->EditItem(e.GetItem(), m_ctrl_char_script->GetColumn(0));
    }
    e.Skip();
}

void EntityPropertiesWindow::OnCharScriptEditingDone(wxDataViewEvent& e)
{
    // The model only applies the committed value after this event; a script edit can change
    // function references, so rebuild the category afterwards to keep reference counts and
    // shared-function markers accurate (mirrors the Characters script editor's policy).
    if (!e.IsEditCancelled() && e.GetItem().IsOk())
    {
        CallAfter([this]()
        {
            // An edit flushed by OnClickOK commits after the dialog has closed - the data is
            // already synced, so just mark the (expensive) tree rebuild for the next open.
            if (IsShown())
            {
                RebuildCharScriptTree();
            }
            else
            {
                m_char_tree_stale = true;
            }
        });
    }
    e.Skip();
}

void EntityPropertiesWindow::RefreshDialogueNames()
{
    for (unsigned int i = 0; i < m_ctrl_dialogue->GetCount(); ++i)
    {
        const std::wstring name = i < m_dialogue_map.size()
            ? m_gd->GetStringData()->GetCharacterDisplayName(m_dialogue_map[i])
            : L"???";
        const wxString label = Landstalker::StrWPrintf(L"[%02d] %ls", i, name.c_str());
        m_ctrl_dialogue->SetString(i, label);
        m_ctrl_dlg_map_dialogue->SetString(i, label);
    }
}

void EntityPropertiesWindow::OnDialogueMapDialogueChange(wxCommandEvent& e)
{
    RefreshDialogueMapScript();
    e.Skip();
}

void EntityPropertiesWindow::OnDialogueMapScriptChange(wxCommandEvent& e)
{
    const int dialogue = m_ctrl_dlg_map_dialogue->GetSelection();
    if (dialogue < 0)
    {
        e.Skip();
        return;
    }
    const std::size_t slot = static_cast<std::size_t>(dialogue);
    const int selection = m_ctrl_dlg_map_script->GetSelection();
    if (selection <= 0)
    {
        // "<None>": the mapping list is contiguous (dialogue slot = list index), so only the
        // last mapped slot can be unmapped without renumbering every later dialogue.
        if (slot >= m_dialogue_map.size())
        {
            // Already unmapped - nothing to do.
        }
        else if (slot == m_dialogue_map.size() - 1)
        {
            m_dialogue_map.pop_back();
        }
        else
        {
            wxMessageBox("Only the last mapped dialogue can be unmapped - removing an earlier "
                         "entry would renumber every later dialogue. Use the room's Dialogue "
                         "editor to restructure the mapping list.",
                         "Dialogue Mapping", wxOK | wxICON_INFORMATION, this);
            RefreshDialogueMapScript();
            e.Skip();
            return;
        }
    }
    else
    {
        if (slot > m_dialogue_map.size())
        {
            // Unreachable via the UI (the Script combo is disabled for slots past the next
            // unmapped one), but never invent gap-filling entries.
            e.Skip();
            return;
        }
        if (slot == m_dialogue_map.size())
        {
            m_dialogue_map.push_back(Landstalker::Character(0));
        }
        m_dialogue_map[slot] = static_cast<Landstalker::Character>(selection - 1);
    }
    RefreshDialogueNames();
    UpdateCharScriptTree();
    e.Skip();
}

void EntityPropertiesWindow::OnApplyBehaviourName(wxCommandEvent& e)
{
    ApplyBehaviourNameChange();
    e.Skip();
}

void EntityPropertiesWindow::OnCancelBehaviourName(wxCommandEvent& e)
{
    RevertBehaviourNameChange();
    e.Skip();
}

void EntityPropertiesWindow::OnClickOK(wxCommandEvent& /*evt*/)
{
    auto* entity = &(*m_entities)[m_id - 1];
    m_ctrl_entity_type->CommitPendingSelection();
    m_ctrl_dialogue->CommitPendingSelection();
    m_ctrl_behaviour->CommitPendingSelection();
    m_ctrl_behaviour_tab->CommitPendingSelection();
    m_ctrl_chest_content->CommitPendingSelection();
    // A script edit still open in the character tree's floating editor commits straight into
    // the script tables (they aren't gated on OK), and would otherwise be lost with the dialog.
    CommitCharScriptEditing();

    m_ctrl_entity_type->CommitPendingSelection();
    m_ctrl_dialogue->CommitPendingSelection();
    m_ctrl_behaviour->CommitPendingSelection();
    m_ctrl_behaviour_tab->CommitPendingSelection();
    m_ctrl_chest_content->CommitPendingSelection();

    entity->SetType(ComboSelectionOrParsed(m_ctrl_entity_type, entity->GetType()));
    entity->SetXDbl(m_ctrl_x->GetValue());
    entity->SetYDbl(m_ctrl_y->GetValue());
    entity->SetZDbl(m_ctrl_z->GetValue());
    entity->SetSpeed(m_ctrl_speed->GetValue());
    const int behaviour_selection = ComboSelectionOrParsed(m_ctrl_behaviour, entity->GetBehaviour());
    if (!ApplyBehaviourNameChange())
    {
        return;
    }
    if (!CommitBehaviourScript())
    {
        return;
    }
    entity->SetBehaviour(behaviour_selection);
    entity->SetDialogue(ComboSelectionOrParsed(m_ctrl_dialogue, entity->GetDialogue()));
    entity->SetOrientation(static_cast<Landstalker::Orientation>(m_ctrl_orientation->GetSelection()));
    entity->SetPalette(m_ctrl_palette->GetSelection());
    entity->SetHostile(m_ctrl_hostile->GetValue());
    entity->SetNoRotate(m_ctrl_no_rotate->GetValue());
    entity->SetNoPickup(m_ctrl_no_pickup->GetValue());
    entity->SetHasDialogue(m_ctrl_has_dialogue->GetValue());
    entity->SetVisible(m_ctrl_visible->GetValue());
    entity->SetSolid(m_ctrl_solid->GetValue());
    entity->SetGravity(m_ctrl_has_gravity->GetValue());
    entity->SetFriction(m_ctrl_has_friction->GetValue());
    entity->SetReserved(m_ctrl_reserved->GetValue());
    entity->SetTileCopy(m_ctrl_copy_tiles->GetValue());
    entity->SetCopySource(m_ctrl_copy_source->GetValue());
    if (entity->IsChest())
    {
        if (m_disabled_for_room)
        {
            m_gd->GetRoomData()->SetNoChestFlagForRoom(m_room, true);
        }
        else
        {
            m_gd->GetRoomData()->SetNoChestFlagForRoom(m_room, false);
            auto chests = m_gd->GetRoomData()->GetChestsForRoom(m_room);
            if (m_chest_id >= static_cast<int>(chests.size()))
            {
                chests.resize(m_chest_id + 1); // Fill with 0s up to chest_id
            }
            chests[m_chest_id] = m_chest_contents;
            m_gd->GetRoomData()->SetChestsForRoom(m_room, chests);
        }
        m_gd->GetRoomData()->CleanupChests(*m_gd);
    }
    if (m_dialogue_map != m_dialogue_map_orig)
    {
        m_gd->GetStringData()->SetRoomCharacters(m_room, m_dialogue_map);
    }
    EndModal(wxID_OK);
}

void EntityPropertiesWindow::OnClickCancel(wxCommandEvent& /*evt*/)
{
    EndModal(wxID_CANCEL);
}

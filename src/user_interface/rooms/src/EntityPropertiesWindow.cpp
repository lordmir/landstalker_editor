#include <user_interface/rooms/include/EntityPropertiesWindow.h>

#include <wx/settings.h>
#include <landstalker/misc/include/Utils.h>
#include <landstalker/main/include/SpriteData.h>
#include <cmath>
#include <algorithm>
#include <vector>

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
    ID_CPYSRC
};

EntityPropertiesWindow::EntityPropertiesWindow(wxWindow* parent, int id, Entity* entity, const std::vector<std::wstring>& char_names)
    : wxDialog(parent, wxID_ANY, "Edit Entity", wxDefaultPosition, wxSize(560, 560)),
      m_entity(entity),
      m_id(id)
{
    wxArrayString entity_types;
    for (std::size_t i = 0; i < 256; ++i)
    {
        std::wstring entity_name = SpriteData::GetEntityDisplayName(i);
        entity_types.Add(StrWPrintf(L"[%02X] %ls", i, entity_name.c_str()));
    }
    wxArrayString behaviours;
    for (std::size_t i = 0; i < 1024; ++i)
    {
        std::wstring behaviour_name = SpriteData::GetBehaviourDisplayName(i);
        behaviours.Add(StrWPrintf(L"[%04d] %ls", i, behaviour_name.c_str()));
    }
    wxArrayString dialogues;
    for (std::size_t i = 0; i < 64; ++i)
    {
        std::wstring dialogue_name = i < char_names.size() ? char_names.at(i) : L"???";
        dialogues.Add(StrWPrintf(L"[%02d] %ls", i, dialogue_name.c_str()));
    }

    wxBoxSizer* szr1 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(szr1);

    m_ctrl_dialog_header = new wxStaticText(this, ID_HEADER, StrPrintf("Edit Entity %d", id), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    wxFont m_ctrl_dialog_header_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    m_ctrl_dialog_header_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_ctrl_dialog_header->SetFont(m_ctrl_dialog_header_font);
    szr1->Add(m_ctrl_dialog_header, 0, wxALL, 5);

    wxBoxSizer* szr2a = new wxBoxSizer(wxHORIZONTAL);

    szr1->Add(szr2a, 0, 0, 0);
    szr2a->Add(new wxStaticText(this, wxID_ANY, "Entity Type:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_entity_type = new wxChoice(this, ID_TYPE, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), entity_types, 0);
    m_ctrl_entity_type->SetSelection(entity->GetType());
    szr2a->Add(m_ctrl_entity_type, 0, wxALL | wxEXPAND, 5);

    szr1->Add(new wxStaticLine(this), 0, wxALL | wxEXPAND, 0);
    wxBoxSizer* szr2b = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2b, 0, wxEXPAND, 0);
    szr2b->Add(new wxStaticText(this, wxID_ANY, "X:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_x = new wxSpinCtrlDouble(this, ID_X, wxT("32.0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_x->SetRange(0.5, 64.0);
    m_ctrl_x->SetIncrement(0.5);
    m_ctrl_x->SetSnapToTicks(true);
    m_ctrl_x->SetDigits(1);
    m_ctrl_x->SetValue(entity->GetXDbl());
    szr2b->Add(m_ctrl_x, 1, wxALL | wxEXPAND, 5);
    szr2b->Add(new wxStaticText(this, wxID_ANY, "Y:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_y = new wxSpinCtrlDouble(this, ID_Y, wxT("32.0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_y->SetRange(0.5, 64.0);
    m_ctrl_y->SetIncrement(0.5);
    m_ctrl_y->SetSnapToTicks(true);
    m_ctrl_y->SetDigits(1);
    m_ctrl_y->SetValue(entity->GetYDbl());
    szr2b->Add(m_ctrl_y, 1, wxALL | wxEXPAND, 5);
    szr2b->Add(new wxStaticText(this, wxID_ANY, "Z:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_z = new wxSpinCtrlDouble(this, ID_Z, wxT("0.0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_z->SetRange(0.0, 15.5);
    m_ctrl_z->SetIncrement(0.5);
    m_ctrl_z->SetSnapToTicks(true);
    m_ctrl_z->SetDigits(1);
    m_ctrl_z->SetValue(entity->GetZDbl());
    szr2b->Add(m_ctrl_z, 1, wxALL | wxEXPAND, 5);

    wxBoxSizer* szr2c = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2c, 0, wxALL | wxEXPAND, 0);
    szr2c->Add(new wxStaticText(this, wxID_ANY, "Orientation:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString orientation_choices;
    orientation_choices.Add(wxT("[0] North East"));
    orientation_choices.Add(wxT("[1] South East"));
    orientation_choices.Add(wxT("[2] South West"));
    orientation_choices.Add(wxT("[3] North West"));
    m_ctrl_orientation = new wxChoice(this, ID_ROT, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), orientation_choices, 0);
    m_ctrl_orientation->SetSelection(static_cast<int>(entity->GetOrientation()));
    szr2c->Add(m_ctrl_orientation, 2, wxALL | wxEXPAND, 5);

    szr2c->Add(new wxStaticText(this, wxID_ANY, "Speed:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_speed = new wxSpinCtrl(this, ID_SPD, wxT("0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_speed->SetRange(0, 7);
    m_ctrl_speed->SetValue(entity->GetSpeed());
    m_ctrl_speed->SetIncrement(1);
    szr2c->Add(m_ctrl_speed, 1, wxALL | wxEXPAND, 5);

    wxBoxSizer* szr2d = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2d, 0, wxALL | wxEXPAND, 0);

    szr2d->Add(new wxStaticText(this, wxID_ANY, "Palette:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString palette_choices;
    palette_choices.Add(wxT("[0] Room"));
    palette_choices.Add(wxT("[1] Sprite Lo/Hi"));
    palette_choices.Add(wxT("[2] Player"));
    palette_choices.Add(wxT("[3] Sprite Lo, HUD"));
    m_ctrl_palette = new wxChoice(this, ID_PAL, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), palette_choices, 0);
    m_ctrl_palette->SetSelection(entity->GetPalette());
    szr2d->Add(m_ctrl_palette, 1, wxALL | wxEXPAND, 5);

    szr2d->Add(new wxStaticText(this, wxID_ANY, "Dialogue:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_dialogue = new wxChoice(this, ID_DLG);
    m_ctrl_dialogue->Insert(dialogues, 0);
    m_ctrl_dialogue->SetSelection(entity->GetDialogue());
    szr2d->Add(m_ctrl_dialogue, 1, wxALL | wxEXPAND, 5);

    wxBoxSizer* szr2e = new wxBoxSizer(wxHORIZONTAL);

    szr2e->Add(new wxStaticText(this, wxID_ANY, "Behaviour:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_behaviour = new wxChoice(this, ID_BEHAV);
    m_ctrl_behaviour->Insert(behaviours, 0);
    m_ctrl_behaviour->SetSelection(entity->GetBehaviour());
    szr2e->Add(m_ctrl_behaviour, 1, wxALL | wxEXPAND, 5);
    szr1->Add(szr2e, 0, wxALL | wxEXPAND, 0);
    szr1->Add(new wxStaticLine(this), 0, wxALL | wxEXPAND, 0);
    wxGridSizer* szr2f = new wxGridSizer(3, 3, 0, 0);
    szr1->Add(szr2f, 2, wxALL | wxEXPAND, 5);

    m_ctrl_hostile = new wxCheckBox(this, ID_FH, _("Hostile"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_hostile->SetValue(entity->IsHostile());
    szr2f->Add(m_ctrl_hostile, 0, wxALL, 5);

    m_ctrl_no_rotate = new wxCheckBox(this, ID_FR, _("No Rotate"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_no_rotate->SetValue(entity->NoRotate());
    szr2f->Add(m_ctrl_no_rotate, 0, wxALL, 5);

    m_ctrl_no_pickup = new wxCheckBox(this, ID_FP, _("No Pickup"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_no_pickup->SetValue(entity->NoPickup());
    szr2f->Add(m_ctrl_no_pickup, 0, wxALL, 5);

    m_ctrl_has_dialogue = new wxCheckBox(this, ID_FD, _("Has Dialogue"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_has_dialogue->SetValue(entity->HasDialogue());
    szr2f->Add(m_ctrl_has_dialogue, 0, wxALL, 5);

    m_ctrl_visible = new wxCheckBox(this, ID_FV, _("Visible"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_visible->SetValue(entity->IsVisible());
    szr2f->Add(m_ctrl_visible, 0, wxALL, 5);

    m_ctrl_solid = new wxCheckBox(this, ID_FS, _("Solid"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_solid->SetValue(entity->IsSolid());
    szr2f->Add(m_ctrl_solid, 0, wxALL, 5);

    m_ctrl_has_gravity = new wxCheckBox(this, ID_FG, _("Gravity"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_has_gravity->SetValue(entity->HasGravity());
    szr2f->Add(m_ctrl_has_gravity, 0, wxALL, 5);

    m_ctrl_has_friction = new wxCheckBox(this, ID_FF, _("Friction"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_has_friction->SetValue(entity->HasFriction());
    szr2f->Add(m_ctrl_has_friction, 0, wxALL, 5);

    m_ctrl_reserved = new wxCheckBox(this, ID_FX, _("Reserved"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_reserved->SetValue(entity->IsReservedSet());
    szr2f->Add(m_ctrl_reserved, 0, wxALL, 5);

    szr1->Add(new wxStaticLine(this), 0, wxALL | wxEXPAND, 0);
    wxBoxSizer* szr2g = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2g, 1, wxALL | wxEXPAND, 5);

    m_ctrl_copy_tiles = new wxCheckBox(this, ID_FT, _("Copy Tiles"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_ctrl_copy_tiles->SetValue(entity->IsTileCopySet());
    szr2g->Add(m_ctrl_copy_tiles, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    szr2g->Add(new wxStaticText(this, wxID_ANY, _("Tile Source:")), 0, wxALL | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_copy_source = new wxSpinCtrl(this, ID_CPYSRC, wxT("0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(60, -1)), wxSP_ARROW_KEYS);
    m_ctrl_copy_source->SetRange(0, 15);
    m_ctrl_copy_source->SetValue(entity->GetCopySource());
    m_ctrl_copy_source->SetIncrement(1);
    szr2g->Add(m_ctrl_copy_source, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    szr1->Add(new wxStaticLine(this), 0, wxALL | wxEXPAND, 0);
    m_sizer_btn = new wxStdDialogButtonSizer();
    szr1->Add(m_sizer_btn, 0, wxALL | wxEXPAND, 5);
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
}

EntityPropertiesWindow::~EntityPropertiesWindow()
{
    m_btn_ok->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnClickOK), NULL, this);
    m_btn_cancel->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(EntityPropertiesWindow::OnClickCancel), NULL, this);
}

void EntityPropertiesWindow::OnClickOK(wxCommandEvent& /*evt*/)
{
    m_entity->SetType(m_ctrl_entity_type->GetSelection());
    m_entity->SetXDbl(m_ctrl_x->GetValue());
    m_entity->SetYDbl(m_ctrl_y->GetValue());
    m_entity->SetZDbl(m_ctrl_z->GetValue());
    m_entity->SetSpeed(m_ctrl_speed->GetValue());
    m_entity->SetBehaviour(m_ctrl_behaviour->GetSelection());
    m_entity->SetDialogue(m_ctrl_dialogue->GetSelection());
    m_entity->SetOrientation(static_cast<Orientation>(m_ctrl_orientation->GetSelection()));
    m_entity->SetPalette(m_ctrl_palette->GetSelection());
    m_entity->SetHostile(m_ctrl_hostile->GetValue());
    m_entity->SetNoRotate(m_ctrl_no_rotate->GetValue());
    m_entity->SetNoPickup(m_ctrl_no_pickup->GetValue());
    m_entity->SetHasDialogue(m_ctrl_has_dialogue->GetValue());
    m_entity->SetVisible(m_ctrl_visible->GetValue());
    m_entity->SetSolid(m_ctrl_solid->GetValue());
    m_entity->SetGravity(m_ctrl_has_gravity->GetValue());
    m_entity->SetFriction(m_ctrl_has_friction->GetValue());
    m_entity->SetReserved(m_ctrl_reserved->GetValue());
    m_entity->SetTileCopy(m_ctrl_copy_tiles->GetValue());
    m_entity->SetCopySource(m_ctrl_copy_source->GetValue());
    EndModal(wxID_OK);
}

void EntityPropertiesWindow::OnClickCancel(wxCommandEvent& /*evt*/)
{
    EndModal(wxID_CANCEL);
}

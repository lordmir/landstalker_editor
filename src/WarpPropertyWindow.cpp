#include <WarpPropertyWindow.h>

#include <wx/settings.h>
#include <Utils.h>
#include <cmath>
#include <algorithm>
#include <vector>

enum ID
{
    ID_HEADER = 20001,
    ID_SRC_ROOM,
    ID_SRC_X,
    ID_SRC_Y,
    ID_DST_ROOM,
    ID_DST_X,
    ID_DST_Y,
    ID_TYPE,
    ID_SIZE
};

WarpPropertyWindow::WarpPropertyWindow(wxWindow* parent, uint16_t src_room, int id, WarpList::Warp* warp, GameData& gd)
    : wxDialog(parent, wxID_ANY, "Edit Warp", wxDefaultPosition, wxSize(500, 250)),
      m_warp(warp),
      m_id(id),
      m_room_src(src_room),
      m_unknown_selectable(false)
{

    wxBoxSizer* szr1 = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(szr1);

    m_ctrl_dialog_header = new wxStaticText(this, ID_HEADER, StrPrintf("Edit Warp %d", id), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    wxFont m_ctrl_dialog_header_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    m_ctrl_dialog_header_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_ctrl_dialog_header->SetFont(m_ctrl_dialog_header_font);
    szr1->Add(m_ctrl_dialog_header, 0, wxALL, 5);

    wxFlexGridSizer* szr2a = new wxFlexGridSizer(2, 6, 0, 0);
    szr2a->SetFlexibleDirection(wxBOTH);
    szr2a->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    szr1->Add(szr2a, 0, 0, 0);
    szr2a->Add(new wxStaticText(this, wxID_ANY, "Source Room:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString room_names;
    for (std::size_t i = 0; i < gd.GetRoomData()->GetRoomCount(); ++i)
    {
        room_names.Add(gd.GetRoomData()->GetRoom(i)->name);
    }
    m_ctrl_src_room = new wxChoice(this, ID_SRC_ROOM, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), room_names, 0);
    m_ctrl_src_room->SetSelection(src_room);
    m_ctrl_src_room->Enable(false);
    szr2a->Add(m_ctrl_src_room, 1, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, 5);
    szr2a->Add(new wxStaticText(this, wxID_ANY, "X:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_src_x = new wxSpinCtrl(this, ID_SRC_X, wxT("0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_src_x->SetRange(0, 63);
    m_ctrl_src_x->SetValue(warp->room1 == src_room ? warp->x1 : warp->x2);
    m_ctrl_src_x->SetIncrement(1);
    szr2a->Add(m_ctrl_src_x, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    szr2a->Add(new wxStaticText(this, wxID_ANY, "Y:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_src_y = new wxSpinCtrl(this, ID_SRC_Y, wxT("0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_src_y->SetRange(0, 63);
    m_ctrl_src_y->SetValue(warp->room1 == src_room ? warp->y1 : warp->y2);
    m_ctrl_src_y->SetIncrement(1);
    szr2a->Add(m_ctrl_src_y, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    szr2a->Add(new wxStaticText(this, wxID_ANY, "Destination Room:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    if (warp->room1 == src_room ? warp->room2 == 0xFFFF : warp->room1 == 0xFFFF)
    {
        room_names.insert(room_names.begin(), "<UNKNOWN>");
        m_unknown_selectable = true;
    }
    m_ctrl_dst_room = new wxChoice(this, ID_DST_ROOM, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), room_names, 0);
    if (m_unknown_selectable)
    {
        if (warp->room1 == src_room ? warp->room2 == 0xFFFF : warp->room1 == 0xFFFF)
        {
            m_ctrl_dst_room->SetSelection(0);
        }
        else
        {
            m_ctrl_dst_room->SetSelection((warp->room1 == src_room ? warp->room2 : warp->room1) + 1);
        }
    }
    else
    {
        m_ctrl_dst_room->SetSelection(warp->room1 == src_room ? warp->room2 : warp->room1);
    }
    szr2a->Add(m_ctrl_dst_room, 1, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, 5);
    szr2a->Add(new wxStaticText(this, wxID_ANY, "X:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_dst_x = new wxSpinCtrl(this, ID_DST_X, wxT("0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_dst_x->SetRange(0, 63);
    m_ctrl_dst_x->SetValue(warp->room1 == src_room ? warp->x2 : warp->x1);
    m_ctrl_dst_x->SetIncrement(1);
    szr2a->Add(m_ctrl_dst_x, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    szr2a->Add(new wxStaticText(this, wxID_ANY, "Y:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_dst_y = new wxSpinCtrl(this, ID_DST_Y, wxT("0"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxSP_ARROW_KEYS);
    m_ctrl_dst_y->SetRange(0, 63);
    m_ctrl_dst_y->SetValue(warp->room1 == src_room ? warp->y2 : warp->y1);
    m_ctrl_dst_y->SetIncrement(1);
    szr2a->Add(m_ctrl_dst_y, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

   
    wxBoxSizer* szr2b = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2b, 0, wxEXPAND, 0);

    wxArrayString type_names;
    type_names.Add("[0] Normal");
    type_names.Add("[1] Stairs SE");
    type_names.Add("[2] Stairs SW");
    type_names.Add("[3] Unknown");
    wxArrayString size_names;
    size_names.Add("1X1");
    size_names.Add("2X1");
    size_names.Add("3X1");
    size_names.Add("1X2");
    size_names.Add("1X3");

    szr2b->Add(new wxStaticText(this, wxID_ANY, "Type:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_type = new wxChoice(this, ID_TYPE, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), type_names, 0);
    m_ctrl_type->SetSelection(static_cast<int>(warp->type));
    szr2b->Add(m_ctrl_type, 1, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, 5);
    szr2b->Add(new wxStaticText(this, wxID_ANY, "Size:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_ctrl_size = new wxChoice(this, ID_SIZE, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), size_names, 0);
    m_ctrl_size->SetSelection(warp->y_size > 1 ? warp->y_size + 1 : warp->x_size - 1);
    szr2b->Add(m_ctrl_size, 1, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, 5);

    wxBoxSizer* szr2c = new wxBoxSizer(wxHORIZONTAL);
    szr1->Add(szr2c, 1, wxEXPAND, 0);
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

    m_btn_ok->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(WarpPropertyWindow::OnClickOK), NULL, this);
    m_btn_cancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(WarpPropertyWindow::OnClickCancel), NULL, this);
}

WarpPropertyWindow::~WarpPropertyWindow()
{
    m_btn_ok->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(WarpPropertyWindow::OnClickOK), NULL, this);
    m_btn_cancel->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(WarpPropertyWindow::OnClickCancel), NULL, this);
}

void WarpPropertyWindow::OnClickOK(wxCommandEvent& evt)
{
    m_warp->type = static_cast<WarpList::Warp::Type>(m_ctrl_type->GetSelection());
    if (m_ctrl_size->GetSelection() > 2)
    {
        m_warp->x_size = 1;
        m_warp->y_size = m_ctrl_size->GetSelection() - 1;
    }
    else
    {
        m_warp->x_size = m_ctrl_size->GetSelection() + 1;
        m_warp->y_size = 1;
    }

    if (m_warp->room1 == m_room_src)
    {
        m_warp->x1 = m_ctrl_src_x->GetValue();
        m_warp->y1 = m_ctrl_src_y->GetValue();
        m_warp->x2 = m_ctrl_dst_x->GetValue();
        m_warp->y2 = m_ctrl_dst_y->GetValue();
        if (m_unknown_selectable)
        {
            if (m_ctrl_dst_room->GetSelection() == 0)
            {
                m_warp->room2 = 0xFFFF;
            }
            else
            {
                m_warp->room2 = m_ctrl_dst_room->GetSelection() - 1;
            }
        }
        else
        {
            m_warp->room2 = m_ctrl_dst_room->GetSelection();
        }
    }
    else
    {
        m_warp->x2 = m_ctrl_src_x->GetValue();
        m_warp->y2 = m_ctrl_src_y->GetValue();
        m_warp->x1 = m_ctrl_dst_x->GetValue();
        m_warp->y1 = m_ctrl_dst_y->GetValue();
        if (m_unknown_selectable)
        {
            if (m_ctrl_dst_room->GetSelection() == 0)
            {
                m_warp->room1 = 0xFFFF;
            }
            else
            {
                m_warp->room1 = m_ctrl_dst_room->GetSelection() - 1;
            }
        }
        else
        {
            m_warp->room1 = m_ctrl_dst_room->GetSelection();
        }
    }

    EndModal(wxID_OK);
}

void WarpPropertyWindow::OnClickCancel(wxCommandEvent& evt)
{
    EndModal(wxID_CANCEL);
}

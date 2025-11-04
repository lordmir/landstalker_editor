#include <misc/SelectionControlFrame.h>
#include <landstalker/misc/Utils.h>
#include <rooms/RoomViewerFrame.h>

SelectionControlFrame::SelectionControlFrame(wxWindow* parent, ImageList* imglst)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, { 220, 200 }, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
    m_imglst(imglst)
{
    const int plus_img = m_imglst->GetIdx("plus");
    const int minus_img = m_imglst->GetIdx("minus");
    const int up_img = m_imglst->GetIdx("up");
    const int down_img = m_imglst->GetIdx("down");
    wxBoxSizer* vboxsizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vboxsizer);

    m_buttons_boxsizer = new wxBoxSizer(wxHORIZONTAL);

    vboxsizer->Add(m_buttons_boxsizer, 0, 0);
    m_ctrl_add = new wxBitmapButton(this, wxID_ADD, imglst->GetBitmap(plus_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    m_buttons_boxsizer->Add(m_ctrl_add, 0, 0, 0);
    m_ctrl_delete = new wxBitmapButton(this, wxID_DELETE, imglst->GetBitmap(minus_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    m_buttons_boxsizer->Add(m_ctrl_delete, 0, 0, 0);
    m_ctrl_move_up = new wxBitmapButton(this, wxID_UP, imglst->GetBitmap(up_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    m_buttons_boxsizer->Add(m_ctrl_move_up, 0, 0, 0);
    m_ctrl_move_down = new wxBitmapButton(this, wxID_DOWN, imglst->GetBitmap(down_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    m_buttons_boxsizer->Add(m_ctrl_move_down, 0, 0, 0);

    m_ctrl_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxArrayString(), wxWANTS_CHARS);
    vboxsizer->Add(m_ctrl_list, 1, wxALL | wxEXPAND);

    m_ctrl_list->Connect(wxEVT_LISTBOX, wxCommandEventHandler(SelectionControlFrame::OnSelect), NULL, this);
    m_ctrl_list->Connect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(SelectionControlFrame::OnListDoubleClick), NULL, this);
    m_ctrl_list->Connect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
    m_ctrl_add->Connect(wxEVT_BUTTON, wxCommandEventHandler(SelectionControlFrame::OnAdd), NULL, this);
    m_ctrl_add->Connect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
    m_ctrl_delete->Connect(wxEVT_BUTTON, wxCommandEventHandler(SelectionControlFrame::OnDelete), NULL, this);
    m_ctrl_delete->Connect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
    m_ctrl_move_up->Connect(wxEVT_BUTTON, wxCommandEventHandler(SelectionControlFrame::OnMoveUp), NULL, this);
    m_ctrl_move_up->Connect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
    m_ctrl_move_down->Connect(wxEVT_BUTTON, wxCommandEventHandler(SelectionControlFrame::OnMoveDown), NULL, this);
    m_ctrl_move_down->Connect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
}

SelectionControlFrame::~SelectionControlFrame()
{
    m_ctrl_list->Disconnect(wxEVT_LISTBOX, wxCommandEventHandler(SelectionControlFrame::OnSelect), NULL, this);
    m_ctrl_list->Disconnect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(SelectionControlFrame::OnListDoubleClick), NULL, this);
    m_ctrl_list->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
    m_ctrl_add->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SelectionControlFrame::OnAdd), NULL, this);
    m_ctrl_add->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
    m_ctrl_delete->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SelectionControlFrame::OnDelete), NULL, this);
    m_ctrl_delete->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
    m_ctrl_move_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SelectionControlFrame::OnMoveUp), NULL, this);
    m_ctrl_move_up->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
    m_ctrl_move_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(SelectionControlFrame::OnMoveDown), NULL, this);
    m_ctrl_move_down->Disconnect(wxEVT_CHAR, wxKeyEventHandler(SelectionControlFrame::OnKeyDown), NULL, this);
}

void SelectionControlFrame::SetSelected(int selected)
{
    if (selected > 0 && selected <= GetMaxSelection())
    {
        m_ctrl_list->SetSelection(selected - 1);
    }
    else
    {
        m_ctrl_list->SetSelection(-1);
    }
}

int SelectionControlFrame::GetSelected()
{
    auto sel = m_ctrl_list->GetSelection() + 1;
    if (sel > 0 && sel <= GetMaxSelection())
    {
        return sel;
    }
    return -1;
}

void SelectionControlFrame::FireEvent(const wxEventType& e)
{
    wxCommandEvent evt(e);
    evt.SetClientData(reinterpret_cast<void*>(this->GetParent()));
    evt.SetInt(GetSelected());
    evt.SetString(m_ctrl_list->GetStringSelection());
    wxPostEvent(this->GetParent(), evt);
}

void SelectionControlFrame::UpdateUI()
{
    m_ctrl_list->Freeze();
    int sel = m_ctrl_list->GetSelection();
    m_ctrl_list->Clear();
    if (m_ctrl_list->GetCount() > static_cast<unsigned int>(GetMaxSelection()))
    {
        for (unsigned int i = m_ctrl_list->GetCount(); i >= static_cast<unsigned int>(GetMaxSelection()); --i)
        {
            m_ctrl_list->Delete(i);
        }
    }
    else if (m_ctrl_list->GetCount() < static_cast<unsigned int>(GetMaxSelection()))
    {
        for (unsigned int i = m_ctrl_list->GetCount(); i < static_cast<unsigned int>(GetMaxSelection()); ++i)
        {
            m_ctrl_list->Insert(MakeLabel(i), i);
        }
    }
    for (unsigned int i = 0; i < static_cast<unsigned int>(GetMaxSelection()); ++i)
    {
        auto label = MakeLabel(i);
        if (m_ctrl_list->GetString(i) != label)
        {
            m_ctrl_list->SetString(i, label);
        }
    }
    UpdateOtherControls();
    if (sel >= static_cast<int>(m_ctrl_list->GetCount()) || sel < 0)
    {
        sel = -1;
    }
    m_ctrl_list->SetSelection(sel);
    m_ctrl_list->Thaw();
}

void SelectionControlFrame::OnSelect(wxCommandEvent& /*evt*/)
{
    Select();
}

void SelectionControlFrame::OnListDoubleClick(wxCommandEvent& /*evt*/)
{
    OpenElement();
}

void SelectionControlFrame::OnKeyDown(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() != WXK_RETURN)
    {
        evt.Skip(HandleKeyPress(evt.GetKeyCode(), evt.GetModifiers()));
    }
    else
    {
        evt.Skip();
    }
}

void SelectionControlFrame::OnAdd(wxCommandEvent& /*evt*/)
{
    Add();
}

void SelectionControlFrame::OnDelete(wxCommandEvent& /*evt*/)
{
    Delete();
}

void SelectionControlFrame::OnMoveUp(wxCommandEvent& /*evt*/)
{
    MoveUp();
}

void SelectionControlFrame::OnMoveDown(wxCommandEvent& /*evt*/)
{
    MoveDown();
}

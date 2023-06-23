#include <EntityControlFrame.h>
#include <Utils.h>
#include <RoomViewerFrame.h>

wxDEFINE_EVENT(EVT_ENTITY_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_ENTITY_OPEN_PROPERTIES, wxCommandEvent);
wxDEFINE_EVENT(EVT_ENTITY_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_ENTITY_DELETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_ENTITY_MOVE_UP, wxCommandEvent);
wxDEFINE_EVENT(EVT_ENTITY_MOVE_DOWN, wxCommandEvent);

EntityControlFrame::EntityControlFrame(RoomViewerFrame* parent, ImageList* imglst)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, { 220, 200 }, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
    m_rvf(parent),
    m_imglst(imglst)
{
    const int plus_img = m_imglst->GetIdx("plus");
    const int minus_img = m_imglst->GetIdx("minus");
    const int up_img = m_imglst->GetIdx("up");
    const int down_img = m_imglst->GetIdx("down");
    wxBoxSizer* vboxsizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vboxsizer);

    wxBoxSizer* hboxsizer = new wxBoxSizer(wxHORIZONTAL);

    vboxsizer->Add(hboxsizer, 0, 0);
    m_ctrl_add_entity = new wxBitmapButton(this, wxID_ADD, imglst->GetBitmap(plus_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    hboxsizer->Add(m_ctrl_add_entity, 0, 0, 0);
    m_ctrl_delete_entity = new wxBitmapButton(this, wxID_DELETE, imglst->GetBitmap(minus_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    hboxsizer->Add(m_ctrl_delete_entity, 0, 0, 0);
    m_ctrl_move_entity_up = new wxBitmapButton(this, wxID_UP, imglst->GetBitmap(up_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    hboxsizer->Add(m_ctrl_move_entity_up, 0, 0, 0);
    m_ctrl_move_entity_down = new wxBitmapButton(this, wxID_DOWN, imglst->GetBitmap(down_img), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxBU_AUTODRAW);
    hboxsizer->Add(m_ctrl_move_entity_down, 0, 0, 0);

    m_ctrl_entity_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxArrayString(), wxWANTS_CHARS);
    vboxsizer->Add(m_ctrl_entity_list, 1, wxALL | wxEXPAND);

    m_ctrl_entity_list->Connect(wxEVT_LISTBOX, wxCommandEventHandler(EntityControlFrame::OnSelect), NULL, this);
    m_ctrl_entity_list->Connect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(EntityControlFrame::OnListDoubleClick), NULL, this);
    m_ctrl_entity_list->Connect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
    m_ctrl_add_entity->Connect(wxEVT_BUTTON, wxCommandEventHandler(EntityControlFrame::OnEntityAdd), NULL, this);
    m_ctrl_add_entity->Connect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
    m_ctrl_delete_entity->Connect(wxEVT_BUTTON, wxCommandEventHandler(EntityControlFrame::OnEntityDelete), NULL, this);
    m_ctrl_delete_entity->Connect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
    m_ctrl_move_entity_up->Connect(wxEVT_BUTTON, wxCommandEventHandler(EntityControlFrame::OnEntityMoveUp), NULL, this);
    m_ctrl_move_entity_up->Connect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
    m_ctrl_move_entity_down->Connect(wxEVT_BUTTON, wxCommandEventHandler(EntityControlFrame::OnEntityMoveDown), NULL, this);
    m_ctrl_move_entity_down->Connect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
}

EntityControlFrame::~EntityControlFrame()
{
    m_ctrl_entity_list->Disconnect(wxEVT_LISTBOX, wxCommandEventHandler(EntityControlFrame::OnSelect), NULL, this);
    m_ctrl_entity_list->Disconnect(wxEVT_LISTBOX_DCLICK, wxCommandEventHandler(EntityControlFrame::OnListDoubleClick), NULL, this);
    m_ctrl_entity_list->Disconnect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
    m_ctrl_add_entity->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(EntityControlFrame::OnEntityAdd), NULL, this);
    m_ctrl_add_entity->Disconnect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
    m_ctrl_delete_entity->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(EntityControlFrame::OnEntityDelete), NULL, this);
    m_ctrl_delete_entity->Disconnect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
    m_ctrl_move_entity_up->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(EntityControlFrame::OnEntityMoveUp), NULL, this);
    m_ctrl_move_entity_up->Disconnect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
    m_ctrl_move_entity_down->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(EntityControlFrame::OnEntityMoveDown), NULL, this);
    m_ctrl_move_entity_down->Disconnect(wxEVT_CHAR, wxKeyEventHandler(EntityControlFrame::OnKeyDown), NULL, this);
}

void EntityControlFrame::SetEntities(const std::vector<Entity>& entities)
{
    m_entities = entities;
    UpdateUI();
}

void EntityControlFrame::SetSelected(int selected)
{
    if (selected > 0 && selected <= m_ctrl_entity_list->GetCount())
    {
        m_ctrl_entity_list->SetSelection(selected - 1);
    }
    else
    {
        m_ctrl_entity_list->SetSelection(-1);
    }
}

int EntityControlFrame::GetSelected()
{
    auto sel = m_ctrl_entity_list->GetSelection() + 1;
    if (sel > 0 && sel <= m_entities.size())
    {
        return sel;
    }
    return -1;
}

void EntityControlFrame::ResetEntities()
{
    m_entities.clear();
}

void EntityControlFrame::FireEvent(const wxEventType& e, intptr_t userdata)
{
    wxCommandEvent evt(e);
    evt.SetClientData(reinterpret_cast<void*>(userdata));
    wxPostEvent(this->GetParent(), evt);
}

void EntityControlFrame::UpdateUI()
{
    m_ctrl_entity_list->Clear();
    int num_elems = m_entities.size();
    if (m_ctrl_entity_list->GetCount() > m_entities.size())
    {
        for (int i = m_ctrl_entity_list->GetCount(); i >= m_entities.size(); --i)
        {
            m_ctrl_entity_list->Delete(i);
        }
    }
    else if (m_ctrl_entity_list->GetCount() < m_entities.size())
    {
        num_elems = m_ctrl_entity_list->GetCount();
        for (int i = m_ctrl_entity_list->GetCount(); i < m_entities.size(); ++i)
        {
            m_ctrl_entity_list->Insert(StrPrintf("%02d: %s", i + 1, m_entities[i].GetTypeName().c_str()), i);
        }
    }
    int i = 0;
    for (const auto& entity : m_entities)
    {
        auto label = StrPrintf("%02d: %s", i + 1, entity.GetTypeName().c_str());
        if (m_ctrl_entity_list->GetString(i) != label)
        {
            m_ctrl_entity_list->SetString(i, label);
        }
        i++;
    }
}

void EntityControlFrame::OnSelect(wxCommandEvent& evt)
{
    FireEvent(EVT_ENTITY_SELECT, reinterpret_cast<intptr_t>(this->GetParent()));
}

void EntityControlFrame::OnListDoubleClick(wxCommandEvent& evt)
{
    FireEvent(EVT_ENTITY_OPEN_PROPERTIES, reinterpret_cast<intptr_t>(this->GetParent()));
}

void EntityControlFrame::OnKeyDown(wxKeyEvent& evt)
{
    if (m_rvf != nullptr && evt.GetKeyCode() != WXK_RETURN)
    {
        evt.Skip(m_rvf->HandleKeyDown(evt.GetKeyCode(), evt.GetModifiers()));
    }
    else
    {
        evt.Skip();
    }
}

void EntityControlFrame::OnEntityAdd(wxCommandEvent& evt)
{
    FireEvent(EVT_ENTITY_ADD, reinterpret_cast<intptr_t>(this->GetParent()));
}

void EntityControlFrame::OnEntityDelete(wxCommandEvent& evt)
{
    FireEvent(EVT_ENTITY_DELETE, reinterpret_cast<intptr_t>(this->GetParent()));
}

void EntityControlFrame::OnEntityMoveUp(wxCommandEvent& evt)
{
    FireEvent(EVT_ENTITY_MOVE_UP, reinterpret_cast<intptr_t>(this->GetParent()));
}

void EntityControlFrame::OnEntityMoveDown(wxCommandEvent& evt)
{
    FireEvent(EVT_ENTITY_MOVE_DOWN, reinterpret_cast<intptr_t>(this->GetParent()));
}

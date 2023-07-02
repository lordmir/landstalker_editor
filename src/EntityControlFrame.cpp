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
    : SelectionControlFrame(parent, imglst),
    m_rvf(parent)
{
}

EntityControlFrame::~EntityControlFrame()
{
}

void EntityControlFrame::SetEntities(const std::vector<Entity>& entities)
{
    m_entities = entities;
    UpdateUI();
}

void EntityControlFrame::ResetEntities()
{
    m_entities.clear();
    UpdateUI();
}

int EntityControlFrame::GetMaxSelection() const
{
    return m_entities.size();
}

void EntityControlFrame::Select()
{
    FireEvent(EVT_ENTITY_SELECT);
}

void EntityControlFrame::OpenElement()
{
    FireEvent(EVT_ENTITY_OPEN_PROPERTIES);
}

bool EntityControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
    return m_rvf->HandleKeyDown(key, modifiers);
}

void EntityControlFrame::Add()
{
    FireEvent(EVT_ENTITY_ADD);
}

void EntityControlFrame::Delete()
{
    FireEvent(EVT_ENTITY_DELETE);
}

void EntityControlFrame::MoveUp()
{
    FireEvent(EVT_ENTITY_MOVE_UP);
}

void EntityControlFrame::MoveDown()
{
    FireEvent(EVT_ENTITY_MOVE_DOWN);
}

std::string EntityControlFrame::MakeLabel(int index) const
{
    return StrPrintf("%02d: (%04.1f, %04.1f, %04.1f) %s", index + 1, m_entities[index].GetXDbl(),
        m_entities[index].GetYDbl(), m_entities[index].GetZDbl(), m_entities[index].GetTypeName().c_str());
}

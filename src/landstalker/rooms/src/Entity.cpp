#include <landstalker/rooms/include/Entity.h>
#include <algorithm>
#include <cmath>
#include <landstalker/misc/include/Utils.h>

Entity::Entity(std::array<uint8_t, 8> data)
{
    m_x_pos = (data[0] & 0x3F) << 8;
    m_y_pos = (data[1] & 0x3F) << 8;
    m_orientation = static_cast<Orientation>((data[0] >> 6) & 3);
    m_palette = (data[1] >> 6) & 0x03;

    m_speed = data[2] & 0x07;
    m_hostile = (data[2] & 0x80) > 0;
    m_pickupable = (data[2] & 0x40) > 0;
    m_has_dialogue = (data[2] & 0x20) > 0;
    m_no_rotate = (data[2] & 0x10) > 0;
    m_no_friction = (data[2] & 0x08) > 0;

    m_x_pos += (data[3] & 0x80) > 0 ? 0x100 : 0x80;
    m_y_pos += (data[3] & 0x40) > 0 ? 0x100 : 0x80;
    m_reserved = (data[3] & 0x20) > 0;
    m_copy_tiles = (data[3] & 0x10) > 0;
    m_copy_source = data[3] & 0x0F;

    m_behaviour = ((data[4] & 0x03) << 8) | data[7];
    m_dialogue = (data[4] >> 2) & 0x3F;

    m_type = data[5];

    m_z_pos = (data[6] & 0x0F) << 8;
    m_z_pos += (data[6] & 0x40) > 0 ? 0x80 : 0x00;
    m_no_gravity = (data[6] & 0x80) > 0;
    m_invisible = (data[6] & 0x20) > 0;
    m_not_solid = (data[6] & 0x10) > 0;
}

Entity::Entity()
    : m_x_pos(0x1F80),
      m_y_pos(0x1F80),
      m_z_pos(0x000),
      m_type(0),
      m_orientation(Orientation::NE),
      m_palette(2),
      m_speed(0),
      m_behaviour(0),
      m_dialogue(0),
      m_copy_tiles(false),
      m_copy_source(0),
      m_hostile(false),
      m_pickupable(false),
      m_has_dialogue(false),
      m_no_rotate(false),
      m_no_friction(false),
      m_no_gravity(false),
      m_invisible(false),
      m_not_solid(false),
      m_reserved(false)
{
}

bool Entity::operator==(const Entity& rhs) const
{
    return this->GetData() == rhs.GetData();
}

bool Entity::operator!=(const Entity& rhs) const
{
    return !(*this == rhs);
}

std::array<uint8_t, 8> Entity::GetData() const
{
    std::array<uint8_t, 8> retval{};

    retval[0] = m_x_pos >> 8;
    if ((m_x_pos & 0x80) == 0)
    {
        retval[0]--;
    }
    retval[0] &= 0x3F;
    retval[0] |= (static_cast<uint8_t>(m_orientation) & 0x03) << 6;

    retval[1] = m_y_pos >> 8;
    if ((m_y_pos & 0x80) == 0)
    {
        retval[1]--;
    }
    retval[1] &= 0x3F;
    retval[1] |= (m_palette & 0x03) << 6;

    retval[2] = m_speed & 0x07;
    retval[2] |= m_hostile ? 0x80 : 0x00;
    retval[2] |= m_pickupable ? 0x40 : 0x00;
    retval[2] |= m_has_dialogue ? 0x20 : 0x00;
    retval[2] |= m_no_rotate ? 0x10 : 0x00;
    retval[2] |= m_no_friction ? 0x08 : 0x00;

    retval[3] |= ((m_x_pos & 0x80) == 0) ? 0x80 : 0x00;
    retval[3] |= ((m_y_pos & 0x80) == 0) ? 0x40 : 0x00;
    retval[3] |= m_reserved ? 0x20 : 0x00;
    retval[3] |= m_copy_tiles ? 0x10 : 0x00;
    retval[3] |= m_copy_source & 0x0F;

    retval[4] |= (m_behaviour >> 8) & 0x03;
    retval[4] |= (m_dialogue << 2) & 0xFC;

    retval[5] = m_type;
    
    retval[6] = (m_z_pos >> 8) & 0x0F;
    retval[6] |= ((m_z_pos & 0x80) > 0) ? 0x40 : 0x00;
    retval[6] |= m_no_gravity ? 0x80 : 0x00;
    retval[6] |= m_invisible ? 0x20 : 0x00;
    retval[6] |= m_not_solid ? 0x10 : 0x00;

    retval[7] = m_behaviour & 0xFF;

    return retval;
}

std::string Entity::ToString() const
{
    return StrPrintf("[%02X] %-24s (%04X,%04X,%04X) %s\n   Pal:%01d Spd:%01d Dlg:%02d Bhv:%04d Cpy:%01X Flags:%c%c%c%c%c%c%c%c%c%c",
        m_type, GetTypeName().c_str(), m_x_pos, m_y_pos, m_z_pos, GetOrientationName().c_str(),
        m_palette, m_speed, m_dialogue, m_behaviour, m_copy_source,
        m_hostile ? 'H' : '-', m_pickupable ? 'P' : '-', m_has_dialogue ? 'D' : '-', m_no_rotate ? 'r' : '-',
        m_no_friction ? 'f' : '-', m_no_gravity ? 'g' : '-', m_invisible ? 'I' : '-', m_not_solid ? 's' : '-',
        m_copy_tiles ? 'T' : '-', m_reserved ? 'X' : '-');
}

uint8_t Entity::GetType() const
{
    return m_type;
}

bool Entity::SetType(uint8_t type)
{
    m_type = type;
    return true;
}

uint8_t Entity::GetPalette() const
{
    return m_palette;
}

bool Entity::SetPalette(uint8_t palette)
{
    if (palette > 3) return false;
    m_palette = palette;
    return true;
}

Orientation Entity::GetOrientation() const
{
    return m_orientation;
}

bool Entity::SetOrientation(Orientation orientation)
{
    m_orientation = orientation;
    return true;
}

uint16_t Entity::GetX() const
{
    return m_x_pos;
}

bool Entity::SetX(uint16_t x)
{
    if (x >= 0x80 && x <= 0x4000 && (x & 0x7F) == 0)
    {
        m_x_pos = x;
        return true;
    }
    return false;
}

double Entity::GetXDbl() const
{
    return CoordinateToDouble(m_x_pos);
}

bool Entity::SetXDbl(double x)
{
    return SetX(DoubleToCoordinate(x, false));
}

uint16_t Entity::GetY() const
{
    return m_y_pos;
}

bool Entity::SetY(uint16_t y)
{
    if (y >= 0x80 && y <= 0x4000 && (y & 0x7F) == 0)
    {
        m_y_pos = y;
        return true;
    }
    return false;
}

double Entity::GetYDbl() const
{
    return CoordinateToDouble(m_y_pos);
}

bool Entity::SetYDbl(double y)
{
    return SetY(DoubleToCoordinate(y, false));
}

uint16_t Entity::GetZ() const
{
    return m_z_pos;
}

bool Entity::SetZ(uint16_t z)
{
    if (z <= 0xF80 && (z & 0x7F) == 0)
    {
        m_z_pos = z;
        return true;
    }
    return false;
}

double Entity::GetZDbl() const
{
    return CoordinateToDouble(m_z_pos);
}

bool Entity::SetZDbl(double z)
{
    return SetZ(DoubleToCoordinate(z, true));
}

uint8_t Entity::GetSpeed() const
{
    return m_speed;
}

bool Entity::SetSpeed(uint8_t speed)
{
    if (speed < 8)
    {
        m_speed = speed;
        return true;
    }
    return false;
}

uint16_t Entity::GetBehaviour() const
{
    return m_behaviour;
}

bool Entity::SetBehaviour(uint16_t behaviour)
{
    if (behaviour < 1024)
    {
        m_behaviour = behaviour;
        return true;
    }
    return false;
}

uint8_t Entity::GetDialogue() const
{
    return m_dialogue;
}

bool Entity::SetDialogue(uint8_t dialogue)
{
    if (dialogue < 64)
    {
        m_dialogue = dialogue;
        return true;
    }
    return false;
}

uint8_t Entity::GetCopySource() const
{
    return m_copy_source;
}

bool Entity::SetCopySource(uint8_t src)
{
    if (src < 16)
    {
        m_copy_source = src;
        return true;
    }
    return false;
}

bool Entity::IsHostile() const
{
    return m_hostile;
}

void Entity::SetHostile(bool hostile)
{
    m_hostile = hostile;
}

bool Entity::NoRotate() const
{
    return m_no_rotate;
}

void Entity::SetNoRotate(bool no_rotate)
{
    m_no_rotate = no_rotate;
}

bool Entity::NoPickup() const
{
    return !m_pickupable;
}

void Entity::SetNoPickup(bool no_pickup)
{
    m_pickupable = !no_pickup;
}

bool Entity::HasDialogue() const
{
    return m_has_dialogue;
}

void Entity::SetHasDialogue(bool has_dialogue)
{
    m_has_dialogue = has_dialogue;
}

bool Entity::IsVisible() const
{
    return !m_invisible;
}

void Entity::SetVisible(bool visible)
{
    m_invisible = !visible;
}

bool Entity::IsSolid() const
{
    return !m_not_solid;
}

void Entity::SetSolid(bool solid)
{
    m_not_solid = !solid;
}

bool Entity::HasGravity() const
{
    return !m_no_gravity;
}

void Entity::SetGravity(bool gravity)
{
    m_no_gravity = !gravity;
}

bool Entity::HasFriction() const
{
    return !m_no_friction;
}

void Entity::SetFriction(bool friction)
{
    m_no_friction = !friction;
}

bool Entity::IsReservedSet() const
{
    return m_reserved;
}

void Entity::SetReserved(bool reserved)
{
    m_reserved = reserved;
}

bool Entity::IsTileCopySet() const
{
    return m_copy_tiles;
}

void Entity::SetTileCopy(bool tile_copy)
{
    m_copy_tiles = tile_copy;
}

double Entity::CoordinateToDouble(uint16_t coord) const
{
    return static_cast<double>(coord) / 256.0;
}

uint16_t Entity::DoubleToCoordinate(double coord, bool z) const
{
    int32_t tmp = static_cast<int32_t>(round(coord * 2.0));
    tmp <<= 7;
    if (z)
    {
        tmp = std::clamp(tmp, 0, 0x0F80);
        tmp &= 0x0F80;
    }
    else
    {
        tmp = std::clamp(tmp, 0x080, 0x4000);
        tmp &= 0x7F80;
    }
    return static_cast<uint16_t>(tmp);
}

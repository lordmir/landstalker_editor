#include <Entity.h>
#include <Utils.h>
#include <algorithm>
#include <cmath>

const std::vector<std::string> Entity::EntityNames =
{
    "Nigel",
    "Old Man",
    "Male Red Bear",
    "Boy",
    "Orc 1",
    "Orc 2",
    "Orc 3",
    "Worm 1",
    "Worm 2",
    "Worm 3",
    "Moneybag",
    "Man 1",
    "Woman 1",
    "Girl",
    "Dog",
    "Chicken",
    "Priest",
    "Vase",
    "Chest",
    "Small Grey Ball",
    "Large Grey Ball",
    "Small Yellow Platform",
    "Crate",
    "Ninja 1",
    "Ninja 2",
    "Ninja 3",
    "Lizard 1",
    "Lizard 2",
    "Lizard 3",
    "Knight 1",
    "Knight 2",
    "Knight 3",
    "Ghost 1",
    "Ghost 2",
    "Ghost 3",
    "Mummy 1",
    "Mummy 2",
    "Mummy 3",
    "Unicorn 1",
    "Unicorn 2",
    "Unicorn 3",
    "Skeleton 1",
    "Skeleton 2",
    "Skeleton 3",
    "Mimic 1",
    "Mimic 2",
    "Mimic 3",
    "Child Red Bear",
    "Male Dwarf",
    "Female Yellow Bear",
    "Soldier",
    "Madame Yard",
    "Dexter",
    "Raft",
    "Mushroom 1",
    "Mushroom 2",
    "Mushroom 3",
    "Giant 1",
    "Giant 2",
    "Giant 3",
    "Bubble 1",
    "Bubble 2",
    "Bubble 3",
    "Old Lady",
    "Male Yellow Bear",
    "Child Yellow Bear",
    "Fara stake",
    "Praying Yellow Bear",
    "Skeleton Statue 1",
    "Praying Red Bear",
    "Skeleton Statue 2",
    "Female Red Bear",
    "Female Dwarf",
    "Small Grey Spiked Ball",
    "Large Grey Spiked Ball",
    "Sacred Tree",
    "Large Wood Platform",
    "Injured Dog",
    "Dead Body",
    "Woman 2",
    "Man 2",
    "Invisible Cube",
    "Ink",
    "Large Concrete Block",
    "Square Switch",
    "Round Switch",
    "Unused Small Yellow Platform",
    "Unused Small Grey Platform",
    "Unused Small Green Platform",
    "Small Thin Yellow Platform",
    "Small Thin Grey Platform",
    "Unused Small Thin Green Platform",
    "Unused Large Dark Platform",
    "Large Thin Grey Platform",
    "Unused Large Thin Green Platform",
    "Unused Large Thin Yellow Platform",
    "Item Box",
    "Friday",
    "Large Boulder",
    "Nigel in Bed",
    "Godess Statue",
    "2-Way Signpost",
    "3-Way Signpost",
    "Gate S",
    "Gate N",
    "Kayla",
    "Wally",
    "Woman 3",
    "Locked Door",
    "Man in bed",
    "Lord Arthur",
    "Kayla in Bath",
    "Duke",
    "Small Wood Platform",
    "Small Thin Dark Grey Platform",
    "Yellow Old Man",
    "Man with Blue Shirt",
    "Chef",
    "Princess",
    "Golem Statue 1",
    "Mir Magic Barrier",
    "Sailor",
    "Pockets",
    "Skeleton Priest",
    "Mir",
    "Unused Bird 1",
    "Unused Bird 2",
    "Unused Bird 3",
    "Moralis",
    "Bubble 4",
    "Bubble 5",
    "Bubble 6",
    "Gnome",
    "Zak",
    "Mir Wall Barrier",
    "Small Fireball",
    "Reaper 1",
    "Reaper 2",
    "Unused Reaper 3",
    "Golem Statue 2",
    "Corrupted Round Switch",
    "Small Green Spiked Ball",
    "Large Grey Spiked Ball",
    "Ghost Generator 1",
    "Ghost Generator 2",
    "Ghost Generator 3",
    "Golem 1",
    "Golem 2",
    "Golem 3",
    "Spectre 1",
    "Spectre 2",
    "Spectre 3",
    "Unused Small Blue Ball",
    "Small Red Ball",
    "Unused Large Blue Ball",
    "Large Red Ball",
    "Unused Magic Fox",
    "Spinner 1",
    "Duke Chair",
    "Miro",
    "Ifrit",
    "Ifrit Fireball",
    "Gola",
    "Pink Golem Statue",
    "Gold Golem Statue",
    "Nole",
    "Nole Axe Projectile",
    "Stone Warrior 1",
    "Zak Boomerang",
    "1-Way Signpost",
    "Spinner 2",
    "Stone Warrior 2",
    "Gola Fireball",
    "?AD",
    "?AE",
    "?AF",
    "?B0",
    "?B1",
    "?B2",
    "?B3",
    "?B4",
    "?B5",
    "?B6",
    "?B7",
    "?B8",
    "?B9",
    "?BA",
    "?BB",
    "?BC",
    "?BD",
    "?BE",
    "?BF",
    "EkeEke",
    "Magic Sword",
    "Ice Sword",
    "Thunder Sword",
    "Gaia Sword",
    "Fireproof",
    "Iron Boots",
    "Healing Boots",
    "Snow Spikes",
    "Steel Breast",
    "Chrome Breast",
    "Shell Breast",
    "Hyper Breast",
    "Mars Stone",
    "Moon Stone",
    "Saturn Stone",
    "Venus Stone",
    "Awakening Book",
    "Detox Grass",
    "Gaia Statue",
    "Golden Statue",
    "Mind Repair",
    "Casino Ticket",
    "Axe Magic",
    "Blue Ribbon",
    "Buyer Card",
    "Lantern",
    "Garlic",
    "Anti-Paralyze",
    "Statue of Jypta",
    "Sun Stone",
    "Armlet",
    "Einstein Whistle",
    "Detox Book",
    "AntiCurse",
    "Record Book",
    "Spell Book",
    "Hotel Register",
    "Island Map",
    "Lithograph",
    "Red Jewel",
    "Pawn Ticket",
    "Purple Jewel",
    "Gola's Eye",
    "Death Statue",
    "Dahl",
    "Restoration",
    "Logs",
    "Oracle Stone",
    "Idol Stone",
    "Key",
    "Safety Pass",
    "No52",
    "Bell",
    "Shortcake",
    "Gola's Nail",
    "Gola's Horn",
    "Gola's Fang",
    "Broad Sword",
    "Leather Breast",
    "Leather Boots",
    "None",
    "Lifestock",
    "No63"
};

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
    : m_type(0),
    m_x_pos(0x1F80),
    m_y_pos(0x1F80),
    m_z_pos(0x000),
    m_speed(0),
    m_orientation(Orientation::SE),
    m_palette(2),
    m_behaviour(0),
    m_dialogue(0),
    m_no_rotate(false),
    m_hostile(false),
    m_pickupable(false),
    m_has_dialogue(false),
    m_invisible(false),
    m_not_solid(false),
    m_no_gravity(false),
    m_no_friction(false),
    m_reserved(false),
    m_copy_tiles(false),
    m_copy_source(0)
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
    if (z >= 0x000 && z <= 0xF80 && (z & 0x7F) == 0)
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

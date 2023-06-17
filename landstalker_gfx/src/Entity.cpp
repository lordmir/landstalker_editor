#include <Entity.h>
#include <Utils.h>
#include <algorithm>
#include <cmath>

const std::unordered_map<uint8_t, std::string> Entity::EntityNames =
{
    {0x00, "Nigel"},
    {0x01, "Old Man"},
    {0x02, "Male Red Bear"},
    {0x03, "Boy"},
    {0x04, "Orc 1"},
    {0x05, "Orc 2"},
    {0x06, "Orc 3"},
    {0x07, "Worm 1"},
    {0x08, "Worm 2"},
    {0x09, "Worm 3"},
    {0x0A, "Moneybag"},
    {0x0B, "Man 1"},
    {0x0C, "Woman 1"},
    {0x0D, "Girl"},
    {0x0E, "Dog"},
    {0x0F, "Chicken"},
    {0x10, "Priest"},
    {0x11, "Vase"},
    {0x12, "Chest"},
    {0x13, "Small Grey Ball"},
    {0x14, "Large Grey Ball"},
    {0x15, "Small Yellow Platform"},
    {0x16, "Crate"},
    {0x17, "Ninja 1"},
    {0x18, "Ninja 2"},
    {0x19, "Ninja 3"},
    {0x1A, "Lizard 1"},
    {0x1B, "Lizard 2"},
    {0x1C, "Lizard 3"},
    {0x1D, "Knight 1"},
    {0x1E, "Knight 2"},
    {0x1F, "Knight 3"},
    {0x20, "Ghost 1"},
    {0x21, "Ghost 2"},
    {0x22, "Ghost 3"},
    {0x23, "Mummy 1"},
    {0x24, "Mummy 2"},
    {0x25, "Mummy 3"},
    {0x26, "Unicorn 1"},
    {0x27, "Unicorn 2"},
    {0x28, "Unicorn 3"},
    {0x29, "Skeleton 1"},
    {0x2A, "Skeleton 2"},
    {0x2B, "Skeleton 3"},
    {0x2C, "Mimic 1"},
    {0x2D, "Mimic 2"},
    {0x2E, "Mimic 3"},
    {0x2F, "Child Red Bear"},
    {0x30, "Male Dwarf"},
    {0x31, "Female Yellow Bear"},
    {0x32, "Soldier"},
    {0x33, "Madame Yard"},
    {0x34, "Dexter"},
    {0x35, "Raft"},
    {0x36, "Mushroom 1"},
    {0x37, "Mushroom 2"},
    {0x38, "Mushroom 3"},
    {0x39, "Giant 1"},
    {0x3A, "Giant 2"},
    {0x3B, "Giant 3"},
    {0x3C, "Bubble 1"},
    {0x3D, "Bubble 2"},
    {0x3E, "Bubble 3"},
    {0x3F, "Old Lady"},
    {0x40, "Male Yellow Bear"},
    {0x41, "Child Yellow Bear"},
    {0x42, "Fara stake"},
    {0x43, "Praying Yellow Bear"},
    {0x44, "Skeleton Statue 1"},
    {0x45, "Praying Red Bear"},
    {0x46, "Skeleton Statue 2"},
    {0x47, "Female Red Bear"},
    {0x48, "Female Dwarf"},
    {0x49, "Small Grey Spiked Ball"},
    {0x4A, "Large Grey Spiked Ball"},
    {0x4B, "Sacred Tree"},
    {0x4C, "Large Wood Platform"},
    {0x4D, "Injured Dog"},
    {0x4E, "Dead Body"},
    {0x4F, "Woman 2"},
    {0x50, "Man 2"},
    {0x51, "Invisible Cube"},
    {0x52, "Ink"},
    {0x53, "Large Concrete Block"},
    {0x54, "Square Switch"},
    {0x55, "Round Switch"},
    {0x56, "Unused Small Yellow Platform"},
    {0x57, "Unused Small Grey Platform"},
    {0x58, "Unused Small Green Platform"},
    {0x59, "Small Thin Yellow Platform"},
    {0x5A, "Small Thin Grey Platform"},
    {0x5B, "Unused Small Thin Green Platform"},
    {0x5C, "Unused Large Dark Platform"},
    {0x5D, "Large Thin Grey Platform"},
    {0x5E, "Unused Large Thin Green Platform"},
    {0x5F, "Unused Large Thin Yellow Platform"},
    {0x60, "Item Box"},
    {0x61, "Friday"},
    {0x62, "Large Boulder"},
    {0x63, "Nigel in Bed"},
    {0x64, "Godess Statue"},
    {0x65, "2-Way Signpost"},
    {0x66, "3-Way Signpost"},
    {0x67, "Gate S"},
    {0x68, "Gate N"},
    {0x69, "Kayla"},
    {0x6A, "Wally"},
    {0x6B, "Woman 3"},
    {0x6C, "Locked Door"},
    {0x6D, "Man in bed"},
    {0x6E, "Lord Arthur"},
    {0x6F, "Kayla in Bath"},
    {0x70, "Duke"},
    {0x71, "Small Wood Platform"},
    {0x72, "Small Thin Dark Grey Platform"},
    {0x73, "Yellow Old Man"},
    {0x74, "Man with Blue Shirt"},
    {0x75, "Chef"},
    {0x76, "Princess"},
    {0x77, "Golem Statue 1"},
    {0x78, "Mir Magic Barrier"},
    {0x79, "Sailor"},
    {0x7A, "Pockets"},
    {0x7B, "Skeleton Priest"},
    {0x7C, "Mir"},
    {0x7D, "Unused Bird 1"},
    {0x7E, "Unused Bird 2"},
    {0x7F, "Unused Bird 3"},
    {0x80, "Moralis"},
    {0x81, "Bubble 4"},
    {0x82, "Bubble 5"},
    {0x83, "Bubble 6"},
    {0x84, "Gnome"},
    {0x85, "Zak"},
    {0x86, "Mir Wall Barrier"},
    {0x87, "Small Fireball"},
    {0x88, "Reaper 1"},
    {0x89, "Reaper 2"},
    {0x8A, "Unused Reaper 3"},
    {0x8B, "Golem Statue 2"},
    {0x8C, "Corrupted Round Switch"},
    {0x8D, "Small Green Spiked Ball"},
    {0x8E, "Large Grey Spiked Ball"},
    {0x8F, "Ghost Generator 1"},
    {0x90, "Ghost Generator 2"},
    {0x91, "Ghost Generator 3"},
    {0x92, "Golem 1"},
    {0x93, "Golem 2"},
    {0x94, "Golem 3"},
    {0x95, "Spectre 1"},
    {0x96, "Spectre 2"},
    {0x97, "Spectre 3"},
    {0x98, "Unused Small Blue Ball"},
    {0x99, "Small Red Ball"},
    {0x9A, "Unused Large Blue Ball"},
    {0x9B, "Large Red Ball"},
    {0x9C, "Unused Magic Fox"},
    {0x9D, "Spinner 1"},
    {0x9E, "Duke Chair"},
    {0x9F, "Miro"},
    {0xA0, "Ifrit"},
    {0xA1, "Ifrit Fireball"},
    {0xA2, "Gola"},
    {0xA3, "Pink Golem Statue"},
    {0xA4, "Gold Golem Statue"},
    {0xA5, "Nole"},
    {0xA6, "Nole Axe Projectile"},
    {0xA7, "Stone Warrior 1"},
    {0xA8, "Zak Boomerang"},
    {0xA9, "1-Way Signpost"},
    {0xAA, "Spinner 2"},
    {0xAB, "Stone Warrior 2"},
    {0xAC, "Gola Fireball"},
    {0xAD, "?AD"},
    {0xAE, "?AE"},
    {0xAF, "?AF"},
    {0xB0, "?B0"},
    {0xB1, "?B1"},
    {0xB2, "?B2"},
    {0xB3, "?B3"},
    {0xB4, "?B4"},
    {0xB5, "?B5"},
    {0xB6, "?B6"},
    {0xB7, "?B7"},
    {0xB8, "?B8"},
    {0xB9, "?B9"},
    {0xBA, "?BA"},
    {0xBB, "?BB"},
    {0xBC, "?BC"},
    {0xBD, "?BD"},
    {0xBE, "?BE"},
    {0xBF, "?BF"},
    {0xC0, "EkeEke"},
    {0xC1, "Magic Sword"},
    {0xC2, "Ice Sword"},
    {0xC3, "Thunder Sword"},
    {0xC4, "Gaia Sword"},
    {0xC5, "Fireproof"},
    {0xC6, "Iron Boots"},
    {0xC7, "Healing Boots"},
    {0xC8, "Snow Spikes"},
    {0xC9, "Steel Breast"},
    {0xCA, "Chrome Breast"},
    {0xCB, "Shell Breast"},
    {0xCC, "Hyper Breast"},
    {0xCD, "Mars Stone"},
    {0xCE, "Moon Stone"},
    {0xCF, "Saturn Stone"},
    {0xD0, "Venus Stone"},
    {0xD1, "Awakening Book"},
    {0xD2, "Detox Grass"},
    {0xD3, "Gaia Statue"},
    {0xD4, "Golden Statue"},
    {0xD5, "Mind Repair"},
    {0xD6, "Casino Ticket"},
    {0xD7, "Axe Magic"},
    {0xD8, "Blue Ribbon"},
    {0xD9, "Buyer Card"},
    {0xDA, "Lantern"},
    {0xDB, "Garlic"},
    {0xDC, "Anti-Paralyze"},
    {0xDD, "Statue of Jypta"},
    {0xDE, "Sun Stone"},
    {0xDF, "Armlet"},
    {0xE0, "Einstein Whistle"},
    {0xE1, "Detox Book"},
    {0xE2, "AntiCurse"},
    {0xE3, "Record Book"},
    {0xE4, "Spell Book"},
    {0xE5, "Hotel Register"},
    {0xE6, "Island Map"},
    {0xE7, "Lithograph"},
    {0xE8, "Red Jewel"},
    {0xE9, "Pawn Ticket"},
    {0xEA, "Purple Jewel"},
    {0xEB, "Gola's Eye"},
    {0xEC, "Death Statue"},
    {0xED, "Dahl"},
    {0xEE, "Restoration"},
    {0xEF, "Logs"},
    {0xF0, "Oracle Stone"},
    {0xF1, "Idol Stone"},
    {0xF2, "Key"},
    {0xF3, "Safety Pass"},
    {0xF4, "No52"},
    {0xF5, "Bell"},
    {0xF6, "Shortcake"},
    {0xF7, "Gola's Nail"},
    {0xF8, "Gola's Horn"},
    {0xF9, "Gola's Fang"},
    {0xFA, "Broad Sword"},
    {0xFB, "Leather Breast"},
    {0xFC, "Leather Boots"},
    {0xFD, "None"},
    {0xFE, "Lifestock"},
    {0xFF, "No63"}
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

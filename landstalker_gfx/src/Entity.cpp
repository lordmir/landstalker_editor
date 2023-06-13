#include <Entity.h>
#include <Utils.h>

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
    x_pos = (data[0] & 0x3F) << 8;
    y_pos = (data[1] & 0x3F) << 8;
    orientation = static_cast<Orientation>((data[0] >> 6) & 3);
    palette = (data[1] >> 6) & 0x03;

    speed = data[2] & 0x07;
    hostile = (data[2] & 0x80) > 0;
    pickupable = (data[2] & 0x40) > 0;
    has_dialogue = (data[2] & 0x20) > 0;
    no_rotate = (data[2] & 0x10) > 0;
    no_friction = (data[2] & 0x08) > 0;

    x_pos += (data[3] & 0x80) > 0 ? 0x100 : 0x80;
    y_pos += (data[3] & 0x40) > 0 ? 0x100 : 0x80;
    reserved = (data[3] & 0x20) > 0;
    copy_tiles = (data[3] & 0x10) > 0;
    copy_source = data[3] & 0x0F;

    behaviour = ((data[4] & 0x03) << 8) | data[7];
    dialogue = (data[4] >> 2) & 0x3F;

    type = data[5];

    z_pos = (data[6] & 0x0F) << 8;
    z_pos += (data[6] & 0x40) > 0 ? 0x80 : 0x00;
    no_gravity = (data[6] & 0x80) > 0;
    invisible = (data[6] & 0x20) > 0;
    not_solid = (data[6] & 0x10) > 0;
}

Entity::Entity()
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

    retval[0] = (x_pos > 8);
    if ((x_pos & 0x80) == 0)
    {
        retval[0]--;
    }
    retval[0] &= 0x3F;
    retval[0] |= (static_cast<uint8_t>(orientation) & 0x03) << 6;

    retval[1] = (y_pos > 8);
    if ((y_pos & 0x80) == 0)
    {
        retval[1]--;
    }
    retval[1] &= 0x3F;
    retval[1] |= (palette & 0x03) << 6;

    retval[2] = speed & 0x07;
    retval[2] |= hostile ? 0x80 : 0x00;
    retval[2] |= pickupable ? 0x40 : 0x00;
    retval[2] |= has_dialogue ? 0x20 : 0x00;
    retval[2] |= no_rotate ? 0x10 : 0x00;
    retval[2] |= no_friction ? 0x08 : 0x00;

    retval[3] |= ((x_pos & 0x80) == 0) ? 0x80 : 0x00;
    retval[3] |= ((y_pos & 0x80) == 0) ? 0x40 : 0x00;
    retval[3] |= reserved ? 0x20 : 0x00;
    retval[3] |= copy_tiles ? 0x10 : 0x00;
    retval[3] |= copy_source & 0x0F;

    retval[4] |= (behaviour >> 8) & 0x03;
    retval[4] |= (dialogue << 2) & 0xFC;

    retval[5] = type;
    
    retval[6] = (z_pos >> 8) & 0x0F;
    retval[6] |= ((z_pos & 0x80) > 0) ? 0x40 : 0x00;
    retval[6] |= no_gravity ? 0x80 : 0x00;
    retval[6] |= invisible ? 0x20 : 0x00;
    retval[6] |= not_solid ? 0x10 : 0x00;

    retval[7] = behaviour & 0xFF;

    return retval;
}

std::string Entity::ToString() const
{
    return StrPrintf("[%02X] %-24s (%04X,%04X,%04X) %s\n   Pal:%01d Spd:%01d Dlg:%02d Bhv:%04d Cpy:%01X Flags:%c%c%c%c%c%c%c%c%c%c",
        type, GetTypeName().c_str(), x_pos, y_pos, z_pos, GetOrientationName().c_str(), palette, speed,
        dialogue, behaviour, copy_source,
        hostile ? 'H' : '-', pickupable ? 'P' : '-', has_dialogue ? 'D' : '-', no_rotate ? 'r' : '-',
        no_friction ? 'f' : '-', no_gravity ? 'g' : '-', invisible ? 'I' : '-', not_solid ? 's' : '-',
        copy_tiles ? 'T' : '-', reserved ? 'X' : '-');
}

uint8_t Entity::GetType() const
{
    return type;
}

uint8_t Entity::GetPalette() const
{
    return palette;
}

Orientation Entity::GetOrientation() const
{
    return orientation;
}

uint16_t Entity::GetX() const
{
    return x_pos;
}

uint16_t Entity::GetY() const
{
    return y_pos;
}

uint16_t Entity::GetZ() const
{
    return z_pos;
}

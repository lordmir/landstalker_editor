#ifndef ENTITY_H
#define ENTITY_H

#include <map>
#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include <landstalker/misc/include/Labels.h>

enum class Orientation
{
    NE = 0,
    SE = 1,
    SW = 2,
    NW = 3
};

class Entity
{
public:

    Entity(std::array<uint8_t, 8> data);
    Entity();

    bool operator==(const Entity& rhs) const;
    bool operator!=(const Entity& rhs) const;

    std::array<uint8_t, 8> GetData() const;

    std::string ToString() const;

    uint8_t GetType() const;
    bool SetType(uint8_t type);
    uint8_t GetPalette() const;
    bool SetPalette(uint8_t palette);
    Orientation GetOrientation() const;
    bool SetOrientation(Orientation orientation);
    uint16_t GetX() const;
    bool SetX(uint16_t x);
    double GetXDbl() const;
    bool SetXDbl(double x);
    uint16_t GetY() const;
    bool SetY(uint16_t y);
    double GetYDbl() const;
    bool SetYDbl(double y);
    uint16_t GetZ() const;
    bool SetZ(uint16_t z);
    double GetZDbl() const;
    bool SetZDbl(double z);
    uint8_t GetSpeed() const;
    bool SetSpeed(uint8_t speed);
    uint16_t GetBehaviour() const;
    bool SetBehaviour(uint16_t behaviour);
    uint8_t GetDialogue() const;
    bool SetDialogue(uint8_t dialogue);
    uint8_t GetCopySource() const;
    bool SetCopySource(uint8_t src);
    bool IsHostile() const;
    void SetHostile(bool hostile);
    bool NoRotate() const;
    void SetNoRotate(bool no_rotate);
    bool NoPickup() const;
    void SetNoPickup(bool no_pickup);
    bool HasDialogue() const;
    void SetHasDialogue(bool has_dialogue);
    bool IsVisible() const;
    void SetVisible(bool visible);
    bool IsSolid() const;
    void SetSolid(bool solid);
    bool HasGravity() const;
    void SetGravity(bool gravity);
    bool HasFriction() const;
    void SetFriction(bool friction);
    bool IsReservedSet() const;
    void SetReserved(bool reserved);
    bool IsTileCopySet() const;
    void SetTileCopy(bool tile_copy);

    std::wstring GetTypeName() const
    {
        return Labels::Get(Labels::C_ENTITIES, m_type).value_or(L"Entity" + std::to_wstring(m_type));
    }

    std::string GetOrientationName() const
    {
        std::map<Orientation, std::string> OrientationToString
        {
            {Orientation::NE, "NE"},
            {Orientation::SE, "SE"},
            {Orientation::SW, "SW"},
            {Orientation::NW, "NW"}
        };
        return OrientationToString.find(m_orientation)->second;
    }
private:
    double CoordinateToDouble(uint16_t coord) const;
    uint16_t DoubleToCoordinate(double coord, bool z) const;

    uint16_t m_x_pos;
    uint16_t m_y_pos;
    uint16_t m_z_pos;
    uint8_t m_type;

    Orientation m_orientation;
    uint8_t m_palette;
    uint8_t m_speed;
    uint16_t m_behaviour;
    uint8_t m_dialogue;

    bool m_copy_tiles;
    uint8_t m_copy_source;

    bool m_hostile;
    bool m_pickupable;
    bool m_has_dialogue;
    bool m_no_rotate;
    bool m_no_friction;
    bool m_no_gravity;
    bool m_invisible;
    bool m_not_solid;
    bool m_reserved;
};


#endif // ENTITY_H

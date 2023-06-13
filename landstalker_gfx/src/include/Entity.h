#ifndef ENTITY_H
#define ENTITY_H

#include <unordered_map>
#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include <memory>


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

    bool operator==(const Entity& rhs) const;
    bool operator!=(const Entity& rhs) const;

    std::array<uint8_t, 8> GetData() const;

    std::string ToString() const;

    uint8_t GetType() const;
    uint8_t GetPalette() const;
    Orientation GetOrientation() const;
    uint16_t GetX() const;
    uint16_t GetY() const;
    uint16_t GetZ() const;

    std::string GetTypeName() const
    {
        return EntityNames.find(type)->second;
    }

    std::string GetOrientationName() const
    {
        std::unordered_map<Orientation, std::string> OrientationToString
        {
            {Orientation::NE, "NE"},
            {Orientation::SE, "SE"},
            {Orientation::SW, "SW"},
            {Orientation::NW, "NW"}
        };
        return OrientationToString.find(orientation)->second;
    }

    static const std::unordered_map<uint8_t, std::string> EntityNames;
private:
    uint16_t x_pos;
    uint16_t y_pos;
    uint16_t z_pos;
    uint8_t type;

    Orientation orientation;
    uint8_t palette;
    uint8_t speed;
    uint16_t behaviour;
    uint8_t dialogue;

    bool copy_tiles;
    uint8_t copy_source;

    bool hostile;
    bool pickupable;
    bool has_dialogue;
    bool no_rotate;
    bool no_friction;
    bool no_gravity;
    bool invisible;
    bool not_solid;
    bool reserved;
};


#endif // ENTITY_H

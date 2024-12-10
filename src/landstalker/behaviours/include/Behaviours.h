#ifndef _BEHAVIOURS_H_
#define _BEHAVIOURS_H_

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <variant>
#include <tuple>

class Behaviours
{
public:
    enum class CommandType : uint8_t
    {
        PAUSE                      = 0,
        MOVE_TIMED                 = 1,
        TURN_CW                    = 2,
        TURN_CCW                   = 3,
        TURN_NE                    = 4,
        TURN_SE                    = 5,
        TURN_SW                    = 6,
        TURN_NW                    = 7,
        TURN_CW_NO_UPDATE          = 8,
        TURN_CCW_NO_UPDATE         = 9,
        TURN_NE_NO_UPDATE          = 10,
        TURN_SE_NO_UPDATE          = 11,
        TURN_SW_NO_UPDATE          = 12,
        TURN_NW_NO_UPDATE          = 13,
        MAKE_VISIBLE               = 14,
        MAKE_INVISIBLE             = 15,
        UNKNOWN_B10                = 16,
        MOVE_RELATIVE              = 17,
        GOTO_COMMAND               = 18,
        MOVE_UNTIL_COLLISION       = 19,
        TURN_RANDOM                = 20,
        TURN_RANDOM_NO_UPDATE      = 21,
        UNKNOWN_B16                = 22,
        TURN_RANDOM_IMMEDIATE      = 23,
        TURN_CW_IMMEDIATE          = 24,
        TURN_CCW_IMMEDIATE         = 25,
        TURN_NE_IMMEDIATE          = 26,
        TURN_SE_IMMEDIATE          = 27,
        TURN_SW_IMMEDIATE          = 28,
        TURN_NW_IMMEDIATE          = 29,
        FREEZE                     = 30,
        SPEED_SLOW                 = 31,
        SPEED_NORMAL               = 32,
        SPEED_FAST                 = 33,
        SPEED_X_FAST               = 34,
        TURN_180                   = 35,
        TURN_180_NO_UPDATE         = 36,
        TURN_180_IMMEDIATE         = 37,
        FOLLOW_PLAYER_WITH_JUMP    = 38,
        JUMP                       = 39,
        ENABLE_FRAME_UPDATE        = 40,
        DISABLE_FRAME_UPDATE       = 41,
        MOVE_RANDOM_TIMED          = 42,
        LOAD_SPECIAL_AI            = 43,
        FOLLOW_PLAYER_NO_JUMP      = 44,
        UNKNOWN_B2D                = 45,
        SHOP_ITEM                  = 46,
        UNKOWN_B2F                 = 47,
        MOVE_UP_RELATIVE           = 48,
        MOVE_DOWN_RELATIVE         = 49,
        UNKNOWN_B32                = 50,
        UNKNOWN_B33                = 51,
        PLAYBACK_INPUT             = 52,
        RESET_PLAYBACK             = 53,
        WAIT_FOR_CONDITION         = 54,
        PAUSE_4_SECONDS            = 55,
        ENABLE_GRAVITY             = 56,
        DISABLE_GRAVITY            = 57,
        MOVE_UP_TIMED              = 58,
        MOVE_DOWN_TIMED            = 59,
        MOVE_UP_ABSOLUTE           = 60,
        MOVE_DOWN_ABSOLUTE         = 61,
        REMOVE_SPRITE              = 62,
        NUDGE_UP                   = 63,
        MOVE_DOWN_UNTIL_COLLISION  = 64,
        SET_FLAG                   = 65,
        WAIT_FOR_FLAG_SET          = 66,
        CLEAR_FLAG                 = 67,
        HIDE                       = 68,
        SHOW_WHEN_COLLISION_CLEAR  = 69,
        WAIT_FOR_FLAG_CLEAR        = 70,
        UNKNOWN_B47                = 71,
        SET_OBJECT_SPEED           = 72,
        ACTIVATE_SWITCH            = 73,
        RESET_SWITCH               = 74,
        MOVE_TO_XY_POS_IMMEDIATE   = 75,
        MOVE_TO_Z_POS_IMMEDIATE    = 76,
        RESET_TO_INITIAL_POS       = 77,
        START_CUTSCENE             = 78,
        MOVE_NO_CLIP               = 79,
        ROTATE_PLAYER              = 80,
        MAKE_HOSTILE               = 81,
        MAKE_NON_HOSTILE           = 82,
        ENABLE_BACKWARDS_MOVEMENT  = 83,
        DISABLE_BACKWARDS_MOVEMENT = 84,
        SPECIAL_ANIMATION          = 85,
        MOVE_UP_TO_INIT_POS        = 86,
        TRIGGER_TILE_SWAP          = 87,
        UPDATE_SPRITE_ORIENTATION  = 88,
        PRINT_TEXT                 = 89,
        UNKNOWN_5A                 = 90,
        SET_TARGET_POSITION        = 91,
        MOVE_TO_TARGET_POSITION    = 92,
        REPEAT_BEGIN               = 93,
        REPEAT_END                 = 94,
        DECAY_FLASH                = 95,
        FLASH_SPIN_APPEAR          = 96,
        FLASH_SPIN_DISAPPEAR       = 97,
        PLAY_SOUND                 = 98,
        UNKNOWN_B63                = 99,
        START_HI_CUTSCENE          = 100,
        UNKNOWN_B65                = 101,
        UNKNOWN_B66                = 102,
        UNKNOWN_B67                = 103,
        NULL_COMMAND               = 104
    };

    enum class ParamType
    {
        NONE,
        UINT8,
        INT8,
        UINT16,
        LABEL,
        COORDINATE,
        LONG_COORDINATE,
        FLAG,
        SOUND,
        LOW_CUTSCENE,
        HIGH_CUTSCENE
    };

    using ParameterValue = std::variant<int, double>;
    using Parameter = std::tuple<std::string, ParameterValue, ParamType>;

    struct CommandDefinition
    {
        CommandType id;
        std::vector<std::string> aliases;
        std::vector<std::pair<std::string, ParamType>> params;
    };

    struct Command
    {
        CommandType command = CommandType::PAUSE;
        std::vector<Parameter> params;

        bool operator==(const Command& rhs) const
        {
            return (this->command == rhs.command && this->params == rhs.params);
        }
        bool operator!=(const Command& rhs) const
        {
            return !(*this == rhs);
        }
    };

    static const CommandDefinition& GetCommand(CommandType cmd);
    static const CommandDefinition& GetCommandById(int id);
    static const CommandDefinition& GetCommandByName(const std::string& name);

    static std::map<int, std::pair<std::string, std::vector<Command>>> Unpack(const std::vector<uint8_t>& offsets, const std::vector<uint8_t>& behaviour_table);
    static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Pack(const std::map<int, std::pair<std::string, std::vector<Command>>>& behaviours);
private:
    static const std::unordered_map<ParamType, int> PARAM_SIZES;
    static const std::unordered_map<CommandType, CommandDefinition> COMMANDS_BY_ID;
    static std::unordered_map<std::string, CommandType> commands_by_name;
}; // namespace Behaviours

#endif // _BEHAVIOURS_H_

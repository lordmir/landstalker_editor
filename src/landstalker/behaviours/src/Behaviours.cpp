#include <landstalker/behaviours/include/Behaviours.h>

#include <sstream>
#include <variant>
#include <yaml-cpp/yaml.h>
#include <cassert>
#include <numeric>

const std::unordered_map<Behaviours::ParamType, int> Behaviours::PARAM_SIZES =
{
    {ParamType::UINT8,           1},
    {ParamType::INT8,            1},
    {ParamType::SOUND,           1},
    {ParamType::LOW_CUTSCENE,    1},
    {ParamType::HIGH_CUTSCENE,   1},
    {ParamType::UINT16,          2},
    {ParamType::LABEL,           1},
    {ParamType::COORDINATE,      1},
    {ParamType::LONG_COORDINATE, 2},
    {ParamType::FLAG,            2}
};

const std::unordered_map<Behaviours::CommandType, Behaviours::CommandDefinition> Behaviours::COMMANDS_BY_ID =
{
    { CommandType::PAUSE                     , {  CommandType::PAUSE                     , {"Pause"},                    {{"Ticks",  ParamType::UINT8}} }},
    { CommandType::MOVE_TIMED                , {  CommandType::MOVE_TIMED                , {"MoveTimed"},                {{"Ticks",  ParamType::UINT8}} }},
    { CommandType::TURN_CW                   , {  CommandType::TURN_CW                   , {"TurnCW"},                   {} }},
    { CommandType::TURN_CCW                  , {  CommandType::TURN_CCW                  , {"TurnCCW"},                  {} }},
    { CommandType::TURN_NE                   , {  CommandType::TURN_NE                   , {"TurnNE"},                   {} }},
    { CommandType::TURN_SE                   , {  CommandType::TURN_SE                   , {"TurnSE"},                   {} }},
    { CommandType::TURN_SW                   , {  CommandType::TURN_SW                   , {"TurnSW"},                   {} }},
    { CommandType::TURN_NW                   , {  CommandType::TURN_NW                   , {"TurnNW"},                   {} }},
    { CommandType::TURN_CW_NO_UPDATE         , {  CommandType::TURN_CW_NO_UPDATE         , {"TurnCWNoUpdate"},           {} }},
    { CommandType::TURN_CCW_NO_UPDATE        , {  CommandType::TURN_CCW_NO_UPDATE        , {"TurnCCWNoUpdate"},          {} }},
    { CommandType::TURN_NE_NO_UPDATE         , {  CommandType::TURN_NE_NO_UPDATE         , {"TurnNENoUpdate"},           {} }},
    { CommandType::TURN_SE_NO_UPDATE         , {  CommandType::TURN_SE_NO_UPDATE         , {"TurnSENoUpdate"},           {} }},
    { CommandType::TURN_SW_NO_UPDATE         , {  CommandType::TURN_SW_NO_UPDATE         , {"TurnSWNoUpdate"},           {} }},
    { CommandType::TURN_NW_NO_UPDATE         , {  CommandType::TURN_NW_NO_UPDATE         , {"TurnNWNoUpdate"},           {} }},
    { CommandType::MAKE_VISIBLE              , {  CommandType::MAKE_VISIBLE              , {"MakeVisible"},              {} }},
    { CommandType::MAKE_INVISIBLE            , {  CommandType::MAKE_INVISIBLE            , {"MakeInvisible"},            {} }},
    { CommandType::UNKNOWN_B10               , {  CommandType::UNKNOWN_B10               , {"UnknownB10"},               {{"Unknown",  ParamType::UINT8}} }},
    { CommandType::MOVE_RELATIVE             , {  CommandType::MOVE_RELATIVE             , {"MoveRelative"},             {{"Distance", ParamType::COORDINATE}} }},
    { CommandType::GOTO_COMMAND              , {  CommandType::GOTO_COMMAND              , {"GotoCommand"},              {{"Command",  ParamType::LABEL}} }},
    { CommandType::MOVE_UNTIL_COLLISION      , {  CommandType::MOVE_UNTIL_COLLISION      , {"MoveUntilCollision"},       {} }},
    { CommandType::TURN_RANDOM               , {  CommandType::TURN_RANDOM               , {"TurnRandom"},               {} }},
    { CommandType::TURN_RANDOM_NO_UPDATE     , {  CommandType::TURN_RANDOM_NO_UPDATE     , {"TurnRandomNoUpdate"},       {} }},
    { CommandType::UNKNOWN_B16               , {  CommandType::UNKNOWN_B16               , {"UnknownB16"},               {{"Unknown", ParamType::UINT8}} }},
    { CommandType::TURN_RANDOM_IMMEDIATE     , {  CommandType::TURN_RANDOM_IMMEDIATE     , {"TurnRandomImmediate"},      {} }},
    { CommandType::TURN_CW_IMMEDIATE         , {  CommandType::TURN_CW_IMMEDIATE         , {"TurnCWImmediate"},          {} }},
    { CommandType::TURN_CCW_IMMEDIATE        , {  CommandType::TURN_CCW_IMMEDIATE        , {"TurnCCWImmedate"},          {} }},
    { CommandType::TURN_NE_IMMEDIATE         , {  CommandType::TURN_NE_IMMEDIATE         , {"TurnNEImmediate"},          {} }},
    { CommandType::TURN_SE_IMMEDIATE         , {  CommandType::TURN_SE_IMMEDIATE         , {"TurnSEImmediate"},          {} }},
    { CommandType::TURN_SW_IMMEDIATE         , {  CommandType::TURN_SW_IMMEDIATE         , {"TurnSWImmediate"},          {} }},
    { CommandType::TURN_NW_IMMEDIATE         , {  CommandType::TURN_NW_IMMEDIATE         , {"TurnNWImmediate"},          {} }},
    { CommandType::FREEZE                    , {  CommandType::FREEZE                    , {"Freeze"},                   {} }},
    { CommandType::SPEED_SLOW                , {  CommandType::SPEED_SLOW                , {"SlowSpeed"},                {} }},
    { CommandType::SPEED_NORMAL              , {  CommandType::SPEED_NORMAL              , {"NormalSpeed"},              {} }},
    { CommandType::SPEED_FAST                , {  CommandType::SPEED_FAST                , {"FastSpeed"},                {} }},
    { CommandType::SPEED_X_FAST              , {  CommandType::SPEED_X_FAST              , {"XFastSpeed"},               {} }},
    { CommandType::TURN_180                  , {  CommandType::TURN_180                  , {"Turn180"},                  {} }},
    { CommandType::TURN_180_NO_UPDATE        , {  CommandType::TURN_180_NO_UPDATE        , {"Turn180NoUpdate"},          {} }},
    { CommandType::TURN_180_IMMEDIATE        , {  CommandType::TURN_180_IMMEDIATE        , {"Turn180Immediate"},         {} }},
    { CommandType::FOLLOW_PLAYER_WITH_JUMP   , {  CommandType::FOLLOW_PLAYER_WITH_JUMP   , {"FollowPlayerWithJump"},     {{"Unknown", ParamType::UINT8}} }},
    { CommandType::JUMP                      , {  CommandType::JUMP                      , {"Jump"},                     {} }},
    { CommandType::ENABLE_FRAME_UPDATE       , {  CommandType::ENABLE_FRAME_UPDATE       , {"EnableFrameUpdate"},        {} }},
    { CommandType::DISABLE_FRAME_UPDATE      , {  CommandType::DISABLE_FRAME_UPDATE      , {"DisableFrameUpdate"},       {} }},
    { CommandType::MOVE_RANDOM_TIMED         , {  CommandType::MOVE_RANDOM_TIMED         , {"MoveRandomTimed"},          {{"Ticks", ParamType::UINT8}, {"BaseTicks", ParamType::UINT8}} }},
    { CommandType::LOAD_SPECIAL_AI           , {  CommandType::LOAD_SPECIAL_AI           , {"LoadSpecialAI"},            {} }},
    { CommandType::FOLLOW_PLAYER_NO_JUMP     , {  CommandType::FOLLOW_PLAYER_NO_JUMP     , {"FollowPlayerNoJump"},       {{"Unknown", ParamType::UINT8}} }},
    { CommandType::UNKNOWN_B2D               , {  CommandType::UNKNOWN_B2D               , {"UnknownB2D"},               {} }},
    { CommandType::SHOP_ITEM                 , {  CommandType::SHOP_ITEM                 , {"ShopItem"},                 {} }},
    { CommandType::UNKOWN_B2F                , {  CommandType::UNKOWN_B2F                , {"UnknownB2F"},               {} }},
    { CommandType::MOVE_UP_RELATIVE          , {  CommandType::MOVE_UP_RELATIVE          , {"MoveUpRelative"},           {{"Distance", ParamType::COORDINATE}} }},
    { CommandType::MOVE_DOWN_RELATIVE        , {  CommandType::MOVE_DOWN_RELATIVE        , {"MoveDownRelative"},         {{"Distance", ParamType::COORDINATE}} }},
    { CommandType::UNKNOWN_B32               , {  CommandType::UNKNOWN_B32               , {"UnknownB32"},               {{"Unknown", ParamType::UINT8}} }},
    { CommandType::UNKNOWN_B33               , {  CommandType::UNKNOWN_B33               , {"UnknownB33"},               {{"Unknown", ParamType::UINT8}} }},
    { CommandType::PLAYBACK_INPUT            , {  CommandType::PLAYBACK_INPUT            , {"PlaybackInput"},            {{"InputScript", ParamType::UINT8}} }},
    { CommandType::RESET_PLAYBACK            , {  CommandType::RESET_PLAYBACK            , {"ResetPlayback"},            {} }},
    { CommandType::WAIT_FOR_CONDITION        , {  CommandType::WAIT_FOR_CONDITION        , {"WaitForCondition"},         {{"Condition", ParamType::UINT8}} }},
    { CommandType::PAUSE_4_SECONDS           , {  CommandType::PAUSE_4_SECONDS           , {"Pause4s"},                  {} }},
    { CommandType::ENABLE_GRAVITY            , {  CommandType::ENABLE_GRAVITY            , {"EnableGravity"},            {} }},
    { CommandType::DISABLE_GRAVITY           , {  CommandType::DISABLE_GRAVITY           , {"DisableGravity"},           {} }},
    { CommandType::MOVE_UP_TIMED             , {  CommandType::MOVE_UP_TIMED             , {"MoveUpTimed"},              {{"Ticks", ParamType::UINT8}} }},
    { CommandType::MOVE_DOWN_TIMED           , {  CommandType::MOVE_DOWN_TIMED           , {"MoveDownTimed"},            {{"Ticks", ParamType::UINT8}} }},
    { CommandType::MOVE_UP_ABSOLUTE          , {  CommandType::MOVE_UP_ABSOLUTE          , {"MoveUpAbsolute"},           {{"Distance", ParamType::COORDINATE}} }},
    { CommandType::MOVE_DOWN_ABSOLUTE        , {  CommandType::MOVE_DOWN_ABSOLUTE        , {"MoveDownAbsolute"},         {{"Distance", ParamType::COORDINATE}} }},
    { CommandType::REMOVE_SPRITE             , {  CommandType::REMOVE_SPRITE             , {"RemoveSprite"},             {} }},
    { CommandType::NUDGE_UP                  , {  CommandType::NUDGE_UP                  , {"NudgeUp"},                  {} }},
    { CommandType::MOVE_DOWN_UNTIL_COLLISION , {  CommandType::MOVE_DOWN_UNTIL_COLLISION , {"MoveDownUntilCollision"},   {} }},
    { CommandType::SET_FLAG                  , {  CommandType::SET_FLAG                  , {"SetFlag"},                  {{"Flag", ParamType::FLAG}} }},
    { CommandType::WAIT_FOR_FLAG_SET         , {  CommandType::WAIT_FOR_FLAG_SET         , {"WaitForFlagSet"},           {{"Flag", ParamType::FLAG}} }},
    { CommandType::CLEAR_FLAG                , {  CommandType::CLEAR_FLAG                , {"ClearFlag"},                {{"Flag", ParamType::FLAG}} }},
    { CommandType::HIDE                      , {  CommandType::HIDE                      , {"Hide"},                     {} }},
    { CommandType::SHOW_WHEN_COLLISION_CLEAR , {  CommandType::SHOW_WHEN_COLLISION_CLEAR , {"ShowWhenCollisionClear"},   {} }},
    { CommandType::WAIT_FOR_FLAG_CLEAR       , {  CommandType::WAIT_FOR_FLAG_CLEAR       , {"WaitForFlagClear"},         {{"Flag", ParamType::FLAG}} }},
    { CommandType::UNKNOWN_B47               , {  CommandType::UNKNOWN_B47               , {"UnknownB47"},               {{"Unknown", ParamType::UINT8}} }},
    { CommandType::SET_OBJECT_SPEED          , {  CommandType::SET_OBJECT_SPEED          , {"SetObjectSpeed"},           {{"Unknown1", ParamType::UINT8}, {"Unknown2", ParamType::UINT8}} }},
    { CommandType::ACTIVATE_SWITCH           , {  CommandType::ACTIVATE_SWITCH           , {"ActivateSwitch"},           {} }},
    { CommandType::RESET_SWITCH              , {  CommandType::RESET_SWITCH              , {"ResetSwitch"},              {} }},
    { CommandType::MOVE_TO_XY_POS_IMMEDIATE  , {  CommandType::MOVE_TO_XY_POS_IMMEDIATE  , {"MoveToXYPosImmedite"},      {{"X", ParamType::COORDINATE}, {"Y", ParamType::COORDINATE}} }},
    { CommandType::MOVE_TO_Z_POS_IMMEDIATE   , {  CommandType::MOVE_TO_Z_POS_IMMEDIATE   , {"MoveToZPosImmediate"},      {{"Z", ParamType::COORDINATE}} }},
    { CommandType::RESET_TO_INITIAL_POS      , {  CommandType::RESET_TO_INITIAL_POS      , {"ResetToInitialPos"},        {} }},
    { CommandType::START_CUTSCENE            , {  CommandType::START_CUTSCENE            , {"StartCutscene"},            {{"Cutscene", ParamType::LOW_CUTSCENE}} }},
    { CommandType::MOVE_NO_CLIP              , {  CommandType::MOVE_NO_CLIP              , {"MoveNoClip"},               {{"Unknown", ParamType::UINT8}} }},
    { CommandType::ROTATE_PLAYER             , {  CommandType::ROTATE_PLAYER             , {"RotatePlayer"},             {{"Unknown", ParamType::UINT8}} }},
    { CommandType::MAKE_HOSTILE              , {  CommandType::MAKE_HOSTILE              , {"MakeHostile"},              {} }},
    { CommandType::MAKE_NON_HOSTILE          , {  CommandType::MAKE_NON_HOSTILE          , {"MakeNonHostile"},           {} }},
    { CommandType::ENABLE_BACKWARDS_MOVEMENT , {  CommandType::ENABLE_BACKWARDS_MOVEMENT , {"EnableBackwardsMovement"},  {} }},
    { CommandType::DISABLE_BACKWARDS_MOVEMENT, {  CommandType::DISABLE_BACKWARDS_MOVEMENT, {"DisableBackwardsMovement"}, {} }},
    { CommandType::SPECIAL_ANIMATION         , {  CommandType::SPECIAL_ANIMATION         , {"SpecialAnimation"},         {{"Unknown", ParamType::UINT8}} }},
    { CommandType::MOVE_UP_TO_INIT_POS       , {  CommandType::MOVE_UP_TO_INIT_POS       , {"MoveUpToInitPos"},          {} }},
    { CommandType::TRIGGER_TILE_SWAP         , {  CommandType::TRIGGER_TILE_SWAP         , {"TriggerTileSwap"},          {{"Swap", ParamType::UINT8}} }},
    { CommandType::UPDATE_SPRITE_ORIENTATION , {  CommandType::UPDATE_SPRITE_ORIENTATION , {"UpdateSpriteOrientation"},  {} }},
    { CommandType::PRINT_TEXT                , {  CommandType::PRINT_TEXT                , {"PrintText"},                {{"String", ParamType::UINT16}} }},
    { CommandType::UNKNOWN_5A                , {  CommandType::UNKNOWN_5A                , {"Unknown5A"},                {{"Unknown", ParamType::UINT8}} }},
    { CommandType::SET_TARGET_POSITION       , {  CommandType::SET_TARGET_POSITION       , {"SetTargetPosition"},        {{"X", ParamType::LONG_COORDINATE}, {"Y", ParamType::LONG_COORDINATE}} }},
    { CommandType::MOVE_TO_TARGET_POSITION   , {  CommandType::MOVE_TO_TARGET_POSITION   , {"MoveToTargetPosition"},     {} }},
    { CommandType::REPEAT_BEGIN              , {  CommandType::REPEAT_BEGIN              , {"RepeatBegin"},              {{"Repetitions", ParamType::UINT8}} }},
    { CommandType::REPEAT_END                , {  CommandType::REPEAT_END                , {"RepeatEnd"},                {} }},
    { CommandType::DECAY_FLASH               , {  CommandType::DECAY_FLASH               , {"DecayFlash"},               {{"Ticks", ParamType::UINT8}} }},
    { CommandType::FLASH_SPIN_APPEAR         , {  CommandType::FLASH_SPIN_APPEAR         , {"FlashSpinAppear"},          {{"Ticks", ParamType::UINT8}} }},
    { CommandType::FLASH_SPIN_DISAPPEAR      , {  CommandType::FLASH_SPIN_DISAPPEAR      , {"FlashSpinDisappear"},       {{"Ticks", ParamType::UINT8}} }},
    { CommandType::PLAY_SOUND                , {  CommandType::PLAY_SOUND                , {"PlaySound"},                {{"Sound", ParamType::SOUND}} }},
    { CommandType::UNKNOWN_B63               , {  CommandType::UNKNOWN_B63               , {"UnknownB63"},               {{"Unknown", ParamType::UINT8}} }},
    { CommandType::START_HI_CUTSCENE         , {  CommandType::START_HI_CUTSCENE         , {"StartHiCutscene"},          {{"Cutscene", ParamType::HIGH_CUTSCENE}} }},
    { CommandType::UNKNOWN_B65               , {  CommandType::UNKNOWN_B65               , {"UnknownB65"},               {{"Unknown", ParamType::UINT8}} }},
    { CommandType::UNKNOWN_B66               , {  CommandType::UNKNOWN_B66               , {"UnknownB66"},               {{"Unknown", ParamType::UINT8}} }},
    { CommandType::UNKNOWN_B67               , {  CommandType::UNKNOWN_B67               , {"UnknownB67"},               {} }},
    { CommandType::NULL_COMMAND              , {  CommandType::NULL_COMMAND              , {"Null"},                     {} }}
};

std::unordered_map<std::string, Behaviours::CommandType> Behaviours::commands_by_name;

const Behaviours::CommandDefinition& Behaviours::GetCommand(CommandType cmd)
{
    return COMMANDS_BY_ID.at(cmd);
}

const Behaviours::CommandDefinition& Behaviours::GetCommandById(int id)
{
    return COMMANDS_BY_ID.at(static_cast<CommandType>(id));
}

const Behaviours::CommandDefinition& Behaviours::GetCommandByName(const std::string& name)
{
    if (commands_by_name.empty())
    {
        for (const auto& command : COMMANDS_BY_ID)
        {
            for (const auto& alias : command.second.aliases)
            {
                commands_by_name[alias] = command.first;
            }
        }
    }
    try
    {
        const auto& result = GetCommand(commands_by_name.at(name));
        return result;
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Unknown command \"" + name + "\"");
    }
}

std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>> Behaviours::Unpack(const std::vector<uint8_t>& offsets, const std::vector<uint8_t>& behaviour_table)
{
    std::map<int, std::vector<uint8_t>> behaviours_raw;
    std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>> behaviours_decoded;

    std::size_t file_pos = 0;
    int i = 0;
    for (const uint8_t offset : offsets)
    {
        std::vector<uint8_t> behav(behaviour_table.begin() + file_pos, behaviour_table.begin() + file_pos + offset);
        behaviours_raw[i++] = behav;
        file_pos += offset;
    }

    for (const auto& b : behaviours_raw)
    {
        int j = 0;
        std::unordered_map<int, int> labels;
        behaviours_decoded.insert({ b.first, {std::string("Behaviour") + std::to_string(b.first), {}} });
        while (j < static_cast<int>(b.second.size()))
        {
            labels[j] = static_cast<int>(labels.size() + 1);
            const auto& cmd = Behaviours::GetCommandById(b.second.at(j));
            behaviours_decoded[b.first].second.push_back({ cmd.id, {} });
            for (const auto& p : cmd.params)
            {
                Behaviours::ParameterValue value;
                switch (p.second)
                {
                case Behaviours::ParamType::UINT8:
                case Behaviours::ParamType::SOUND:
                case Behaviours::ParamType::LOW_CUTSCENE:
                    value = b.second[++j];
                    break;
                case Behaviours::ParamType::INT8:
                    value = static_cast<int8_t>(b.second[++j]);
                    break;
                case Behaviours::ParamType::HIGH_CUTSCENE:
                    value = b.second[++j] + 256;
                    break;
                case Behaviours::ParamType::UINT16:
                    value = (b.second[static_cast<uint8_t>(j + 1)] << 8) | b.second[static_cast<uint8_t>(j + 2)];
                    j += 2;
                    break;
                case Behaviours::ParamType::LABEL:
                {
                    int bpos = static_cast<int8_t>(b.second[++j]) - 1;
                    if (labels.find(j + bpos) != labels.cend())
                    {
                        value = labels.at(j + bpos);
                    }
                    break;
                }
                case Behaviours::ParamType::COORDINATE:
                    value = static_cast<double>(b.second[++j]) / 16.0;
                    break;
                case Behaviours::ParamType::FLAG:
                {
                    int byte = b.second[++j];
                    int bit = b.second[++j];
                    value = (byte << 3) | (bit & 7);
                    break;
                }
                case Behaviours::ParamType::LONG_COORDINATE:
                    value = static_cast<double>(((b.second[static_cast<uint8_t>(j + 1)] << 8) | b.second[static_cast<uint8_t>(j + 2)]) / 256.0);
                    j += 2;
                    break;
                case Behaviours::ParamType::NONE:
                    throw std::logic_error("Unexpected Parameter Type <NONE>");
                }
                behaviours_decoded[b.first].second.back().params.push_back({ p.first, value, p.second });
            }
            ++j;
        }
    }

    return behaviours_decoded;
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Behaviours::Pack(const std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>>& behaviours)
{
    std::size_t cur_offset = 0;
    std::vector<uint8_t> offset_bytes;
    std::vector<uint8_t> behaviour_bytes;

    for (const auto& b : behaviours)
    {
        std::unordered_map<int, int> labels;
        for (const auto& c : b.second.second)
        {
            labels.insert({ static_cast<int>(labels.size() + 1), static_cast<int>(behaviour_bytes.size()) });
            behaviour_bytes.push_back(static_cast<uint8_t>(c.command));
            for (const auto& p : c.params)
            {
                const auto& param_value = std::get<1>(p);
                const auto& param_type = std::get<2>(p);
                switch (param_type)
                {
                case Behaviours::ParamType::UINT8:
                case Behaviours::ParamType::INT8:
                case Behaviours::ParamType::SOUND:
                case Behaviours::ParamType::LOW_CUTSCENE:
                    behaviour_bytes.push_back(static_cast<uint8_t>(std::get<int>(param_value)));
                    break;
                case Behaviours::ParamType::HIGH_CUTSCENE:
                    behaviour_bytes.push_back(static_cast<uint8_t>(std::get<int>(param_value) - 256));
                    break;
                case Behaviours::ParamType::UINT16:
                    behaviour_bytes.push_back(static_cast<uint8_t>(std::get<int>(param_value) >> 8));
                    behaviour_bytes.push_back(static_cast<uint8_t>(std::get<int>(param_value) & 0xFF));
                    break;
                case Behaviours::ParamType::LABEL:
                    if (labels.find(std::get<int>(param_value)) != labels.cend())
                    {
                        behaviour_bytes.push_back(static_cast<uint8_t>(labels.find(std::get<int>(param_value))->second - behaviour_bytes.size() + 1));
                    }
                    else
                    {
                        assert(false);
                    }
                    break;
                case Behaviours::ParamType::FLAG:
                    behaviour_bytes.push_back(static_cast<uint8_t>(std::get<int>(param_value) >> 3));
                    behaviour_bytes.push_back(static_cast<uint8_t>(std::get<int>(param_value) & 0x07));
                    break;
                case Behaviours::ParamType::COORDINATE:
                    behaviour_bytes.push_back(static_cast<uint8_t>(std::get<double>(param_value) * 16.0));
                    break;
                case Behaviours::ParamType::LONG_COORDINATE:
                    behaviour_bytes.push_back(static_cast<uint8_t>(std::get<double>(param_value)));
                    behaviour_bytes.push_back(static_cast<uint8_t>(std::get<double>(param_value) * 256.0));
                    break;
                case Behaviours::ParamType::NONE:
                    throw std::logic_error("Unexpected Parameter Type <NONE>");
                }
            }
        }
        offset_bytes.push_back(static_cast<uint8_t>(behaviour_bytes.size() - cur_offset));
        cur_offset = behaviour_bytes.size();
    }
    return { offset_bytes, behaviour_bytes };
}

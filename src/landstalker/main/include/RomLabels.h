#ifndef _ROM_LABELS_H_
#define _ROM_LABELS_H_

#include <string>

namespace RomLabels
{
	extern const std::string DEFINES_SECTION;
	extern const std::string DEFINES_FILE;

	namespace Script
	{
		extern const std::string SCRIPT_SECTION;
		extern const std::string SCRIPT_END;
		extern const std::string SCRIPT_STRINGS_BEGIN;

		extern const std::string CUTSCENE_TABLE_SECTION;
		extern const std::string CHAR_TABLE_SECTION;
		extern const std::string SHOP_TABLE_SECTION;
		extern const std::string ITEM_TABLE_SECTION;

		extern const std::string CHAR_FUNCS_SECTION;
		extern const std::string CUTSCENE_FUNCS_SECTION;
		extern const std::string SHOP_FUNCS_SECTION;
		extern const std::string ITEM_FUNCS_SECTION;
		extern const std::string FLAG_PROGRESS_SECTION;

		extern const std::string SCRIPT_FILE;
		extern const std::string CUTSCENE_TABLE_FILE;
		extern const std::string CHAR_TABLE_FILE;
		extern const std::string SHOP_TABLE_FILE;
		extern const std::string ITEM_TABLE_FILE;

		extern const std::string CHAR_FUNCS_FILE;
		extern const std::string CUTSCENE_FUNCS_FILE;
		extern const std::string SHOP_FUNCS_FILE;
		extern const std::string ITEM_FUNCS_FILE;
		extern const std::string FLAG_PROGRESS_FILE;
	}

	namespace Sprites
	{
		extern const std::string POINTER;
		extern const std::string SPRITE_SECTION;
		extern const std::string SPRITE_LUT;
		extern const std::string SPRITE_ANIM_PTR_DATA;
		extern const std::string SPRITE_FRAME_PTR_DATA;
		extern const std::string SPRITE_FRAMES_DATA;
		extern const std::string SPRITE_GFX;
		extern const std::string SPRITE_ANIM;
		extern const std::string SPRITE_FRAME;

		extern const std::string PALETTE_DATA;
		extern const std::string PALETTE_LUT;
		extern const std::string PALETTE_LO_DATA;
		extern const std::string PALETTE_LO_LEA;
		extern const std::string PALETTE_LO;
		extern const std::string PALETTE_HI_DATA;
		extern const std::string PALETTE_HI;
		extern const std::string PALETTE_PROJECTILE_1;
		extern const std::string PALETTE_PROJECTILE_2;
		extern const std::string PALETTE_PROJECTILE_1_MOVEW1;
		extern const std::string PALETTE_PROJECTILE_1_MOVEW2;
		extern const std::string PALETTE_PROJECTILE_1_MOVEW3;
		extern const std::string PALETTE_PROJECTILE_1_MOVEW4;
		extern const std::string PALETTE_PROJECTILE_2_MOVEW1;
		extern const std::string PALETTE_PROJECTILE_2_MOVEW2;
		extern const std::string PALETTE_PROJECTILE_2_MOVEW3;
		extern const std::string PALETTE_PROJECTILE_2_MOVEW4;

		extern const std::string ITEM_PROPERTIES;
		extern const std::string ITEM_PROPERTIES_SECTION;
		extern const std::string SPRITE_BEHAVIOUR_OFFSETS;
		extern const std::string SPRITE_BEHAVIOUR_TABLE;
		extern const std::string SPRITE_BEHAVIOUR_SECTION;

		extern const std::string SPRITE_DATA_SECTION;
		extern const std::string SPRITE_VISIBILITY_FLAGS;
		extern const std::string ONE_TIME_EVENT_FLAGS;
		extern const std::string ROOM_CLEAR_FLAGS;
		extern const std::string LOCKED_DOOR_SPRITE_FLAGS;
		extern const std::string LOCKED_DOOR_SPRITE_FLAGS_LEA2;
		extern const std::string LOCKED_DOOR_SPRITE_FLAGS_LEA3;
		extern const std::string PERMANENT_SWITCH_FLAGS;
		extern const std::string SACRED_TREE_FLAGS;
		extern const std::string SACRED_TREE_FLAGS_LEA2;
		extern const std::string SPRITE_GFX_IDX_LOOKUP;
		extern const std::string SPRITE_DIMENSIONS_LOOKUP;
		extern const std::string ROOM_SPRITE_TABLE_OFFSETS;
		extern const std::string ENEMY_STATS;
		extern const std::string ROOM_SPRITE_TABLE;

		extern const std::string SPRITE_ANIM_FLAGS_LOOKUP;
		extern const std::string SPRITE_ANIM_FLAGS_LOOKUP_SECTION;

		extern const std::string PALETTE_DATA_FILE;
		extern const std::string PALETTE_LUT_FILE;
		extern const std::string PALETTE_LO_FILE;
		extern const std::string PALETTE_HI_FILE;
		extern const std::string PALETTE_PROJECTILE_1_FILE;
		extern const std::string PALETTE_PROJECTILE_2_FILE;
		extern const std::string SPRITE_LUT_FILE;
		extern const std::string SPRITE_ANIMS_FILE;
		extern const std::string SPRITE_ANIM_FRAMES_FILE;
		extern const std::string SPRITE_FRAME_DATA_FILE;
		extern const std::string SPRITE_FRAME_FILE;
		extern const std::string ITEM_PROPERTIES_FILE;
		extern const std::string SPRITE_BEHAVIOUR_OFFSET_FILE;
		extern const std::string SPRITE_BEHAVIOUR_TABLE_FILE;
		extern const std::string SPRITE_ANIM_FLAGS_LOOKUP_FILE;
		extern const std::string SPRITE_VISIBILITY_FLAGS_FILE;
		extern const std::string ONE_TIME_EVENT_FLAGS_FILE;
		extern const std::string ROOM_CLEAR_FLAGS_FILE;
		extern const std::string LOCKED_DOOR_SPRITE_FLAGS_FILE;
		extern const std::string PERMANENT_SWITCH_FLAGS_FILE;
		extern const std::string SACRED_TREE_FLAGS_FILE;
		extern const std::string SPRITE_GFX_IDX_LOOKUP_FILE;
		extern const std::string SPRITE_DIMENSIONS_LOOKUP_FILE;
		extern const std::string ROOM_SPRITE_TABLE_OFFSETS_FILE;
		extern const std::string ENEMY_STATS_FILE;
		extern const std::string ROOM_SPRITE_TABLE_FILE;
	}

	namespace Strings
	{
		extern const std::string REGION_CHECK;
		extern const std::string REGION_CHECK_ROUTINE;
		extern const std::string REGION_CHECK_STRINGS;
		extern const std::string REGION_CHECK_DATA_SECTION;
		extern const std::string REGION_ERROR_LINE1;
		extern const std::string REGION_ERROR_NTSC;
		extern const std::string REGION_ERROR_PAL;
		extern const std::string REGION_ERROR_LINE3;
		extern const std::string STRING_SECTION;
		extern const std::string STRING_BANK;
		extern const std::string STRING_BANK_PTR;
		extern const std::string STRING_BANK_PTR_DATA;
		extern const std::string STRING_BANK_PTR_PTR;
		extern const std::string STRING_PTRS;
		extern const std::string HUFFMAN_SECTION;
		extern const std::string HUFFMAN_OFFSETS;
		extern const std::string HUFFMAN_TABLES;

		extern const std::string STRING_TABLE_DATA;
		extern const std::string SAVE_GAME_LOCATIONS;
		extern const std::string SAVE_GAME_LOCATIONS_LEA;
		extern const std::string ISLAND_MAP_LOCATIONS;
		extern const std::string CHAR_NAME_TABLE;
		extern const std::string SPECIAL_CHAR_NAME_TABLE;
		extern const std::string DEFAULT_NAME;
		extern const std::string ITEM_NAME_TABLE;
		extern const std::string MENU_STRING_TABLE;

		extern const std::string INTRO_STRING_DATA;
		extern const std::string INTRO_STRING_PTRS;
		extern const std::string INTRO_STRING;
		extern const std::string END_CREDIT_STRING_SECTION;
		extern const std::string END_CREDIT_STRINGS;

		extern const std::string SPRITE_TALK_SFX;
		extern const std::string SPRITE_TALK_SFX_SECTION;
		extern const std::string CHARACTER_TALK_SFX;
		extern const std::string CHARACTER_TALK_SFX_SECTION;

		extern const std::string CUTSCENE_SCRIPT_TABLE;
		extern const std::string ROOM_CHARACTER_TABLE;
		extern const std::string CHARACTER_SCRIPT_TABLE;
		extern const std::string MAIN_SCRIPT_TABLE;
		extern const std::string MISC_SCRIPT_SECTION;
		extern const std::string MAIN_SCRIPT_SECTION;
		extern const std::string ROOM_CHARACTER_TABLE_SECTION;

		extern const std::string STRINGS_FILE;
		extern const std::string STRING_PTR_FILE;
		extern const std::string STRING_BANK_PTR_FILE;
		extern const std::string STRING_BANK_FILE;
		extern const std::string REGION_CHECK_FILE;
		extern const std::string REGION_CHECK_STRINGS_FILE;
		extern const std::string REGION_CHECK_ROUTINE_FILE;
		extern const std::string HUFFMAN_OFFSETS_FILE;
		extern const std::string HUFFMAN_TABLE_FILE;
		extern const std::string STRING_TABLE_DATA_FILE;
		extern const std::string SAVE_GAME_LOCATIONS_FILE;
		extern const std::string ISLAND_MAP_LOCATIONS_FILE;
		extern const std::string CHAR_NAME_TABLE_FILE;
		extern const std::string SPECIAL_CHAR_NAME_TABLE_FILE;
		extern const std::string DEFAULT_NAME_FILE;
		extern const std::string ITEM_NAME_TABLE_FILE;
		extern const std::string MENU_STRING_TABLE_FILE;
		extern const std::string INTRO_STRING_DATA_FILE;
		extern const std::string INTRO_STRING_PTRS_FILE;
		extern const std::string INTRO_STRING_FILE;
		extern const std::string END_CREDIT_STRINGS_FILE;
		extern const std::string SPRITE_TALK_SFX_FILE;
		extern const std::string CHARACTER_TALK_SFX_FILE;
		extern const std::string CUTSCENE_SCRIPT_FILE;
		extern const std::string ROOM_DIALOGUE_TABLE_FILE;
		extern const std::string CHARACTER_SCRIPT_FILE;
		extern const std::string MAIN_SCRIPT_FILE;
	}

	namespace Graphics
	{
		extern const std::string SYS_FONT;
		extern const std::string SYS_FONT_SIZE;
		extern const std::string MAIN_FONT;
		extern const std::string MAIN_FONT_PTR;

		extern const std::string INV_SECTION;
		extern const std::string INV_FONT;
		extern const std::string INV_FONT_SIZE;
		extern const std::string INV_CURSOR;
		extern const std::string INV_CURSOR_SIZE;
		extern const std::string INV_ARROW;
		extern const std::string INV_ARROW_SIZE;
		extern const std::string INV_UNUSED1;
		extern const std::string INV_UNUSED1_PLUS6;
		extern const std::string INV_UNUSED1_SIZE;
		extern const std::string INV_UNUSED2;
		extern const std::string INV_UNUSED2_PLUS4;
		extern const std::string INV_UNUSED2_SIZE;
		extern const std::string INV_PAL1;
		extern const std::string INV_PAL2;
		extern const std::string INV_ITEM_PAL;
		extern const std::string INV_ITEM_PAL_SECTION;

		extern const std::string DOWN_ARROW;
		extern const std::string DOWN_ARROW_SECTION;
		extern const std::string RIGHT_ARROW;
		extern const std::string RIGHT_ARROW_SECTION;
		extern const std::string TEXTBOX_2LINE_MAP;
		extern const std::string TEXTBOX_3LINE_MAP;

		extern const std::string PLAYER_PAL;
		extern const std::string HUD_PAL;
		extern const std::string HUD_PAL_LEA2;
		extern const std::string MISC_PAL_SECTION;
		extern const std::string EQUIP_PAL_SECTION;
		extern const std::string SWORD_PAL_SWAPS;
		extern const std::string ARMOUR_PAL_SWAPS;

		extern const std::string STATUS_FX_POINTERS;
		extern const std::string STATUS_FX_DATA;
		extern const std::string STATUS_FX_POISON;
		extern const std::string STATUS_FX_POISON_FRAME;
		extern const std::string STATUS_FX_CONFUSION;
		extern const std::string STATUS_FX_CONFUSION_FRAME;
		extern const std::string STATUS_FX_PARALYSIS;
		extern const std::string STATUS_FX_PARALYSIS_FRAME;
		extern const std::string STATUS_FX_CURSE;
		extern const std::string STATUS_FX_CURSE_FRAME;
		extern const std::string STATUS_FX_SECTION;

		extern const std::string SWORD_FX_DATA;
		extern const std::string INV_TILEMAP;
		extern const std::string SWORD_MAGIC;
		extern const std::string SWORD_MAGIC_LEA;
		extern const std::string SWORD_THUNDER;
		extern const std::string SWORD_THUNDER_LEA;
		extern const std::string SWORD_GAIA;
		extern const std::string SWORD_ICE;
		extern const std::string SWORD_ICE_LEA;
		extern const std::string COINFALL;
		extern const std::string SWORD_FX_SECTION;

		extern const std::string HUD_SECTION;
		extern const std::string HUD_TILEMAP;
		extern const std::string HUD_TILESET;

		extern const std::string ISLAND_MAP_DATA;
		extern const std::string ISLAND_MAP_FG_TILES;
		extern const std::string ISLAND_MAP_FG_MAP;
		extern const std::string ISLAND_MAP_FG_PAL;
		extern const std::string ISLAND_MAP_BG_TILES;
		extern const std::string ISLAND_MAP_BG_MAP;
		extern const std::string ISLAND_MAP_BG_PAL;
		extern const std::string ISLAND_MAP_DOTS;
		extern const std::string ISLAND_MAP_FRIDAY;

		extern const std::string LITHOGRAPH_DATA;
		extern const std::string LITHOGRAPH_PAL;
		extern const std::string LITHOGRAPH_TILES;
		extern const std::string LITHOGRAPH_MAP;

		extern const std::string SEGA_LOGO_DATA;
		extern const std::string SEGA_LOGO_ROUTINES1;
		extern const std::string SEGA_LOGO_PAL;
		extern const std::string SEGA_LOGO_PAL_LEA;
		extern const std::string SEGA_LOGO_ROUTINES2;
		extern const std::string SEGA_LOGO_TILES;

		extern const std::string CLIMAX_LOGO_DATA;
		extern const std::string CLIMAX_LOGO_PAL;
		extern const std::string CLIMAX_LOGO_TILES;
		extern const std::string CLIMAX_LOGO_MAP;

		extern const std::string TITLE_DATA;
		extern const std::string TITLE_PALETTE_BLUE;
		extern const std::string TITLE_PALETTE_BLUE_LEA;
		extern const std::string TITLE_PALETTE_YELLOW;
		extern const std::string TITLE_PALETTE_YELLOW_LEA;
		extern const std::string TITLE_1_TILES;
		extern const std::string TITLE_1_MAP;
		extern const std::string TITLE_2_TILES;
		extern const std::string TITLE_2_MAP;
		extern const std::string TITLE_3_TILES;
		extern const std::string TITLE_3_MAP;
		extern const std::string TITLE_3_PAL;
		extern const std::string TITLE_3_PAL_LEA2;
		extern const std::string TITLE_3_PAL_LEA3;
		extern const std::string TITLE_3_PAL_LEA4;
		extern const std::string TITLE_3_PAL_HIGHLIGHT;
		extern const std::string TITLE_ROUTINES_1;
		extern const std::string TITLE_ROUTINES_2;
		extern const std::string TITLE_ROUTINES_3;

		extern const std::string GAME_LOAD_DATA;
		extern const std::string GAME_LOAD_ROUTINES_1;
		extern const std::string GAME_LOAD_ROUTINES_2;
		extern const std::string GAME_LOAD_ROUTINES_3;
		extern const std::string GAME_LOAD_PALETTE;
		extern const std::string GAME_LOAD_PALETTE_LEA;
		extern const std::string GAME_LOAD_PLAYER_PALETTE;
		extern const std::string GAME_LOAD_CHARS;
		extern const std::string GAME_LOAD_TILES;
		extern const std::string GAME_LOAD_MAP;

		extern const std::string END_CREDITS_DATA;
		extern const std::string END_CREDITS_PAL;
		extern const std::string END_CREDITS_FONT;
		extern const std::string END_CREDITS_LOGOS;
		extern const std::string END_CREDITS_MAP;

		extern const std::string INV_GRAPHICS_FILE;
		extern const std::string SYS_FONT_FILE;
		extern const std::string INV_FONT_FILE;
		extern const std::string INV_FONT_LARGE_FILE;
		extern const std::string MAIN_FONT_FILE;
		extern const std::string INV_CURSOR_FILE;
		extern const std::string INV_ARROW_FILE;
		extern const std::string INV_UNUSED1_FILE;
		extern const std::string INV_UNUSED2_FILE;
		extern const std::string INV_PAL1_FILE;
		extern const std::string INV_PAL2_FILE;
		extern const std::string PLAYER_PAL_FILE;
		extern const std::string HUD_PAL_FILE;
		extern const std::string INV_ITEM_PAL_FILE;
		extern const std::string SWORD_PAL_FILE;
		extern const std::string ARMOUR_PAL_FILE;
		extern const std::string DOWN_ARROW_FILE;
		extern const std::string RIGHT_ARROW_FILE;
		extern const std::string STATUS_FX_POINTER_FILE;
		extern const std::string STATUS_FX_DATA_FILE;
		extern const std::string STATUS_FX_POISON_FILE;
		extern const std::string STATUS_FX_CONFUSION_FILE;
		extern const std::string STATUS_FX_PARALYSIS_FILE;
		extern const std::string STATUS_FX_CURSE_FILE;
		extern const std::string SWORD_FX_DATA_FILE;
		extern const std::string INV_TILEMAP_FILE;
		extern const std::string SWORD_MAGIC_FILE;
		extern const std::string SWORD_THUNDER_FILE;
		extern const std::string SWORD_GAIA_FILE;
		extern const std::string SWORD_ICE_FILE;
		extern const std::string COINFALL_FILE;
		extern const std::string TEXTBOX_2LINE_MAP_FILE;
		extern const std::string TEXTBOX_3LINE_MAP_FILE;
		extern const std::string HUD_TILEMAP_FILE;
		extern const std::string HUD_TILESET_FILE;
		extern const std::string END_CREDITS_DATA_FILE;
		extern const std::string END_CREDITS_PAL_FILE;
		extern const std::string END_CREDITS_FONT_FILE;
		extern const std::string END_CREDITS_LOGOS_FILE;
		extern const std::string END_CREDITS_MAP_FILE;
		extern const std::string ISLAND_MAP_DATA_FILE;
		extern const std::string ISLAND_MAP_FG_TILES_FILE;
		extern const std::string ISLAND_MAP_FG_MAP_FILE;
		extern const std::string ISLAND_MAP_BG_TILES_FILE;
		extern const std::string ISLAND_MAP_BG_MAP_FILE;
		extern const std::string ISLAND_MAP_DOTS_FILE;
		extern const std::string ISLAND_MAP_FRIDAY_FILE;
		extern const std::string ISLAND_MAP_FG_PAL_FILE;
		extern const std::string ISLAND_MAP_BG_PAL_FILE;
		extern const std::string TITLE_DATA_FILE;
		extern const std::string TITLE_PALETTE_BLUE_FILE;
		extern const std::string TITLE_PALETTE_YELLOW_FILE;
		extern const std::string TITLE_1_TILES_FILE;
		extern const std::string TITLE_1_MAP_FILE;
		extern const std::string TITLE_2_TILES_FILE;
		extern const std::string TITLE_2_MAP_FILE;
		extern const std::string TITLE_3_TILES_FILE;
		extern const std::string TITLE_3_MAP_FILE;
		extern const std::string TITLE_3_PAL_FILE;
		extern const std::string TITLE_3_PAL_HIGHLIGHT_FILE;
		extern const std::string TITLE_ROUTINES_1_FILE;
		extern const std::string TITLE_ROUTINES_2_FILE;
		extern const std::string TITLE_ROUTINES_3_FILE;
		extern const std::string LITHOGRAPH_DATA_FILE;
		extern const std::string LITHOGRAPH_PAL_FILE;
		extern const std::string LITHOGRAPH_TILES_FILE;
		extern const std::string LITHOGRAPH_MAP_FILE;
		extern const std::string SEGA_LOGO_DATA_FILE;
		extern const std::string SEGA_LOGO_ROUTINES1_FILE;
		extern const std::string SEGA_LOGO_PAL_FILE;
		extern const std::string SEGA_LOGO_ROUTINES2_FILE;
		extern const std::string SEGA_LOGO_TILES_FILE;
		extern const std::string CLIMAX_LOGO_DATA_FILE;
		extern const std::string CLIMAX_LOGO_PAL_FILE;
		extern const std::string CLIMAX_LOGO_TILES_FILE;
		extern const std::string CLIMAX_LOGO_MAP_FILE;
		extern const std::string GAME_LOAD_DATA_FILE;
		extern const std::string GAME_LOAD_ROUTINES_1_FILE;
		extern const std::string GAME_LOAD_ROUTINES_2_FILE;
		extern const std::string GAME_LOAD_ROUTINES_3_FILE;
		extern const std::string GAME_LOAD_PALETTE_FILE;
		extern const std::string GAME_LOAD_PLAYER_PALETTE_FILE;
		extern const std::string GAME_LOAD_CHARS_FILE;
		extern const std::string GAME_LOAD_TILES_FILE;
		extern const std::string GAME_LOAD_MAP_FILE;
	}

	namespace Tilesets
	{
		extern const std::string INTRO_FONT;
		extern const std::string INTRO_FONT_PTR;
		extern const std::string DATA_LOC;
		extern const std::string PTRTAB_LOC;
		extern const std::string ANIM_DATA_LOC;
		extern const std::string ANIM_LIST_LOC;
		extern const std::string ANIM_IDX_LOC;
		extern const std::string PTRTAB_BEGIN_LOC;
		extern const std::string PTR_LOC;
		extern const std::string LABEL_FORMAT_STRING;
		extern const std::string ANIM_LABEL_FORMAT_STRING;
		extern const std::string ANIM_PTR_LABEL_FORMAT_STRING;
		extern const std::string SECTION;

		extern const std::string ANIM_FILENAME_FORMAT_STRING;
		extern const std::string FILENAME_FORMAT_STRING;
		extern const std::string INTRO_FONT_FILENAME;
		extern const std::string INCLUDE_FILE;
		extern const std::string PTRTAB_FILE;
		extern const std::string ANIM_FILE;
	}

	namespace Blocksets
	{
		extern const std::string PRI_PTRS;
		extern const std::string SEC_PTRS;
		extern const std::string DATA;
		extern const std::string POINTER;
		extern const std::string PRI_LABEL;
		extern const std::string SECTION;
		extern const std::string SEC_LABEL;
		extern const std::string BLOCKSET_LABEL;

		extern const std::string PRI_PTR_FILE;
		extern const std::string SEC_PTR_FILE;
		extern const std::string DATA_FILE;
		extern const std::string BLOCKSET_FILE;
	}

	namespace Rooms
	{
		extern const std::string ROOM_DATA_PTR;
		extern const std::string ROOM_DATA;
		extern const std::string MAP_DATA;
		extern const std::string ROOM_EXITS_PTR;
		extern const std::string ROOM_EXITS;
		extern const std::string ROOM_PALS_PTR;
		extern const std::string ROOM_PALS;
		extern const std::string ROOM_PAL_NAME;
		extern const std::string PALETTE_FORMAT_STRING;
		extern const std::string MAP_FORMAT_STRING;
		extern const std::string ROOM_FALL_DEST;
		extern const std::string ROOM_CLIMB_DEST;
		extern const std::string ROOM_TRANSITIONS;
		extern const std::string PALETTE_LAVA;
		extern const std::string PALETTE_WARP;
		extern const std::string PALETTE_LANTERN;
		extern const std::string CHEST_CONTENTS;
		extern const std::string CHEST_OFFSETS;

		extern const std::string MISC_WARP_SECTION;
		extern const std::string FALL_TABLE_LEA_LOC;
		extern const std::string CLIMB_TABLE_LEA_LOC;
		extern const std::string TRANSITION_TABLE_LEA_LOC1;
		extern const std::string TRANSITION_TABLE_LEA_LOC2;
		extern const std::string ROOM_DATA_SECTION;
		extern const std::string CHEST_SECTION;

		extern const std::string ROOM_VISIT_FLAGS;

		extern const std::string DOOR_OFFSET_TABLE;
		extern const std::string DOOR_TABLE;
		extern const std::string DOOR_TABLE_SECTION;
		extern const std::string GFX_SWAP_FLAGS;
		extern const std::string GFX_SWAP_LOCKED_DOOR_FLAGS_LEA1;
		extern const std::string GFX_SWAP_LOCKED_DOOR_FLAGS_LEA2;
		extern const std::string GFX_SWAP_TREE_FLAGS;
		extern const std::string GFX_SWAP_TABLE;
		extern const std::string GFX_SWAP_SECTION;
		extern const std::string LIFESTOCK_SOLD_FLAGS;
		extern const std::string LIFESTOCK_SOLD_FLAGS_SECTION;
		extern const std::string BIG_TREE_LOCATIONS;
		extern const std::string BIG_TREE_LOCATIONS_SECTION;
		extern const std::string SHOP_LIST_LEA1;
		extern const std::string SHOP_LIST_LEA2;
		extern const std::string SHOP_LIST_SECTION;
		extern const std::string LANTERN_ROOM_FLAGS;
		extern const std::string LANTERN_ROOM_FLAGS_SECTION;

		extern const std::string DOOR_OFFSET_TABLE_FILE;
		extern const std::string DOOR_TABLE_FILE;
		extern const std::string GFX_SWAP_FLAGS_FILE;
		extern const std::string GFX_SWAP_LOCKED_DOOR_FLAGS_FILE;
		extern const std::string GFX_SWAP_TREE_FLAGS_FILE;
		extern const std::string GFX_SWAP_TABLE_FILE;
		extern const std::string LIFESTOCK_SOLD_FLAGS_FILE;
		extern const std::string BIG_TREE_LOCATIONS_FILE;
		extern const std::string SHOP_LIST_FILE;
		extern const std::string LANTERN_ROOM_FILE;

		extern const std::string ROOM_DATA_FILE;
		extern const std::string MAP_DATA_FILE;
		extern const std::string MAP_FILENAME_FORMAT_STRING;
		extern const std::string PALETTE_DATA_FILE;
		extern const std::string PALETTE_FILENAME_FORMAT_STRING;
		extern const std::string WARP_FILENAME;
		extern const std::string FALL_DEST_FILENAME;
		extern const std::string CLIMB_DEST_FILENAME;
		extern const std::string TRANSITION_FILENAME;
		extern const std::string PALETTE_LAVA_FILENAME;
		extern const std::string PALETTE_WARP_FILENAME;
		extern const std::string PALETTE_LANTERN_FILENAME;
		extern const std::string ROOM_VISIT_FLAGS_FILE;
		extern const std::string CHEST_CONTENTS_FILENAME;
		extern const std::string CHEST_OFFSETS_FILENAME;
	}
}

#endif // _ROM_LABELS_H_

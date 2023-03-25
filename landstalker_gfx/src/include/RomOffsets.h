#ifndef _ROM_OFFSETS_H_
#define _ROM_OFFSETS_H_

#include <cstdint>
#include <unordered_map>

namespace RomOffsets
{

	struct Section
	{
		uint32_t begin;
		uint32_t end;
		inline uint32_t size() const
		{
			return end - begin;
		};
	};

	enum class Region
	{
		JP,
		US,
		UK,
		FR,
		DE,
		US_BETA,
		COUNT
	};

	inline const uint32_t CHECKSUM_ADDRESS  = 0x00018E;
	inline const uint32_t CHECKSUM_BEGIN    = 0x000200;
	inline const uint32_t BUILD_DATE_BEGIN  = 0x000202;
	inline const uint32_t BUILD_DATE_LENGTH = 0x00000E;
	inline const uint32_t EXPECTED_SIZE     = 0x200000;

	namespace Sprites
	{
		static const std::string POINTER("SpriteGfxPtrPtr");
		static const std::string SPRITE_SECTION("SpriteGfxPtr");
		static const std::string SPRITE_LUT("SpriteGfxOffsetTable");
		static const std::string SPRITE_ANIM_PTR_DATA("SpriteAnimationPtrs");
		static const std::string SPRITE_FRAME_PTR_DATA("SpriteFramePtrs");
		static const std::string SPRITE_FRAMES_DATA("SpriteFrames");
		static const std::string SPRITE_GFX("SpriteGfx%03d");
		static const std::string SPRITE_ANIM("SpriteGfx%03dAnim%02d");
		static const std::string SPRITE_FRAME("SpriteGfx%03dFrame%02d");

		static const std::string PALETTE_DATA("SpritePalettes");
		static const std::string PALETTE_LUT("SpritePaletteLUT");
		static const std::string PALETTE_LO_DATA("SpritePaletteLo");
		static const std::string PALETTE_LO_LEA("SpritePaletteLoLea");
		static const std::string PALETTE_LO("LowSpritePal%03d");
		static const std::string PALETTE_HI_DATA("SpritePaletteHi");
		static const std::string PALETTE_HI("HiSpritePal%03d");
		static const std::string PALETTE_PROJECTILE_1("ProjectilePalette1");
		static const std::string PALETTE_PROJECTILE_2("ProjectilePalette2");
		static const std::string PALETTE_PROJECTILE_1_MOVEW1("ProjectilePalette1Movew1");
		static const std::string PALETTE_PROJECTILE_1_MOVEW2("ProjectilePalette1Movew2");
		static const std::string PALETTE_PROJECTILE_1_MOVEW3("ProjectilePalette1Movew3");
		static const std::string PALETTE_PROJECTILE_1_MOVEW4("ProjectilePalette1Movew4");
		static const std::string PALETTE_PROJECTILE_2_MOVEW1("ProjectilePalette2Movew1");
		static const std::string PALETTE_PROJECTILE_2_MOVEW2("ProjectilePalette2Movew2");
		static const std::string PALETTE_PROJECTILE_2_MOVEW3("ProjectilePalette2Movew3");
		static const std::string PALETTE_PROJECTILE_2_MOVEW4("ProjectilePalette2Movew4");

		static const std::string ITEM_PROPERTIES("ItemProperties");
		static const std::string ITEM_PROPERTIES_SECTION("ItemPropertiesSection");
		static const std::string SPRITE_BEHAVIOUR_OFFSETS("SpriteBehaviourOffsets");
		static const std::string SPRITE_BEHAVIOUR_TABLE("SpriteBehaviourTable");
		static const std::string SPRITE_BEHAVIOUR_SECTION("SpriteBehaviourSection");

		static const std::string SPRITE_DATA_SECTION("SpriteDataSection");
		static const std::string SPRITE_VISIBILITY_FLAGS("SpriteVisibilityFlags");
		static const std::string ONE_TIME_EVENT_FLAGS("OneTimeEventFlags");
		static const std::string ROOM_CLEAR_FLAGS("RoomClearFlags");
		static const std::string LOCKED_DOOR_SPRITE_FLAGS("LockedDoorSpriteFlags");
		static const std::string LOCKED_DOOR_SPRITE_FLAGS_LEA2("LockedDoorSpriteFlagsLea2");
		static const std::string LOCKED_DOOR_SPRITE_FLAGS_LEA3("LockedDoorSpriteFlagsLea3");
		static const std::string PERMANENT_SWITCH_FLAGS("PermanentSwitchFlags");
		static const std::string SACRED_TREE_FLAGS("SacredTreeFlags");
		static const std::string SACRED_TREE_FLAGS_LEA2("SacredTreeFlagsLea2");
		static const std::string SPRITE_GFX_IDX_LOOKUP("SpriteGfxIdxLookup");
		static const std::string SPRITE_DIMENSIONS_LOOKUP("SpriteDimensionsLookup");
		static const std::string ROOM_SPRITE_TABLE_OFFSETS("RoomSpriteTableOffset");
		static const std::string ENEMY_STATS("EnemyStats");
		static const std::string ROOM_SPRITE_TABLE("RoomSpriteTable");

		static const std::string UNKNOWN_SPRITE_LOOKUP("UnknownSpriteLookup_1");
		static const std::string UNKNOWN_SPRITE_LOOKUP_SECTION("UnknownSpriteLookup_1Section");

		static const std::string PALETTE_DATA_FILE("code/sprites/spritepalettes.asm");
		static const std::string PALETTE_LUT_FILE("assets_packed/spritedata/spritepallookup.bin");
		static const std::string PALETTE_LO_FILE("assets_packed/graphics/spritepalettes/lo%03d.pal");
		static const std::string PALETTE_HI_FILE("assets_packed/graphics/spritepalettes/hi%03d.pal");
		static const std::string PALETTE_PROJECTILE_1_FILE("assets_packed/graphics/spritepalettes/projectile1.pal");
		static const std::string PALETTE_PROJECTILE_2_FILE("assets_packed/graphics/spritepalettes/projectile2.pal");
		static const std::string SPRITE_LUT_FILE("assets_packed/spritedata/spritegfxoffsettable.bin");
		static const std::string SPRITE_ANIMS_FILE("code/pointertables/sprites/spriteanimations.asm");
		static const std::string SPRITE_ANIM_FRAMES_FILE("code/pointertables/sprites/spriteanimationframes.asm");
		static const std::string SPRITE_FRAME_DATA_FILE("code/sprites/spriteframes.asm");
		static const std::string SPRITE_FRAME_FILE("assets_packed/graphics/sprites/spr%03d_frm%02d.frm");
		static const std::string ITEM_PROPERTIES_FILE("assets_packed/script/items.bin");
		static const std::string SPRITE_BEHAVIOUR_OFFSET_FILE("assets_packed/spritedata/behaviouroffsets.bin");
		static const std::string SPRITE_BEHAVIOUR_TABLE_FILE("assets_packed/spritedata/behaviourtable.bin");
		static const std::string UNKNOWN_SPRITE_LOOKUP_FILE("assets_packed/spritedata/spritetable6F.bin");
		static const std::string SPRITE_VISIBILITY_FLAGS_FILE("assets_packed/roomdata/flagactions/spritevisibility.bin");
		static const std::string ONE_TIME_EVENT_FLAGS_FILE("assets_packed/roomdata/flagactions/onetime.bin");
		static const std::string ROOM_CLEAR_FLAGS_FILE("assets_packed/roomdata/flagactions/roomcleared.bin");
		static const std::string LOCKED_DOOR_SPRITE_FLAGS_FILE("assets_packed/roomdata/flagactions/lockeddoorsprites.bin");
		static const std::string PERMANENT_SWITCH_FLAGS_FILE("assets_packed/roomdata/flagactions/permanentswitches.bin");
		static const std::string SACRED_TREE_FLAGS_FILE("assets_packed/roomdata/flagactions/sacredtrees.bin");
		static const std::string SPRITE_GFX_IDX_LOOKUP_FILE("assets_packed/spritedata/spritegfx.bin");
		static const std::string SPRITE_DIMENSIONS_LOOKUP_FILE("assets_packed/spritedata/spritedimensions.bin");
		static const std::string ROOM_SPRITE_TABLE_OFFSETS_FILE("assets_packed/spritedata/roomtableoffsets.bin");
		static const std::string ENEMY_STATS_FILE("assets_packed/spritedata/enemystats.bin");
		static const std::string ROOM_SPRITE_TABLE_FILE("assets_packed/spritedata/roomspritetable.bin");
	}

	namespace Strings
	{
		static const std::string REGION_CHECK("RegionCheck");
		static const std::string REGION_CHECK_ROUTINE("RegionCheckRoutine");
		static const std::string REGION_CHECK_STRINGS("RegionCheckStrings");
		static const std::string REGION_CHECK_DATA_SECTION("RegionCheckDataSection");
		static const std::string REGION_ERROR_LINE1("RegionErrorLine1");
		static const std::string REGION_ERROR_NTSC("RegionErrorNTSC");
		static const std::string REGION_ERROR_PAL("RegionErrorPAL");
		static const std::string REGION_ERROR_LINE3("RegionErrorLine3");
		static const std::string STRING_SECTION("StringData");
		static const std::string STRING_BANK("StringBank%01d");
		static const std::string STRING_BANK_PTR("StringBankPtr");
		static const std::string STRING_BANK_PTR_DATA("StringBankPtrs");
		static const std::string STRING_BANK_PTR_PTR("StringPtr");
		static const std::string STRING_PTRS("StringPtrs");
		static const std::string HUFFMAN_SECTION("HuffmanSection");
		static const std::string HUFFMAN_OFFSETS("HuffTableOffsets");
		static const std::string HUFFMAN_TABLES("HuffTables");
		
		static const std::string STRING_TABLE_DATA("StringTables");
		static const std::string SAVE_GAME_LOCATIONS("SaveGameLocations");
		static const std::string SAVE_GAME_LOCATIONS_LEA("SaveGameLocationsLea");
		static const std::string ISLAND_MAP_LOCATIONS("IslandMapLocations");
		static const std::string CHAR_NAME_TABLE("CharacterNameTable");
		static const std::string SPECIAL_CHAR_NAME_TABLE("SpecialCharacterNameTable");
		static const std::string DEFAULT_NAME("DefaultName");
		static const std::string ITEM_NAME_TABLE("ItemNameTable");
		static const std::string MENU_STRING_TABLE("MenuStringTable");

		static const std::string INTRO_STRING_DATA("IntroStrings");
		static const std::string INTRO_STRING_PTRS("IntroStringPointers");
		static const std::string INTRO_STRING("IntroString%d");
		static const std::string END_CREDIT_STRING_SECTION("EndCreditTextSection");
		static const std::string END_CREDIT_STRINGS("EndCreditText");

		static const std::string SPRITE_TALK_SFX("SpriteIdToTalkSfx");
		static const std::string SPRITE_TALK_SFX_SECTION("SpriteIdToTalkSfxSection");
		static const std::string CHARACTER_TALK_SFX("SpecialCharacterSfxList");
		static const std::string CHARACTER_TALK_SFX_SECTION("SpecialCharacterSfxListSection");

		static const std::string STRINGS_FILE("code/text/strings.asm");
		static const std::string STRING_PTR_FILE("code/pointertables/stringbankptr.asm");
		static const std::string STRING_BANK_PTR_FILE("code/pointertables/strings/stringptrs.asm");
		static const std::string STRING_BANK_FILE("assets_packed/strings/main/strings%02d.huf");
		static const std::string REGION_CHECK_FILE("code/system/regioncheck.asm");
		static const std::string REGION_CHECK_STRINGS_FILE("code/system/regioncheck_strings.asm");
		static const std::string REGION_CHECK_ROUTINE_FILE("code/system/regioncheck_routine.asm");
		static const std::string HUFFMAN_OFFSETS_FILE("assets_packed/strings/main/huffmancharoffsets.bin");
		static const std::string HUFFMAN_TABLE_FILE("assets_packed/strings/main/huffmantables.bin");
		static const std::string STRING_TABLE_DATA_FILE("code/text/stringtables.asm");
		static const std::string SAVE_GAME_LOCATIONS_FILE("assets_packed/script/savegamelocations.bin");
		static const std::string ISLAND_MAP_LOCATIONS_FILE("assets_packed/script/islandmaplocations.bin");
		static const std::string CHAR_NAME_TABLE_FILE("assets_packed/strings/names/characternames.bin");
		static const std::string SPECIAL_CHAR_NAME_TABLE_FILE("assets_packed/strings/names/specialcharacternames.bin");
		static const std::string DEFAULT_NAME_FILE("assets_packed/strings/names/defaultname.bin");
		static const std::string ITEM_NAME_TABLE_FILE("assets_packed/strings/names/itemnames.bin");
		static const std::string MENU_STRING_TABLE_FILE("assets_packed/strings/names/system.bin");
		static const std::string INTRO_STRING_DATA_FILE("code/text/introstrings.asm");
		static const std::string INTRO_STRING_PTRS_FILE("code/pointertables/strings/introstringptrs.asm");
		static const std::string INTRO_STRING_FILE("assets_packed/strings/intro/string%02d.bin");
		static const std::string END_CREDIT_STRINGS_FILE("assets_packed/strings/ending/credits.bin");
		static const std::string SPRITE_TALK_SFX_FILE("assets_packed/script/spritetalksfx.bin");
		static const std::string CHARACTER_TALK_SFX_FILE("assets_packed/script/charactertalksfx.bin");
	}

	namespace Graphics
	{
		static const std::string SYS_FONT("SystemFont");
		static const std::string SYS_FONT_SIZE("SystemFontSize");
		static const std::string MAIN_FONT("MainFont");
		static const std::string MAIN_FONT_PTR("MainFontPtr");

		static const std::string INV_SECTION("InventoryGraphics");
		static const std::string INV_FONT("MenuFont");
		static const std::string INV_FONT_SIZE("MenuFontSize");
		static const std::string INV_CURSOR("MenuCursor2BPP");
		static const std::string INV_CURSOR_SIZE("MenuCursor2BPPSize");
		static const std::string INV_ARROW("ArrowGraphic2BPP");
		static const std::string INV_ARROW_SIZE("ArrowGraphic2BPPSize");
		static const std::string INV_UNUSED1("Unused1_2BPP");
		static const std::string INV_UNUSED1_PLUS6("Unused1_2BPP+6");
		static const std::string INV_UNUSED1_SIZE("Unused1_2BPPSize");
		static const std::string INV_UNUSED2("Unused2_2BPP");
		static const std::string INV_UNUSED2_PLUS4("Unused2_2BPP+4");
		static const std::string INV_UNUSED2_SIZE("Unused2_2BPPSize");
		static const std::string INV_PAL1("InvPalette1");
		static const std::string INV_PAL2("InvPalette2_GreyedOut");
		static const std::string INV_ITEM_PAL("InvItemPal");
		static const std::string INV_ITEM_PAL_SECTION("InvItemPalData");

		static const std::string DOWN_ARROW("DownArrowGfx");
		static const std::string DOWN_ARROW_SECTION("DownArrowGfxSection");
		static const std::string RIGHT_ARROW("RightArrowGfx");
		static const std::string RIGHT_ARROW_SECTION("RightArrowGfxSection");
		static const std::string TEXTBOX_2LINE_MAP("InventoryTextBoxTilemap");
		static const std::string TEXTBOX_3LINE_MAP("TextBoxTilemap");

		static const std::string PLAYER_PAL("DefaultPlayerPal");
		static const std::string HUD_PAL("StatusBarPal");
		static const std::string HUD_PAL_LEA2("StatusBarPal_Lea2");
		static const std::string MISC_PAL_SECTION("MiscPalSection");
		static const std::string EQUIP_PAL_SECTION("EquipPalSection");
		static const std::string SWORD_PAL_SWAPS("SwordPalSwaps");
		static const std::string ARMOUR_PAL_SWAPS("ArmourPalSwaps");

		static const std::string STATUS_FX_POINTERS("StatusAnimPtrs");
		static const std::string STATUS_FX_DATA("StatusAnimData");
		static const std::string STATUS_FX_POISON("PoisonAnim");
		static const std::string STATUS_FX_POISON_FRAME("PoisonAnimFrame%d");
		static const std::string STATUS_FX_CONFUSION("ConfusionAnim");
		static const std::string STATUS_FX_CONFUSION_FRAME("ConfusionAnimFrame%d");
		static const std::string STATUS_FX_PARALYSIS("ParalysisAnim");
		static const std::string STATUS_FX_PARALYSIS_FRAME("ParalysisAnimFrame%d");
		static const std::string STATUS_FX_CURSE("CurseAnim");
		static const std::string STATUS_FX_CURSE_FRAME("CurseAnimFrame%d");
		static const std::string STATUS_FX_SECTION("StatusFxSection");

		static const std::string SWORD_FX_DATA("SwordGfxData");
		static const std::string INV_TILEMAP("InvScreenTilemap");
		static const std::string SWORD_MAGIC("MagicSwordGfx");
		static const std::string SWORD_THUNDER("ThunderSwordGfx");
		static const std::string SWORD_GAIA("GaiaSwordGfx");
		static const std::string SWORD_ICE("IceSwordGfx");
		static const std::string COINFALL("CoinFallGfx");
		static const std::string SWORD_FX_SECTION("SwordGfxSection");

		static const std::string HUD_SECTION("HudSection");
		static const std::string HUD_TILEMAP("StatusBarTilemap");
		static const std::string HUD_TILESET("StatusBarGfx");

		static const std::string ISLAND_MAP_DATA("IslandMap");
		static const std::string ISLAND_MAP_FG_TILES("IslandMapFg");
		static const std::string ISLAND_MAP_FG_MAP("IslandMapFgMap");
		static const std::string ISLAND_MAP_FG_PAL("IslandMapFgPal");
		static const std::string ISLAND_MAP_BG_TILES("IslandMapBg");
		static const std::string ISLAND_MAP_BG_MAP("IslandMapBgMap");
		static const std::string ISLAND_MAP_BG_PAL("IslandMapBgPal");
		static const std::string ISLAND_MAP_DOTS("MapDots");
		static const std::string ISLAND_MAP_FRIDAY("MapFriday");

		static const std::string LITHOGRAPH_DATA("LithographData");
		static const std::string LITHOGRAPH_PAL("LithographPalette");
		static const std::string LITHOGRAPH_TILES("Lithograph");
		static const std::string LITHOGRAPH_MAP("LithographTilemap");

		static const std::string SEGA_LOGO_DATA("SegaLogo");
		static const std::string SEGA_LOGO_ROUTINES1("SegaLogoRoutines1");
		static const std::string SEGA_LOGO_PAL("SegaLogoPalette");
		static const std::string SEGA_LOGO_PAL_LEA("SegaLogoPaletteLea");
		static const std::string SEGA_LOGO_ROUTINES2("SegaLogoRoutines2");
		static const std::string SEGA_LOGO_TILES("SegaLogoTiles");

		static const std::string CLIMAX_LOGO_DATA("ClimaxLogoData");
		static const std::string CLIMAX_LOGO_PAL("ClimaxLogoPalette");
		static const std::string CLIMAX_LOGO_TILES("ClimaxLogo");
		static const std::string CLIMAX_LOGO_MAP("ClimaxLogoMap");

		static const std::string TITLE_DATA("TitleScreen");
		static const std::string TITLE_PALETTE_BLUE("TitlePaletteBlueFade");
		static const std::string TITLE_PALETTE_BLUE_LEA("TitlePaletteBlueFadeLea");
		static const std::string TITLE_PALETTE_YELLOW("TitlePaletteYellowFade");
		static const std::string TITLE_PALETTE_YELLOW_LEA("TitlePaletteYellowFadeLea");
		static const std::string TITLE_1_TILES("Title1");
		static const std::string TITLE_1_MAP("Title1Map");
		static const std::string TITLE_2_TILES("Title2");
		static const std::string TITLE_2_MAP("Title2Map");
		static const std::string TITLE_3_TILES("Title3");
		static const std::string TITLE_3_MAP("Title3Map");
		static const std::string TITLE_3_PAL("Title3Palette");
		static const std::string TITLE_3_PAL_LEA2("Title3Palette_Lea2");
		static const std::string TITLE_3_PAL_LEA3("Title3Palette_Lea3");
		static const std::string TITLE_3_PAL_LEA4("Title3Palette_Lea4");
		static const std::string TITLE_3_PAL_HIGHLIGHT("Title3PaletteHighlight");
		static const std::string TITLE_ROUTINES_1("TitleRoutines1");
		static const std::string TITLE_ROUTINES_2("TitleRoutines2");
		static const std::string TITLE_ROUTINES_3("TitleRoutines3");

		static const std::string GAME_LOAD_DATA("GameLoadScreen");
		static const std::string GAME_LOAD_ROUTINES_1("GameLoadScreenRoutines1");
		static const std::string GAME_LOAD_ROUTINES_2("GameLoadScreenRoutines2");
		static const std::string GAME_LOAD_ROUTINES_3("GameLoadScreenRoutines3");
		static const std::string GAME_LOAD_PALETTE("GameStartPalette");
		static const std::string GAME_LOAD_PALETTE_LEA("GameLoadScreen");
		static const std::string GAME_LOAD_PLAYER_PALETTE("InitialPlayerPal");
		static const std::string GAME_LOAD_CHARS("LoadGameScreenCharsCmp");
		static const std::string GAME_LOAD_TILES("LoadGameScreenGfxCmp");
		static const std::string GAME_LOAD_MAP("LoadGameScreenTilemapCmp");

		static const std::string END_CREDITS_DATA("EndCreditsData");
		static const std::string END_CREDITS_PAL("EndCreditPal");
		static const std::string END_CREDITS_FONT("EndCreditFont");
		static const std::string END_CREDITS_LOGOS("EndCreditLogos");
		static const std::string END_CREDITS_MAP("EndCreditLogoMap");

		static const std::string INV_GRAPHICS_FILE("code/inventory/graphics.asm");
		static const std::string SYS_FONT_FILE("assets_packed/graphics/fonts/system.bin");
		static const std::string INV_FONT_FILE("assets_packed/graphics/fonts/menufont.1bpp");
		static const std::string MAIN_FONT_FILE("assets_packed/graphics/fonts/mainfont.1bpp");
		static const std::string INV_CURSOR_FILE("assets_packed/graphics/static/inventory/cursor.2bpp");
		static const std::string INV_ARROW_FILE("assets_packed/graphics/static/inventory/arrow.2bpp");
		static const std::string INV_UNUSED1_FILE("assets_packed/graphics/static/inventory/unused1.2bpp");
		static const std::string INV_UNUSED2_FILE("assets_packed/graphics/static/inventory/unused2.2bpp");
		static const std::string INV_PAL1_FILE("assets_packed/graphics/static/inventory/inv1.pal");
		static const std::string INV_PAL2_FILE("assets_packed/graphics/static/inventory/invitemgreyedout.pal");
		static const std::string PLAYER_PAL_FILE("assets_packed/graphics/miscpalettes/defaultplayer.pal");
		static const std::string HUD_PAL_FILE("assets_packed/graphics/static/hud/hud.pal");
		static const std::string INV_ITEM_PAL_FILE("assets_packed/graphics/static/inventory/invitempal.pal");
		static const std::string SWORD_PAL_FILE("assets_packed/graphics/miscpalettes/swordpalswaps.pal");
		static const std::string ARMOUR_PAL_FILE("assets_packed/graphics/miscpalettes/armourpalswaps.pal");
		static const std::string DOWN_ARROW_FILE("assets_packed/graphics/static/textbox/downarrow.bin");
		static const std::string RIGHT_ARROW_FILE("assets_packed/graphics/static/textbox/rightarrow.bin");
		static const std::string STATUS_FX_POINTER_FILE("code/pointertables/graphics/statusanimptrs.asm");
		static const std::string STATUS_FX_DATA_FILE("code/graphics/staticimages/statusfx.asm");
		static const std::string STATUS_FX_POISON_FILE("assets_packed/graphics/static/statuseffects/poison%d.lz77");
		static const std::string STATUS_FX_CONFUSION_FILE("assets_packed/graphics/static/statuseffects/confusion%d.lz77");
		static const std::string STATUS_FX_PARALYSIS_FILE("assets_packed/graphics/static/statuseffects/paralysis%d.lz77");
		static const std::string STATUS_FX_CURSE_FILE("assets_packed/graphics/static/statuseffects/curse%d.lz77");
		static const std::string SWORD_FX_DATA_FILE("code/graphics/staticimages/swordfx.asm");
		static const std::string INV_TILEMAP_FILE("assets_packed/graphics/static/inventory/invtilemap.lz77");
		static const std::string SWORD_MAGIC_FILE("assets_packed/graphics/static/swordeffects/magic.lz77");
		static const std::string SWORD_THUNDER_FILE("assets_packed/graphics/static/swordeffects/thunder.lz77");
		static const std::string SWORD_GAIA_FILE("assets_packed/graphics/static/swordeffects/gaia.lz77");
		static const std::string SWORD_ICE_FILE("assets_packed/graphics/static/swordeffects/ice.lz77");
		static const std::string COINFALL_FILE("assets_packed/graphics/static/swordeffects/coinfall.lz77");
		static const std::string TEXTBOX_2LINE_MAP_FILE("assets_packed/graphics/static/textbox/twolinetextbox.map");
		static const std::string TEXTBOX_3LINE_MAP_FILE("assets_packed/graphics/static/textbox/threelinetextbox.map");
		static const std::string HUD_TILEMAP_FILE("assets_packed/graphics/static/hud/hudtilemap.map");
		static const std::string HUD_TILESET_FILE("assets_packed/graphics/static/hud/hud.lz77");
		static const std::string END_CREDITS_DATA_FILE("code/ending/endcreditsdata.asm");
		static const std::string END_CREDITS_PAL_FILE("assets_packed/graphics/static/ending/credits.pal");
		static const std::string END_CREDITS_FONT_FILE("assets_packed/graphics/fonts/credits.lz77");
		static const std::string END_CREDITS_LOGOS_FILE("assets_packed/graphics/static/ending/logos.lz77");
		static const std::string END_CREDITS_MAP_FILE("assets_packed/graphics/static/ending/logos.rle");
		static const std::string ISLAND_MAP_DATA_FILE("code/graphics/staticimages/islandmap.asm");
		static const std::string ISLAND_MAP_FG_TILES_FILE("assets_packed/graphics/static/islandmap/foreground.lz77");
		static const std::string ISLAND_MAP_FG_MAP_FILE("assets_packed/graphics/static/islandmap/foreground.rle");
		static const std::string ISLAND_MAP_BG_TILES_FILE("assets_packed/graphics/static/islandmap/background.lz77");
		static const std::string ISLAND_MAP_BG_MAP_FILE("assets_packed/graphics/static/islandmap/background.rle");
		static const std::string ISLAND_MAP_DOTS_FILE("assets_packed/graphics/static/islandmap/dots.lz77");
		static const std::string ISLAND_MAP_FRIDAY_FILE("assets_packed/graphics/static/islandmap/friday.lz77");
		static const std::string ISLAND_MAP_FG_PAL_FILE("assets_packed/graphics/static/islandmap/foreground.pal");
		static const std::string ISLAND_MAP_BG_PAL_FILE("assets_packed/graphics/static/islandmap/background.pal");
		static const std::string TITLE_DATA_FILE("code/graphics/staticimages/titlescreen.asm");
		static const std::string TITLE_PALETTE_BLUE_FILE("assets_packed/graphics/static/titlescreen/blues.pal");
		static const std::string TITLE_PALETTE_YELLOW_FILE("assets_packed/graphics/static/titlescreen/yellows.pal");
		static const std::string TITLE_1_TILES_FILE("assets_packed/graphics/static/titlescreen/title1.lz77");
		static const std::string TITLE_1_MAP_FILE("assets_packed/graphics/static/titlescreen/title1.rle");
		static const std::string TITLE_2_TILES_FILE("assets_packed/graphics/static/titlescreen/title2.lz77");
		static const std::string TITLE_2_MAP_FILE("assets_packed/graphics/static/titlescreen/title2.rle");
		static const std::string TITLE_3_TILES_FILE("assets_packed/graphics/static/titlescreen/title3.lz77");
		static const std::string TITLE_3_MAP_FILE("assets_packed/graphics/static/titlescreen/title3.rle");
		static const std::string TITLE_3_PAL_FILE("assets_packed/graphics/static/titlescreen/title3.pal");
		static const std::string TITLE_3_PAL_HIGHLIGHT_FILE("assets_packed/graphics/static/titlescreen/title3_highlight.pal");
		static const std::string TITLE_ROUTINES_1_FILE("code/graphics/staticimages/titlescreen1.asm");
		static const std::string TITLE_ROUTINES_2_FILE("code/graphics/staticimages/titlescreen2.asm");
		static const std::string TITLE_ROUTINES_3_FILE("code/graphics/staticimages/titlescreen3.asm");
		static const std::string LITHOGRAPH_DATA_FILE("code/graphics/staticimages/lithographdata.asm");
		static const std::string LITHOGRAPH_PAL_FILE("assets_packed/graphics/static/lithograph/lithograph.pal");
		static const std::string LITHOGRAPH_TILES_FILE("assets_packed/graphics/static/lithograph/lithograph.lz77");
		static const std::string LITHOGRAPH_MAP_FILE("assets_packed/graphics/static/lithograph/lithograph.rle");
		static const std::string SEGA_LOGO_DATA_FILE("code/graphics/staticimages/segalogo.asm");
		static const std::string SEGA_LOGO_ROUTINES1_FILE("code/graphics/staticimages/segalogo1.asm");
		static const std::string SEGA_LOGO_PAL_FILE("assets_packed/graphics/static/logos/sega.pal");
		static const std::string SEGA_LOGO_ROUTINES2_FILE("code/graphics/staticimages/segalogo2.asm");
		static const std::string SEGA_LOGO_TILES_FILE("assets_packed/graphics/static/logos/sega.lz77");
		static const std::string CLIMAX_LOGO_DATA_FILE("code/graphics/staticimages/climaxlogodata.asm");
		static const std::string CLIMAX_LOGO_PAL_FILE("assets_packed/graphics/static/logos/climax.pal");
		static const std::string CLIMAX_LOGO_TILES_FILE("assets_packed/graphics/static/logos/climax.lz77");
		static const std::string CLIMAX_LOGO_MAP_FILE("assets_packed/graphics/static/logos/climax.rle");
		static const std::string GAME_LOAD_DATA_FILE("code/title/gameloadscreen.asm");
		static const std::string GAME_LOAD_ROUTINES_1_FILE("code/title/gameloadscreen1.asm");
		static const std::string GAME_LOAD_ROUTINES_2_FILE("code/title/gameloadscreen2.asm");
		static const std::string GAME_LOAD_ROUTINES_3_FILE("code/title/gameloadscreen3.asm");
		static const std::string GAME_LOAD_PALETTE_FILE("assets_packed/graphics/static/loadgame/loadgame.pal");
		static const std::string GAME_LOAD_PLAYER_PALETTE_FILE("assets_packed/graphics/static/loadgame/nigel.pal");
		static const std::string GAME_LOAD_CHARS_FILE("assets_packed/graphics/static/loadgame/chars.lz77");
		static const std::string GAME_LOAD_TILES_FILE("assets_packed/graphics/static/loadgame/tiles.lz77");
		static const std::string GAME_LOAD_MAP_FILE("assets_packed/graphics/static/loadgame/tilemap.rle");
	}

	namespace Tilesets
	{
		static const std::string INTRO_FONT("IntroFont");
		static const std::string INTRO_FONT_PTR("IntroFontPtr");
		static const std::string DATA_LOC("TilesetData");
		static const std::string PTRTAB_LOC("TilesetPtrTable");
		static const std::string ANIM_DATA_LOC("AnimatedTilesetTable");
		static const std::string ANIM_LIST_LOC("AnimatedTilesetData");
		static const std::string ANIM_IDX_LOC("AnimatedTilesetIdxs");
		static const std::string PTRTAB_BEGIN_LOC("Tilesets");
		static const std::string PTR_LOC("TilesetPointers");
		static const std::string LABEL_FORMAT_STRING("Tileset%02d");
		static const std::string ANIM_LABEL_FORMAT_STRING("Tileset%02dAnim%02d");
		static const std::string ANIM_PTR_LABEL_FORMAT_STRING("Tileset%02dAnim%02dPtr");
		static const std::string SECTION("TilesetSection");

		static const std::string ANIM_FILENAME_FORMAT_STRING("assets_packed/graphics/tilesets/animated/tileset%02dAnim%02d.bin");
		static const std::string FILENAME_FORMAT_STRING("assets_packed/graphics/tilesets/tileset%02d.lz77");
		static const std::string INTRO_FONT_FILENAME("assets_packed/graphics/fonts/introfont.bin");
		static const std::string INCLUDE_FILE("code/graphics/tileset_data.asm");
		static const std::string PTRTAB_FILE("code/pointertables/graphics/tilesetpointers.asm");
		static const std::string ANIM_FILE("code/maps/animtilesettbl.asm");
	}

	namespace Blocksets
	{
		static const std::string PRI_PTRS("BlocksetPrimaryPointers");
		static const std::string SEC_PTRS("BlocksetSecondaryPointers");
		static const std::string DATA("Blocksets");
		static const std::string POINTER("BigTilesListPtr");
		static const std::string PRI_LABEL("BigTilesList");
		static const std::string SECTION("BlocksetSection");
		static const std::string SEC_LABEL("BTPtr%02d");
		static const std::string BLOCKSET_LABEL("BT%02d_%02d");

		static const std::string PRI_PTR_FILE("code/pointertables/blocks/primaryblocksetpointers.asm");
		static const std::string SEC_PTR_FILE("code/pointertables/blocks/secondaryblocksetpointers.asm");
		static const std::string DATA_FILE("code/blocks/blocksets.asm");
		static const std::string BLOCKSET_FILE("assets_packed/graphics/blocksets/blockset%02d_%02d.cbs");
	}

	namespace Rooms
	{
		static const std::string ROOM_DATA_PTR("RoomDataPtr");
		static const std::string ROOM_DATA("RoomData_0");
		static const std::string MAP_DATA("RoomMaps");
		static const std::string ROOM_EXITS_PTR("RoomExitsPtr");
		static const std::string ROOM_EXITS("RoomExits");
		static const std::string ROOM_PALS_PTR("RoomPalPtr");
		static const std::string ROOM_PALS("RoomPals");
		static const std::string ROOM_PAL_NAME("RoomPal%02d");
		static const std::string PALETTE_FORMAT_STRING("pal%03d");
		static const std::string MAP_FORMAT_STRING("Map%03d");
		static const std::string ROOM_FALL_DEST("RoomFallDestination");
		static const std::string ROOM_CLIMB_DEST("RoomClimbDestination");
		static const std::string ROOM_TRANSITIONS("RoomTransitionLookup");
		static const std::string PALETTE_LAVA("LavaPaletteRotation");
		static const std::string PALETTE_WARP("KazaltWarpPalette");
		static const std::string PALETTE_LANTERN("LabrynthLitPal");

		static const std::string MISC_WARP_SECTION("MiscWarpSection");
		static const std::string FALL_TABLE_LEA_LOC("FallTableLeaLoc");
		static const std::string CLIMB_TABLE_LEA_LOC("ClimbTableLeaLoc");
		static const std::string TRANSITION_TABLE_LEA_LOC1("TransitionTableLeaLoc1");
		static const std::string TRANSITION_TABLE_LEA_LOC2("TransitionTableLeaLoc2");
		static const std::string ROOM_DATA_SECTION("RoomDataSection");

		static const std::string ROOM_VISIT_FLAGS("RoomVisitedFlagLookup");

		static const std::string ROOM_DATA_FILE("code/pointertables/maps/roomlist.asm");
		static const std::string MAP_DATA_FILE("code/graphics/roommaps.asm");
		static const std::string MAP_FILENAME_FORMAT_STRING("assets_packed/maps/%s.cmp");
		static const std::string PALETTE_DATA_FILE("code/palettes/roompals.asm");
		static const std::string PALETTE_FILENAME_FORMAT_STRING("assets_packed/graphics/roompalettes/%s.pal");
		static const std::string WARP_FILENAME("assets_packed/roomdata/warps/exits.bin");
		static const std::string FALL_DEST_FILENAME("assets_packed/roomdata/warps/roomfalldests.bin");
		static const std::string CLIMB_DEST_FILENAME("assets_packed/roomdata/warps/roomclimbdests.bin");
		static const std::string TRANSITION_FILENAME("assets_packed/roomdata/flagactions/roomtransitions.bin");
		static const std::string PALETTE_LAVA_FILENAME("assets_packed/graphics/miscpalettes/lavapalette.pal");
		static const std::string PALETTE_WARP_FILENAME("assets_packed/graphics/miscpalettes/kazaltwarp.pal");
		static const std::string PALETTE_LANTERN_FILENAME("assets_packed/graphics/miscpalettes/labrynthlit.pal");
		static const std::string ROOM_VISIT_FLAGS_FILE("assets_packed/roomdata/flagactions/roomvisitflags.bin");
	}

	inline const std::unordered_map<Region, std::string> REGION_NAMES
	{
		{Region::JP,      "Japanese"},
		{Region::US,      "USA"     },
		{Region::UK,      "UK"      },
		{Region::FR,      "French"  },
		{Region::DE,      "German"  },
		{Region::US_BETA, "USA Beta"}
	};

	inline const std::unordered_map<std::string, Region> RELEASE_BUILD_DATE
	{
		{"92/09/18 14:30", Region::JP},
		{"93/07/13 20:03", Region::US},
		{"93/07/15 18:08", Region::UK},
		{"93/11/05 13:22", Region::FR},
		{"93/11/05 13:58", Region::DE},
		{"93/02/24 11:55", Region::US_BETA}
	};

	inline const std::unordered_map<std::string, std::unordered_map<Region, uint32_t>> ADDRESS
	{
		{ Sprites::POINTER,                      {{Region::JP, 0x120000}, {Region::US, 0x120000}, {Region::UK, 0x120000}, {Region::FR, 0x120000}, {Region::DE, 0x120000}, {Region::US_BETA, 0x120000}}},
		{ Sprites::SPRITE_VISIBILITY_FLAGS,      {{Region::JP, 0x000000}, {Region::US, 0x01A382}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::ONE_TIME_EVENT_FLAGS,         {{Region::JP, 0x000000}, {Region::US, 0x01AD38}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::ROOM_CLEAR_FLAGS,             {{Region::JP, 0x000000}, {Region::US, 0x01A44C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::LOCKED_DOOR_SPRITE_FLAGS,     {{Region::JP, 0x000000}, {Region::US, 0x01A446}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::LOCKED_DOOR_SPRITE_FLAGS_LEA2,{{Region::JP, 0x000000}, {Region::US, 0x01A4A4}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::LOCKED_DOOR_SPRITE_FLAGS_LEA3,{{Region::JP, 0x000000}, {Region::US, 0x01A4BC}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PERMANENT_SWITCH_FLAGS,       {{Region::JP, 0x000000}, {Region::US, 0x01A46C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::SACRED_TREE_FLAGS,            {{Region::JP, 0x000000}, {Region::US, 0x01A510}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::SACRED_TREE_FLAGS_LEA2,       {{Region::JP, 0x000000}, {Region::US, 0x01A58C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::SPRITE_GFX_IDX_LOOKUP,        {{Region::JP, 0x000000}, {Region::US, 0x019716}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::SPRITE_DIMENSIONS_LOOKUP,     {{Region::JP, 0x000000}, {Region::US, 0x01981C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::ROOM_SPRITE_TABLE_OFFSETS,    {{Region::JP, 0x000000}, {Region::US, 0x01953A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::ENEMY_STATS,                  {{Region::JP, 0x000000}, {Region::US, 0x019964}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::ROOM_SPRITE_TABLE,            {{Region::JP, 0x000000}, {Region::US, 0x019566}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::ITEM_PROPERTIES,              {{Region::JP, 0x000000}, {Region::US, 0x0292FC}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::SPRITE_BEHAVIOUR_OFFSETS,     {{Region::JP, 0x000000}, {Region::US, 0x09B02C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::SPRITE_BEHAVIOUR_TABLE,       {{Region::JP, 0x000000}, {Region::US, 0x09B044}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::UNKNOWN_SPRITE_LOOKUP,        {{Region::JP, 0x000000}, {Region::US, 0x0107BC}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_LUT,                  {{Region::JP, 0x000000}, {Region::US, 0x1A4468}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_HI,                   {{Region::JP, 0x000000}, {Region::US, 0x1A44AE}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_LO,                   {{Region::JP, 0x000000}, {Region::US, 0x1A4498}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_LO_LEA,               {{Region::JP, 0x000000}, {Region::US, 0x1A44C4}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_PROJECTILE_1_MOVEW1,  {{Region::JP, 0x000000}, {Region::US, 0x1AC404}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_PROJECTILE_1_MOVEW2,  {{Region::JP, 0x000000}, {Region::US, 0x1AC41A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_PROJECTILE_1_MOVEW3,  {{Region::JP, 0x000000}, {Region::US, 0x1AC43A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_PROJECTILE_1_MOVEW4,  {{Region::JP, 0x000000}, {Region::US, 0x1AC450}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_PROJECTILE_2_MOVEW1,  {{Region::JP, 0x000000}, {Region::US, 0x1AF590}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_PROJECTILE_2_MOVEW2,  {{Region::JP, 0x000000}, {Region::US, 0x1AF5A8}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_PROJECTILE_2_MOVEW3,  {{Region::JP, 0x000000}, {Region::US, 0x1AF5C0}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Sprites::PALETTE_PROJECTILE_2_MOVEW4,  {{Region::JP, 0x000000}, {Region::US, 0x1AF5D8}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{"sprite_data_end",                      {{Region::JP, 0x1A4400}, {Region::US, 0x1A4400}, {Region::UK, 0x1A4400}, {Region::FR, 0x1A4400}, {Region::DE, 0x1A4400}, {Region::US_BETA, 0x1A4400}}},
		{ Strings::SPRITE_TALK_SFX,              {{Region::JP, 0x000000}, {Region::US, 0x0290A8}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::CHARACTER_TALK_SFX,           {{Region::JP, 0x000000}, {Region::US, 0x029132}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::SAVE_GAME_LOCATIONS_LEA,      {{Region::JP, 0x000000}, {Region::US, 0x0294CC}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::ISLAND_MAP_LOCATIONS,         {{Region::JP, 0x000000}, {Region::US, 0x02951A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::CHAR_NAME_TABLE,              {{Region::JP, 0x000000}, {Region::US, 0x029490}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::SPECIAL_CHAR_NAME_TABLE,      {{Region::JP, 0x000000}, {Region::US, 0x02947C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::DEFAULT_NAME,                 {{Region::JP, 0x000000}, {Region::US, 0x02948A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::ITEM_NAME_TABLE,              {{Region::JP, 0x000000}, {Region::US, 0x0294A2}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::MENU_STRING_TABLE,            {{Region::JP, 0x000000}, {Region::US, 0x0294AC}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::STRING_BANK_PTR_PTR,          {{Region::JP, 0x000000}, {Region::US, 0x022E80}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::HUFFMAN_OFFSETS,              {{Region::JP, 0x000000}, {Region::US, 0x0246CA}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::HUFFMAN_TABLES,               {{Region::JP, 0x000000}, {Region::US, 0x0246D2}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::INTRO_STRING_PTRS,            {{Region::JP, 0x000000}, {Region::US, 0x00C482}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::END_CREDIT_STRINGS,           {{Region::JP, 0x000000}, {Region::US, 0x09EC96}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::REGION_ERROR_LINE1,           {{Region::JP, 0x000000}, {Region::US, 0x11EA82}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::REGION_ERROR_LINE1,           {{Region::JP, 0x000000}, {Region::US, 0x11EA82}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::REGION_ERROR_NTSC,            {{Region::JP, 0x000000}, {Region::US, 0x11EA9A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::REGION_ERROR_PAL,             {{Region::JP, 0x000000}, {Region::US, 0x11EAAA}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Strings::REGION_ERROR_LINE3,           {{Region::JP, 0x000000}, {Region::US, 0x11EAB8}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::GAME_LOAD_PALETTE_LEA,       {{Region::JP, 0x000000}, {Region::US, 0x00F694}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::GAME_LOAD_PLAYER_PALETTE,    {{Region::JP, 0x000000}, {Region::US, 0x00FB1E}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::GAME_LOAD_CHARS,             {{Region::JP, 0x000000}, {Region::US, 0x00F646}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::GAME_LOAD_TILES,             {{Region::JP, 0x000000}, {Region::US, 0x00F65A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::GAME_LOAD_MAP,               {{Region::JP, 0x000000}, {Region::US, 0x00F66E}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::CLIMAX_LOGO_TILES,           {{Region::JP, 0x000000}, {Region::US, 0x03DF2E}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::CLIMAX_LOGO_MAP,             {{Region::JP, 0x000000}, {Region::US, 0x03DF42}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::CLIMAX_LOGO_PAL,             {{Region::JP, 0x000000}, {Region::US, 0x03DF62}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::SEGA_LOGO_PAL_LEA,           {{Region::JP, 0x000000}, {Region::US, 0x0386B6}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::SEGA_LOGO_TILES,             {{Region::JP, 0x000000}, {Region::US, 0x0386D0}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::LITHOGRAPH_PAL,              {{Region::JP, 0x000000}, {Region::US, 0x038AA2}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::LITHOGRAPH_TILES,            {{Region::JP, 0x000000}, {Region::US, 0x038A12}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::LITHOGRAPH_MAP,              {{Region::JP, 0x000000}, {Region::US, 0x038A5E}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_1_TILES,               {{Region::JP, 0x000000}, {Region::US, 0x03985E}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_2_TILES,               {{Region::JP, 0x000000}, {Region::US, 0x039892}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_3_TILES,               {{Region::JP, 0x000000}, {Region::US, 0x0398C6}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_1_MAP,                 {{Region::JP, 0x000000}, {Region::US, 0x039872}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_2_MAP,                 {{Region::JP, 0x000000}, {Region::US, 0x0398A6}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_3_MAP,                 {{Region::JP, 0x000000}, {Region::US, 0x0398DA}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_3_PAL,                 {{Region::JP, 0x000000}, {Region::US, 0x03994C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_3_PAL_LEA2,            {{Region::JP, 0x000000}, {Region::US, 0x039956}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_3_PAL_LEA3,            {{Region::JP, 0x000000}, {Region::US, 0x039CC8}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_3_PAL_LEA4,            {{Region::JP, 0x000000}, {Region::US, 0x039DF4}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_3_PAL_HIGHLIGHT,       {{Region::JP, 0x000000}, {Region::US, 0x039D4A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_PALETTE_BLUE_LEA,      {{Region::JP, 0x000000}, {Region::US, 0x039AB4}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TITLE_PALETTE_YELLOW_LEA,    {{Region::JP, 0x000000}, {Region::US, 0x039BC6}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::ISLAND_MAP_FG_TILES,         {{Region::JP, 0x000000}, {Region::US, 0x03E6F2}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::ISLAND_MAP_FG_MAP,           {{Region::JP, 0x000000}, {Region::US, 0x03E726}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::ISLAND_MAP_BG_TILES,         {{Region::JP, 0x000000}, {Region::US, 0x03E70C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::ISLAND_MAP_BG_MAP,           {{Region::JP, 0x000000}, {Region::US, 0x03E766}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::ISLAND_MAP_DOTS,             {{Region::JP, 0x000000}, {Region::US, 0x03E694}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::ISLAND_MAP_FRIDAY,           {{Region::JP, 0x000000}, {Region::US, 0x03E6AE}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::ISLAND_MAP_FG_PAL,           {{Region::JP, 0x000000}, {Region::US, 0x03E7D0}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::END_CREDITS_PAL,             {{Region::JP, 0x000000}, {Region::US, 0x09EC56}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::END_CREDITS_FONT,            {{Region::JP, 0x000000}, {Region::US, 0x09EC1E}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::END_CREDITS_LOGOS,           {{Region::JP, 0x000000}, {Region::US, 0x09EC32}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::END_CREDITS_MAP,             {{Region::JP, 0x000000}, {Region::US, 0x09EC46}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::HUD_TILEMAP,                 {{Region::JP, 0x000000}, {Region::US, 0x008F8C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::HUD_TILESET,                 {{Region::JP, 0x000000}, {Region::US, 0x008F34}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TEXTBOX_2LINE_MAP,           {{Region::JP, 0x000000}, {Region::US, 0x0234D4}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::TEXTBOX_3LINE_MAP,           {{Region::JP, 0x000000}, {Region::US, 0x0234E6}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::STATUS_FX_POISON,            {{Region::JP, 0x000000}, {Region::US, 0x016E92}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::STATUS_FX_CONFUSION,         {{Region::JP, 0x000000}, {Region::US, 0x016E88}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::STATUS_FX_PARALYSIS,         {{Region::JP, 0x000000}, {Region::US, 0x016E7E}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::STATUS_FX_CURSE,             {{Region::JP, 0x000000}, {Region::US, 0x016E70}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_TILEMAP,                 {{Region::JP, 0x000000}, {Region::US, 0x007816}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::SWORD_MAGIC,                 {{Region::JP, 0x000000}, {Region::US, 0x0077B2}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::SWORD_THUNDER,               {{Region::JP, 0x000000}, {Region::US, 0x0077BC}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::SWORD_GAIA,                  {{Region::JP, 0x000000}, {Region::US, 0x0077C6}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::SWORD_ICE,                   {{Region::JP, 0x000000}, {Region::US, 0x0077D0}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::COINFALL,                    {{Region::JP, 0x000000}, {Region::US, 0x0077DA}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::DOWN_ARROW,                  {{Region::JP, 0x000000}, {Region::US, 0x02339E}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::RIGHT_ARROW,                 {{Region::JP, 0x000000}, {Region::US, 0x024998}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::SWORD_PAL_SWAPS,             {{Region::JP, 0x000000}, {Region::US, 0x0078D0}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::ARMOUR_PAL_SWAPS,            {{Region::JP, 0x000000}, {Region::US, 0x0078C0}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_ITEM_PAL,                {{Region::JP, 0x000000}, {Region::US, 0x007876}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::HUD_PAL,                     {{Region::JP, 0x000000}, {Region::US, 0x008E80}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::HUD_PAL_LEA2,                {{Region::JP, 0x000000}, {Region::US, 0x008FCE}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::PLAYER_PAL,                  {{Region::JP, 0x000000}, {Region::US, 0x008FB4}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::MAIN_FONT_PTR,               {{Region::JP, 0x000000}, {Region::US, 0x022E84}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::SYS_FONT_SIZE,               {{Region::JP, 0x000000}, {Region::US, 0x000800}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::SYS_FONT,                    {{Region::JP, 0x000000}, {Region::US, 0x11EAE4}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_FONT,                    {{Region::JP, 0x000000}, {Region::US, 0x00DA02}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_FONT_SIZE,               {{Region::JP, 0x000000}, {Region::US, 0x000300}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_CURSOR,                  {{Region::JP, 0x000000}, {Region::US, 0x00D99C}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_CURSOR_SIZE,             {{Region::JP, 0x000000}, {Region::US, 0x000100}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_ARROW,                   {{Region::JP, 0x000000}, {Region::US, 0x00D9BE}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_ARROW_SIZE,              {{Region::JP, 0x000000}, {Region::US, 0x000040}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_UNUSED1,                 {{Region::JP, 0x000000}, {Region::US, 0x00DA62}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_UNUSED1_PLUS6,           {{Region::JP, 0x000000}, {Region::US, 0x00DA6A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_UNUSED1_SIZE,            {{Region::JP, 0x000000}, {Region::US, 0x000016}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_UNUSED2,                 {{Region::JP, 0x000000}, {Region::US, 0x00DA72}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_UNUSED2_PLUS4,           {{Region::JP, 0x000000}, {Region::US, 0x00DA7A}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_UNUSED2_SIZE,            {{Region::JP, 0x000000}, {Region::US, 0x000014}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_PAL1,                    {{Region::JP, 0x000000}, {Region::US, 0x00D2D2}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Graphics::INV_PAL2,                    {{Region::JP, 0x000000}, {Region::US, 0x00D2F4}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Blocksets::POINTER,                    {{Region::JP, 0x1AF800}, {Region::US, 0x1AF800}, {Region::UK, 0x1AF800}, {Region::FR, 0x1AF800}, {Region::DE, 0x1AF800}, {Region::US_BETA, 0x1AF800}}},
		{ Rooms::ROOM_DATA_PTR,                  {{Region::JP, 0x0A0A00}, {Region::US, 0x0A0A00}, {Region::UK, 0x0A0A00}, {Region::FR, 0x0A0A00}, {Region::DE, 0x0A0A00}, {Region::US_BETA, 0x0A0A00}}},
		{ Rooms::ROOM_PALS_PTR,                  {{Region::JP, 0x0A0A04}, {Region::US, 0x0A0A04}, {Region::UK, 0x0A0A04}, {Region::FR, 0x0A0A04}, {Region::DE, 0x0A0A04}, {Region::US_BETA, 0x0A0A04}}},
		{ Rooms::ROOM_EXITS_PTR,                 {{Region::JP, 0x0A0A08}, {Region::US, 0x0A0A08}, {Region::UK, 0x0A0A08}, {Region::FR, 0x0A0A08}, {Region::DE, 0x0A0A08}, {Region::US_BETA, 0x0A0A08}}},
		{ Rooms::FALL_TABLE_LEA_LOC,             {{Region::JP, 0x000000}, {Region::US, 0x00A114}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Rooms::CLIMB_TABLE_LEA_LOC,            {{Region::JP, 0x000000}, {Region::US, 0x00A120}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Rooms::TRANSITION_TABLE_LEA_LOC1,      {{Region::JP, 0x000000}, {Region::US, 0x00A150}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Rooms::TRANSITION_TABLE_LEA_LOC2,      {{Region::JP, 0x000000}, {Region::US, 0x00A186}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}} },
		{ Rooms::ROOM_VISIT_FLAGS,               {{Region::JP, 0x000000}, {Region::US, 0x002942}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}},
		{ Tilesets::INTRO_FONT_PTR,              {{Region::JP, 0x000000}, {Region::US, 0x00C528}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}}
	};

	inline const std::unordered_map<std::string, std::unordered_map<Region, Section>> SECTION
	{
		{"sprite_gfx_lookup",                    {{Region::JP, {0x01ABBE, 0x01AD96}}, {Region::US, {0x01ABF2, 0x01ADCA}}, {Region::UK, {0x01ABF2, 0x01ADCA}}, {Region::FR, {0x01ABE6, 0x01ADBE}}, {Region::DE, {0x01ABEC, 0x01ADC4}}, {Region::US_BETA, {0x01ABB8, 0x01AD90}}}},
		{"sprite_gfx_offset_table",              {{Region::JP, {0x120004, 0x1201B4}}, {Region::US, {0x120004, 0x1201B4}}, {Region::UK, {0x120004, 0x1201B4}}, {Region::FR, {0x120004, 0x1201B4}}, {Region::DE, {0x120004, 0x1201B4}}, {Region::US_BETA, {0x120004, 0x1201B4}}}},
		{"sprite_palette_lookup",                {{Region::JP, {0x1A453A, 0x1A47E0}}, {Region::US, {0x1A453A, 0x1A47E0}}, {Region::UK, {0x1A453A, 0x1A47E0}}, {Region::FR, {0x1A453A, 0x1A47E0}}, {Region::DE, {0x1A453A, 0x1A47E0}}, {Region::US_BETA, {0x1A453A, 0x1A47E0}}}},
		{ Sprites::SPRITE_SECTION,               {{Region::JP, {0x120000, 0x1AF800}}, {Region::US, {0x120000, 0x1AF800}}, {Region::UK, {0x120000, 0x1AF800}}, {Region::FR, {0x120000, 0x1AF800}}, {Region::DE, {0x120000, 0x1AF800}}, {Region::US_BETA, {0x120000, 0x1AF800}}}},
		{ Sprites::SPRITE_DATA_SECTION,          {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x01A5BA, 0x022E80}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Sprites::SPRITE_BEHAVIOUR_SECTION,     {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x09B058, 0x09E75E}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Sprites::ITEM_PROPERTIES_SECTION,      {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x029304, 0x029404}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Sprites::UNKNOWN_SPRITE_LOOKUP_SECTION,{{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x0107D8, 0x010842}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Strings::STRING_SECTION,               {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x02A884, 0x038600}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Strings::HUFFMAN_SECTION,              {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x023900, 0x02469C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Strings::SPRITE_TALK_SFX_SECTION,      {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x0290CA, 0x02912C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Strings::CHARACTER_TALK_SFX_SECTION,   {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x02913E, 0x02914E}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Strings::SAVE_GAME_LOCATIONS,          {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x0294E6, 0x029516}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Strings::STRING_TABLE_DATA,            {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x029536, 0x029ACC}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Strings::INTRO_STRING_DATA,            {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x00C59E, 0x00CDAE}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Strings::END_CREDIT_STRING_SECTION,    {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x09ED1A, 0x09F644}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Strings::REGION_CHECK_DATA_SECTION,    {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x11EB4C, 0x120000}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::LITHOGRAPH_DATA,             {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x038AAC, 0x039762}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::SEGA_LOGO_DATA,              {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x0386E4, 0x0389D4}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::CLIMAX_LOGO_DATA,            {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x03DF8A, 0x03E654}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::GAME_LOAD_DATA,              {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x00FB32, 0x00FF12}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::TITLE_DATA,                  {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x039ED8, 0x03DF0A}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::ISLAND_MAP_DATA,             {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x03ED9A, 0x044010}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::END_CREDITS_DATA,            {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x09FA3A, 0x0A0A00}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::HUD_SECTION,                 {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x009152, 0x0095AC}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::SWORD_FX_SECTION,            {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x007A3C, 0x0085F2}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::STATUS_FX_SECTION,           {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x016ECA, 0x01776C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::DOWN_ARROW_SECTION,          {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x0233BA, 0x02343A}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::RIGHT_ARROW_SECTION,         {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x0249B4, 0x024A34}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::INV_SECTION,                 {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x00DC76, 0x00E110}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Blocksets::SECTION,                    {{Region::JP, {0x1AF800, 0x1E0000}}, {Region::US, {0x1AF800, 0x1E0000}}, {Region::UK, {0x1AF800, 0x1E0000}}, {Region::FR, {0x1AF800, 0x1E0000}}, {Region::DE, {0x1AF800, 0x1E0000}}, {Region::US_BETA, {0x1AF800, 0x1E0000}}}},
		{ Tilesets::SECTION,                     {{Region::JP, {0x043E70, 0x000000}}, {Region::US, {0x044010, 0x09B000}}, {Region::UK, {0x044010, 0x09B000}}, {Region::FR, {0x044010, 0x09B000}}, {Region::DE, {0x044010, 0x09B000}}, {Region::US_BETA, {0x043E70, 0x000000}}}},
		{ Tilesets::DATA_LOC,                    {{Region::JP, {0x043E70, 0x000000}}, {Region::US, {0x044010, 0x09B000}}, {Region::UK, {0x044010, 0x09B000}}, {Region::FR, {0x044010, 0x09B000}}, {Region::DE, {0x044010, 0x09B000}}, {Region::US_BETA, {0x043E70, 0x000000}}}},
		{ Tilesets::INTRO_FONT,                  {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x09A4EA, 0x09AC6A}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Tilesets::ANIM_DATA_LOC,               {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x009E04, 0x009EF8}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Rooms::ROOM_DATA_SECTION,              {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x0A0A12, 0x11EA64}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Rooms::ROOM_FALL_DEST,                 {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x00A1A8, 0x00A35A}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Rooms::ROOM_CLIMB_DEST,                {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x00A35A, 0x00A3D8}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Rooms::ROOM_TRANSITIONS,               {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x00A3D8, 0x00A61C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Rooms::MISC_WARP_SECTION,              {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x00A1A8, 0x00A61C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		// Palettes
		{ Sprites::PALETTE_DATA,                 {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x02A884, 0x038600}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Sprites::PALETTE_PROJECTILE_1,         {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x1AC468, 0x1AC474}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Sprites::PALETTE_PROJECTILE_2,         {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x1AF5F2, 0x1AF800}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
        { Rooms::PALETTE_LAVA,                   {{Region::JP, {0x002608, 0x002628}}, {Region::US, {0x00260E, 0x00262E}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Rooms::PALETTE_LANTERN,                {{Region::JP, {0x002DAE, 0x002DC8}}, {Region::US, {0x002DB4, 0x002DCE}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Rooms::PALETTE_WARP,                   {{Region::JP, {0x00E98A, 0x00E9AE}}, {Region::US, {0x00E80E, 0x00E832}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::SEGA_LOGO_PAL,               {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x0386C2, 0x0386D0}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::GAME_LOAD_PALETTE,           {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x00F6BC, 0x00F6DC}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::TITLE_PALETTE_BLUE,          {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x039B04, 0x039B3C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::TITLE_PALETTE_YELLOW,        {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x039C3C, 0x039C46}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::EQUIP_PAL_SECTION,           {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x007900, 0x00792C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::MISC_PAL_SECTION,            {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x00901C, 0x009046}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{ Graphics::INV_ITEM_PAL_SECTION,        {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x007896, 0x0078B6}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"equipped_sword_palette",               {{Region::JP, {0x0078A6, 0x0078BE}}, {Region::US, {0x007900, 0x007918}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"equipped_armour_palette",              {{Region::JP, {0x0078BE, 0x0078D2}}, {Region::US, {0x007918, 0x00792C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"player_palette",                       {{Region::JP, {0x008FB6, 0x008FD6}}, {Region::US, {0x00901C, 0x00903C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"hud_palette",                          {{Region::JP, {0x008FD6, 0x008FE0}}, {Region::US, {0x00903C, 0x009046}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"inventory_palette_1_8col",             {{Region::JP, {0x00E25C, 0x00E26C}}, {Region::US, {0x00E0E0, 0x00E0F0}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"inventory_palette_2",                  {{Region::JP, {0x00E26C, 0x00E28C}}, {Region::US, {0x00E0F0, 0x00E110}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"inventory_palette_3",                  {{Region::JP, {0x00783C, 0x00785C}}, {Region::US, {0x007896, 0x0078B6}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"new_game_palette",                     {{Region::JP, {0x00F828, 0x00F848}}, {Region::US, {0x00F6BC, 0x00F6DC}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"new_game_player_palette",              {{Region::JP, {0x00FC9E, 0x00FCBE}}, {Region::US, {0x00FB32, 0x00FB52}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"sega_logo_palette",                    {{Region::JP, {0x0386C2, 0x0386D0}}, {Region::US, {0x0386C2, 0x0386D0}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"lithograph_palette",                   {{Region::JP, {0x038A98, 0x038AB8}}, {Region::US, {0x038AAC, 0x038ACC}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title_seq_blue_palette",               {{Region::JP, {0x039AF4, 0x039B2C}}, {Region::US, {0x039B04, 0x039B3C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title_seq_yellow_pallette",            {{Region::JP, {0x039C2C, 0x039C36}}, {Region::US, {0x039C3C, 0x039C46}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title_seq_single_colours",             {{Region::JP, {0x039C90, 0x039C92}}, {Region::US, {0x039CA0, 0x039CA4}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title_seq_palette",                    {{Region::JP, {0x03DDA2, 0x03DDE2}}, {Region::US, {0x03DECA, 0x03DF0A}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"climax_logo_palette",                  {{Region::JP, {0x03E524, 0x03E52C}}, {Region::US, {0x03E64C, 0x03E654}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"island_map_palette",                   {{Region::JP, {0x043D06, 0x043D46}}, {Region::US, {0x043E2E, 0x043E6E}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"end_credits_palette",                  {{Region::JP, {0x09FA24, 0x09FA2C}}, {Region::US, {0x09FA3A, 0x09FA42}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"projectiles_palette_1",                {{Region::JP, {0x1AC468, 0x1AC474}}, {Region::US, {0x1AC468, 0x1AC474}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"projectiles_palette_2",                {{Region::JP, {0x1AF5F2, 0x1AF5FA}}, {Region::US, {0x1AF5F2, 0x1AF5FA}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"sprite_low_palettes",                  {{Region::JP, {0x1A47E0, 0x1A4BA0}}, {Region::US, {0x1A47E0, 0x1A4BA0}}, {Region::UK, {0x1A47E0, 0x1A4BA0}}, {Region::FR, {0x1A47E0, 0x1A4BA0}}, {Region::DE, {0x1A47E0, 0x1A4BA0}}, {Region::US_BETA, {0x1A47E0, 0x1A4BA0}}}},
		{"sprite_high_palettes",                 {{Region::JP, {0x1A4BA0, 0x1A4C8E}}, {Region::US, {0x1A4BA0, 0x1A4C8E}}, {Region::UK, {0x1A4BA0, 0x1A4C8E}}, {Region::FR, {0x1A4BA0, 0x1A4C8E}}, {Region::DE, {0x1A4BA0, 0x1A4C8E}}, {Region::US_BETA, {0x1A4BA0, 0x1A4C8E}}}},
		{"room_palettes",                        {{Region::JP, {0x11C974, 0x11CEF0}}, {Region::US, {0x11C926, 0x11CEA2}}, {Region::UK, {0x11C926, 0x11CEA2}}, {Region::FR, {0x11C926, 0x11CEA2}}, {Region::DE, {0x11C926, 0x11CEA2}}, {Region::US_BETA, {0x11C904, 0x11CE80}}}}
	};
} // namespace RomOffsets

#endif // _ROM_OFFSETS_H_

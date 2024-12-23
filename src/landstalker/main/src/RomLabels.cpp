#include <landstalker/main/include/RomLabels.h>

namespace RomLabels
{
	const std::string DEFINES_SECTION("Defines");
	const std::string DEFINES_FILE("code/include/landstalker.inc");

	namespace Script
	{
		const std::string SCRIPT_SECTION("Script");
		const std::string SCRIPT_END("ScriptEnd");
		const std::string SCRIPT_STRINGS_BEGIN("SCRIPT_STRINGS_BEGIN");

		const std::string CUTSCENE_TABLE_SECTION("CutsceneScriptTable");
		const std::string CHAR_TABLE_SECTION("CharacterScriptTable");
		const std::string SHOP_TABLE_SECTION("ShopScript");
		const std::string ITEM_TABLE_SECTION("ShopSpecialItemsScript");

		const std::string CHAR_FUNCS_SECTION("CharacterScriptFuncs");
		const std::string CUTSCENE_FUNCS_SECTION("CutsceneScriptFuncs");
		const std::string SHOP_FUNCS_SECTION("ShopScriptFuncs");
		const std::string ITEM_FUNCS_SECTION("ShopSpecialItemsFuncs");
		const std::string FLAG_PROGRESS_SECTION("ScriptProgressFlags");

		const std::string SCRIPT_FILE("assets_packed/script/script.bin");
		const std::string CUTSCENE_TABLE_FILE("code/script/cutscenes/script_cutscenetable.asm");
		const std::string CHAR_TABLE_FILE("code/script/characters/script_charactertable.asm");
		const std::string SHOP_TABLE_FILE("code/script/shops/shoptable.asm");
		const std::string ITEM_TABLE_FILE("code/script/shops/shoptable_specialitems.asm");

		const std::string CHAR_FUNCS_FILE("code/script/characters/script_characters.asm");
		const std::string CUTSCENE_FUNCS_FILE("code/script/cutscenes/script_cutscenes.asm");
		const std::string SHOP_FUNCS_FILE("code/script/shops/script_shops.asm");
		const std::string ITEM_FUNCS_FILE("code/script/shops/script_shopspecialitems.asm");
		const std::string FLAG_PROGRESS_FILE("code/script/scriptflagcheck.asm");
	}

	namespace Sprites
	{
		const std::string POINTER("SpriteGfxPtrPtr");
		const std::string SPRITE_SECTION("SpriteGfxPtr");
		const std::string SPRITE_LUT("SpriteGfxOffsetTable");
		const std::string SPRITE_ANIM_PTR_DATA("SpriteAnimationPtrs");
		const std::string SPRITE_FRAME_PTR_DATA("SpriteFramePtrs");
		const std::string SPRITE_FRAMES_DATA("SpriteFrames");
		const std::string SPRITE_GFX("SpriteGfx%03d");
		const std::string SPRITE_ANIM("SpriteGfx%03dAnim%02d");
		const std::string SPRITE_FRAME("SpriteGfx%03dFrame%02d");

		const std::string PALETTE_DATA("SpritePalettes");
		const std::string PALETTE_LUT("SpritePaletteLUT");
		const std::string PALETTE_LO_DATA("SpritePaletteLo");
		const std::string PALETTE_LO_LEA("SpritePaletteLoLea");
		const std::string PALETTE_LO("LowSpritePal%03d");
		const std::string PALETTE_HI_DATA("SpritePaletteHi");
		const std::string PALETTE_HI("HiSpritePal%03d");
		const std::string PALETTE_PROJECTILE_1("ProjectilePalette1");
		const std::string PALETTE_PROJECTILE_2("ProjectilePalette2");
		const std::string PALETTE_PROJECTILE_1_MOVEW1("ProjectilePalette1Movew1");
		const std::string PALETTE_PROJECTILE_1_MOVEW2("ProjectilePalette1Movew2");
		const std::string PALETTE_PROJECTILE_1_MOVEW3("ProjectilePalette1Movew3");
		const std::string PALETTE_PROJECTILE_1_MOVEW4("ProjectilePalette1Movew4");
		const std::string PALETTE_PROJECTILE_2_MOVEW1("ProjectilePalette2Movew1");
		const std::string PALETTE_PROJECTILE_2_MOVEW2("ProjectilePalette2Movew2");
		const std::string PALETTE_PROJECTILE_2_MOVEW3("ProjectilePalette2Movew3");
		const std::string PALETTE_PROJECTILE_2_MOVEW4("ProjectilePalette2Movew4");

		const std::string ITEM_PROPERTIES("ItemProperties");
		const std::string ITEM_PROPERTIES_SECTION("ItemPropertiesSection");
		const std::string SPRITE_BEHAVIOUR_OFFSETS("SpriteBehaviourOffsets");
		const std::string SPRITE_BEHAVIOUR_TABLE("SpriteBehaviourTable");
		const std::string SPRITE_BEHAVIOUR_SECTION("SpriteBehaviourSection");

		const std::string SPRITE_DATA_SECTION("SpriteDataSection");
		const std::string SPRITE_VISIBILITY_FLAGS("SpriteVisibilityFlags");
		const std::string ONE_TIME_EVENT_FLAGS("OneTimeEventFlags");
		const std::string ROOM_CLEAR_FLAGS("RoomClearFlags");
		const std::string LOCKED_DOOR_SPRITE_FLAGS("LockedDoorSpriteFlags");
		const std::string LOCKED_DOOR_SPRITE_FLAGS_LEA2("LockedDoorSpriteFlagsLea2");
		const std::string LOCKED_DOOR_SPRITE_FLAGS_LEA3("LockedDoorSpriteFlagsLea3");
		const std::string PERMANENT_SWITCH_FLAGS("PermanentSwitchFlags");
		const std::string SACRED_TREE_FLAGS("SacredTreeFlags");
		const std::string SACRED_TREE_FLAGS_LEA2("SacredTreeFlagsLea2");
		const std::string SPRITE_GFX_IDX_LOOKUP("SpriteGfxIdxLookup");
		const std::string SPRITE_DIMENSIONS_LOOKUP("SpriteDimensionsLookup");
		const std::string ROOM_SPRITE_TABLE_OFFSETS("RoomSpriteTableOffset");
		const std::string ENEMY_STATS("EnemyStats");
		const std::string ROOM_SPRITE_TABLE("RoomSpriteTable");

		const std::string SPRITE_ANIM_FLAGS_LOOKUP("SpriteAnimFlags");
		const std::string SPRITE_ANIM_FLAGS_LOOKUP_SECTION("SpriteAnimFlagsSection");

		const std::string PALETTE_DATA_FILE("code/sprites/spritepalettes.asm");
		const std::string PALETTE_LUT_FILE("assets_packed/spritedata/spritepallookup.bin");
		const std::string PALETTE_LO_FILE("assets_packed/graphics/spritepalettes/lo%03d.pal");
		const std::string PALETTE_HI_FILE("assets_packed/graphics/spritepalettes/hi%03d.pal");
		const std::string PALETTE_PROJECTILE_1_FILE("assets_packed/graphics/spritepalettes/projectile1.pal");
		const std::string PALETTE_PROJECTILE_2_FILE("assets_packed/graphics/spritepalettes/projectile2.pal");
		const std::string SPRITE_LUT_FILE("assets_packed/spritedata/spritegfxoffsettable.bin");
		const std::string SPRITE_ANIMS_FILE("code/pointertables/sprites/spriteanimations.asm");
		const std::string SPRITE_ANIM_FRAMES_FILE("code/pointertables/sprites/spriteanimationframes.asm");
		const std::string SPRITE_FRAME_DATA_FILE("code/sprites/spriteframes.asm");
		const std::string SPRITE_FRAME_FILE("assets_packed/graphics/sprites/spr%03d_frm%02d.frm");
		const std::string ITEM_PROPERTIES_FILE("assets_packed/script/items.bin");
		const std::string SPRITE_BEHAVIOUR_OFFSET_FILE("assets_packed/spritedata/behaviouroffsets.bin");
		const std::string SPRITE_BEHAVIOUR_TABLE_FILE("assets_packed/spritedata/behaviourtable.bin");
		const std::string SPRITE_ANIM_FLAGS_LOOKUP_FILE("assets_packed/spritedata/spriteanimflags.bin");
		const std::string SPRITE_VISIBILITY_FLAGS_FILE("assets_packed/roomdata/flagactions/spritevisibility.bin");
		const std::string ONE_TIME_EVENT_FLAGS_FILE("assets_packed/roomdata/flagactions/onetime.bin");
		const std::string ROOM_CLEAR_FLAGS_FILE("assets_packed/roomdata/flagactions/roomcleared.bin");
		const std::string LOCKED_DOOR_SPRITE_FLAGS_FILE("assets_packed/roomdata/flagactions/lockeddoorsprites.bin");
		const std::string PERMANENT_SWITCH_FLAGS_FILE("assets_packed/roomdata/flagactions/permanentswitches.bin");
		const std::string SACRED_TREE_FLAGS_FILE("assets_packed/roomdata/flagactions/sacredtrees.bin");
		const std::string SPRITE_GFX_IDX_LOOKUP_FILE("assets_packed/spritedata/spritegfx.bin");
		const std::string SPRITE_DIMENSIONS_LOOKUP_FILE("assets_packed/spritedata/spritedimensions.bin");
		const std::string ROOM_SPRITE_TABLE_OFFSETS_FILE("assets_packed/spritedata/roomtableoffsets.bin");
		const std::string ENEMY_STATS_FILE("assets_packed/spritedata/enemystats.bin");
		const std::string ROOM_SPRITE_TABLE_FILE("assets_packed/spritedata/roomspritetable.bin");
	}

	namespace Strings
	{
		const std::string REGION_CHECK("RegionCheck");
		const std::string REGION_CHECK_ROUTINE("RegionCheckRoutine");
		const std::string REGION_CHECK_STRINGS("RegionCheckStrings");
		const std::string REGION_CHECK_DATA_SECTION("RegionCheckDataSection");
		const std::string REGION_ERROR_LINE1("RegionErrorLine1");
		const std::string REGION_ERROR_NTSC("RegionErrorNTSC");
		const std::string REGION_ERROR_PAL("RegionErrorPAL");
		const std::string REGION_ERROR_LINE3("RegionErrorLine3");
		const std::string STRING_SECTION("StringData");
		const std::string STRING_BANK("StringBank%01d");
		const std::string STRING_BANK_PTR("StringBankPtr");
		const std::string STRING_BANK_PTR_DATA("StringBankPtrs");
		const std::string STRING_BANK_PTR_PTR("StringPtr");
		const std::string STRING_PTRS("StringPtrs");
		const std::string HUFFMAN_SECTION("HuffmanSection");
		const std::string HUFFMAN_OFFSETS("HuffTableOffsets");
		const std::string HUFFMAN_TABLES("HuffTables");

		const std::string STRING_TABLE_DATA("StringTables");
		const std::string SAVE_GAME_LOCATIONS("SaveGameLocations");
		const std::string SAVE_GAME_LOCATIONS_LEA("SaveGameLocationsLea");
		const std::string ISLAND_MAP_LOCATIONS("IslandMapLocations");
		const std::string CHAR_NAME_TABLE("CharacterNameTable");
		const std::string SPECIAL_CHAR_NAME_TABLE("SpecialCharacterNameTable");
		const std::string DEFAULT_NAME("DefaultName");
		const std::string ITEM_NAME_TABLE("ItemNameTable");
		const std::string MENU_STRING_TABLE("MenuStringTable");

		const std::string INTRO_STRING_DATA("IntroStrings");
		const std::string INTRO_STRING_PTRS("IntroStringPointers");
		const std::string INTRO_STRING("IntroString%d");
		const std::string END_CREDIT_STRING_SECTION("EndCreditTextSection");
		const std::string END_CREDIT_STRINGS("EndCreditText");

		const std::string SPRITE_TALK_SFX("SpriteIdToTalkSfx");
		const std::string SPRITE_TALK_SFX_SECTION("SpriteIdToTalkSfxSection");
		const std::string CHARACTER_TALK_SFX("SpecialCharacterSfxList");
		const std::string CHARACTER_TALK_SFX_SECTION("SpecialCharacterSfxListSection");

		const std::string CUTSCENE_SCRIPT_TABLE("CutsceneScriptTable");
		const std::string ROOM_CHARACTER_TABLE("RoomDialogueTable");
		const std::string CHARACTER_SCRIPT_TABLE("CharacterScriptTable");
		const std::string MAIN_SCRIPT_TABLE("Script");
		const std::string MISC_SCRIPT_SECTION("MiscScriptSection");
		const std::string MAIN_SCRIPT_SECTION("MainScriptSection");
		const std::string ROOM_CHARACTER_TABLE_SECTION("RoomCharacterTableSection");

		const std::string STRINGS_FILE("code/text/strings.asm");
		const std::string STRING_PTR_FILE("code/pointertables/stringbankptr.asm");
		const std::string STRING_BANK_PTR_FILE("code/pointertables/strings/stringptrs.asm");
		const std::string STRING_BANK_FILE("assets_packed/strings/main/strings%02d.huf");
		const std::string REGION_CHECK_FILE("code/system/regioncheck.asm");
		const std::string REGION_CHECK_STRINGS_FILE("code/system/regioncheck_strings.asm");
		const std::string REGION_CHECK_ROUTINE_FILE("code/system/regioncheck_routine.asm");
		const std::string HUFFMAN_OFFSETS_FILE("assets_packed/strings/main/huffmancharoffsets.bin");
		const std::string HUFFMAN_TABLE_FILE("assets_packed/strings/main/huffmantables.bin");
		const std::string STRING_TABLE_DATA_FILE("code/text/stringtables.asm");
		const std::string SAVE_GAME_LOCATIONS_FILE("assets_packed/script/savegamelocations.bin");
		const std::string ISLAND_MAP_LOCATIONS_FILE("assets_packed/script/islandmaplocations.bin");
		const std::string CHAR_NAME_TABLE_FILE("assets_packed/strings/names/characternames.bin");
		const std::string SPECIAL_CHAR_NAME_TABLE_FILE("assets_packed/strings/names/specialcharacternames.bin");
		const std::string DEFAULT_NAME_FILE("assets_packed/strings/names/defaultname.bin");
		const std::string ITEM_NAME_TABLE_FILE("assets_packed/strings/names/itemnames.bin");
		const std::string MENU_STRING_TABLE_FILE("assets_packed/strings/names/system.bin");
		const std::string INTRO_STRING_DATA_FILE("code/text/introstrings.asm");
		const std::string INTRO_STRING_PTRS_FILE("code/pointertables/strings/introstringptrs.asm");
		const std::string INTRO_STRING_FILE("assets_packed/strings/intro/string%02d.bin");
		const std::string END_CREDIT_STRINGS_FILE("assets_packed/strings/ending/credits.bin");
		const std::string SPRITE_TALK_SFX_FILE("assets_packed/script/spritetalksfx.bin");
		const std::string CHARACTER_TALK_SFX_FILE("assets_packed/script/charactertalksfx.bin");
		const std::string CUTSCENE_SCRIPT_FILE("code/script/cutscenes/script_cutscenetable.asm");
		const std::string ROOM_DIALOGUE_TABLE_FILE("assets_packed/roomdata/roomcharatertable.bin");
		const std::string CHARACTER_SCRIPT_FILE("code/script/characters/script_charactertable.asm");
		const std::string MAIN_SCRIPT_FILE("assets_packed/script/script.bin");
	}

	namespace Graphics
	{
		const std::string SYS_FONT("SystemFont");
		const std::string SYS_FONT_SIZE("SystemFontSize");
		const std::string MAIN_FONT("MainFont");
		const std::string MAIN_FONT_PTR("MainFontPtr");

		const std::string INV_SECTION("InventoryGraphics");
		const std::string INV_FONT("MenuFont");
		const std::string INV_FONT_SIZE("MenuFontSize");
		const std::string INV_CURSOR("MenuCursor2BPP");
		const std::string INV_CURSOR_SIZE("MenuCursor2BPPSize");
		const std::string INV_ARROW("ArrowGraphic2BPP");
		const std::string INV_ARROW_SIZE("ArrowGraphic2BPPSize");
		const std::string INV_UNUSED1("Unused1_2BPP");
		const std::string INV_UNUSED1_PLUS6("Unused1_2BPP+6");
		const std::string INV_UNUSED1_SIZE("Unused1_2BPPSize");
		const std::string INV_UNUSED2("Unused2_2BPP");
		const std::string INV_UNUSED2_PLUS4("Unused2_2BPP+4");
		const std::string INV_UNUSED2_SIZE("Unused2_2BPPSize");
		const std::string INV_PAL1("InvPalette1");
		const std::string INV_PAL2("InvPalette2_GreyedOut");
		const std::string INV_ITEM_PAL("InvItemPal");
		const std::string INV_ITEM_PAL_SECTION("InvItemPalData");

		const std::string DOWN_ARROW("DownArrowGfx");
		const std::string DOWN_ARROW_SECTION("DownArrowGfxSection");
		const std::string RIGHT_ARROW("RightArrowGfx");
		const std::string RIGHT_ARROW_SECTION("RightArrowGfxSection");
		const std::string TEXTBOX_2LINE_MAP("InventoryTextBoxTilemap");
		const std::string TEXTBOX_3LINE_MAP("TextBoxTilemap");

		const std::string PLAYER_PAL("DefaultPlayerPal");
		const std::string HUD_PAL("StatusBarPal");
		const std::string HUD_PAL_LEA2("StatusBarPal_Lea2");
		const std::string MISC_PAL_SECTION("MiscPalSection");
		const std::string EQUIP_PAL_SECTION("EquipPalSection");
		const std::string SWORD_PAL_SWAPS("SwordPalSwaps");
		const std::string ARMOUR_PAL_SWAPS("ArmourPalSwaps");

		const std::string STATUS_FX_POINTERS("StatusAnimPtrs");
		const std::string STATUS_FX_DATA("StatusAnimData");
		const std::string STATUS_FX_POISON("PoisonAnim");
		const std::string STATUS_FX_POISON_FRAME("PoisonAnimFrame%d");
		const std::string STATUS_FX_CONFUSION("ConfusionAnim");
		const std::string STATUS_FX_CONFUSION_FRAME("ConfusionAnimFrame%d");
		const std::string STATUS_FX_PARALYSIS("ParalysisAnim");
		const std::string STATUS_FX_PARALYSIS_FRAME("ParalysisAnimFrame%d");
		const std::string STATUS_FX_CURSE("CurseAnim");
		const std::string STATUS_FX_CURSE_FRAME("CurseAnimFrame%d");
		const std::string STATUS_FX_SECTION("StatusFxSection");

		const std::string SWORD_FX_DATA("SwordGfxData");
		const std::string INV_TILEMAP("InvScreenTilemap");
		const std::string SWORD_MAGIC("MagicSwordGfx");
		const std::string SWORD_MAGIC_LEA("MagicSwordGfxLea");
		const std::string SWORD_THUNDER("ThunderSwordGfx");
		const std::string SWORD_THUNDER_LEA("ThunderSwordGfxLea");
		const std::string SWORD_GAIA("GaiaSwordGfx");
		const std::string SWORD_ICE("IceSwordGfx");
		const std::string SWORD_ICE_LEA("IceSwordGfxLea");
		const std::string COINFALL("CoinFallGfx");
		const std::string SWORD_FX_SECTION("SwordGfxSection");

		const std::string HUD_SECTION("HudSection");
		const std::string HUD_TILEMAP("StatusBarTilemap");
		const std::string HUD_TILESET("StatusBarGfx");

		const std::string ISLAND_MAP_DATA("IslandMap");
		const std::string ISLAND_MAP_FG_TILES("IslandMapFg");
		const std::string ISLAND_MAP_FG_MAP("IslandMapFgMap");
		const std::string ISLAND_MAP_FG_PAL("IslandMapFgPal");
		const std::string ISLAND_MAP_BG_TILES("IslandMapBg");
		const std::string ISLAND_MAP_BG_MAP("IslandMapBgMap");
		const std::string ISLAND_MAP_BG_PAL("IslandMapBgPal");
		const std::string ISLAND_MAP_DOTS("MapDots");
		const std::string ISLAND_MAP_FRIDAY("MapFriday");

		const std::string LITHOGRAPH_DATA("LithographData");
		const std::string LITHOGRAPH_PAL("LithographPalette");
		const std::string LITHOGRAPH_TILES("Lithograph");
		const std::string LITHOGRAPH_MAP("LithographTilemap");

		const std::string SEGA_LOGO_DATA("SegaLogo");
		const std::string SEGA_LOGO_ROUTINES1("SegaLogoRoutines1");
		const std::string SEGA_LOGO_PAL("SegaLogoPalette");
		const std::string SEGA_LOGO_PAL_LEA("SegaLogoPaletteLea");
		const std::string SEGA_LOGO_ROUTINES2("SegaLogoRoutines2");
		const std::string SEGA_LOGO_TILES("SegaLogoTiles");

		const std::string CLIMAX_LOGO_DATA("ClimaxLogoData");
		const std::string CLIMAX_LOGO_PAL("ClimaxLogoPalette");
		const std::string CLIMAX_LOGO_TILES("ClimaxLogo");
		const std::string CLIMAX_LOGO_MAP("ClimaxLogoMap");

		const std::string TITLE_DATA("TitleScreen");
		const std::string TITLE_PALETTE_BLUE("TitlePaletteBlueFade");
		const std::string TITLE_PALETTE_BLUE_LEA("TitlePaletteBlueFadeLea");
		const std::string TITLE_PALETTE_YELLOW("TitlePaletteYellowFade");
		const std::string TITLE_PALETTE_YELLOW_LEA("TitlePaletteYellowFadeLea");
		const std::string TITLE_1_TILES("Title1");
		const std::string TITLE_1_MAP("Title1Map");
		const std::string TITLE_2_TILES("Title2");
		const std::string TITLE_2_MAP("Title2Map");
		const std::string TITLE_3_TILES("Title3");
		const std::string TITLE_3_MAP("Title3Map");
		const std::string TITLE_3_PAL("Title3Palette");
		const std::string TITLE_3_PAL_LEA2("Title3Palette_Lea2");
		const std::string TITLE_3_PAL_LEA3("Title3Palette_Lea3");
		const std::string TITLE_3_PAL_LEA4("Title3Palette_Lea4");
		const std::string TITLE_3_PAL_HIGHLIGHT("Title3PaletteHighlight");
		const std::string TITLE_ROUTINES_1("TitleRoutines1");
		const std::string TITLE_ROUTINES_2("TitleRoutines2");
		const std::string TITLE_ROUTINES_3("TitleRoutines3");

		const std::string GAME_LOAD_DATA("GameLoadScreen");
		const std::string GAME_LOAD_ROUTINES_1("GameLoadScreenRoutines1");
		const std::string GAME_LOAD_ROUTINES_2("GameLoadScreenRoutines2");
		const std::string GAME_LOAD_ROUTINES_3("GameLoadScreenRoutines3");
		const std::string GAME_LOAD_PALETTE("GameStartPalette");
		const std::string GAME_LOAD_PALETTE_LEA("GameLoadScreen");
		const std::string GAME_LOAD_PLAYER_PALETTE("InitialPlayerPal");
		const std::string GAME_LOAD_CHARS("LoadGameScreenCharsCmp");
		const std::string GAME_LOAD_TILES("LoadGameScreenGfxCmp");
		const std::string GAME_LOAD_MAP("LoadGameScreenTilemapCmp");

		const std::string END_CREDITS_DATA("EndCreditsData");
		const std::string END_CREDITS_PAL("EndCreditPal");
		const std::string END_CREDITS_FONT("EndCreditFont");
		const std::string END_CREDITS_LOGOS("EndCreditLogos");
		const std::string END_CREDITS_MAP("EndCreditLogoMap");

		const std::string INV_GRAPHICS_FILE("code/inventory/graphics.asm");
		const std::string SYS_FONT_FILE("assets_packed/graphics/fonts/system.bin");
		const std::string INV_FONT_FILE("assets_packed/graphics/fonts/menufont.1bpp");
		const std::string INV_FONT_LARGE_FILE("assets_packed/graphics/fonts/menufont.lz77");
		const std::string MAIN_FONT_FILE("assets_packed/graphics/fonts/mainfont.1bpp");
		const std::string INV_CURSOR_FILE("assets_packed/graphics/static/inventory/cursor.2bpp");
		const std::string INV_ARROW_FILE("assets_packed/graphics/static/inventory/arrow.2bpp");
		const std::string INV_UNUSED1_FILE("assets_packed/graphics/static/inventory/unused1.2bpp");
		const std::string INV_UNUSED2_FILE("assets_packed/graphics/static/inventory/unused2.2bpp");
		const std::string INV_PAL1_FILE("assets_packed/graphics/static/inventory/inv1.pal");
		const std::string INV_PAL2_FILE("assets_packed/graphics/static/inventory/invitemgreyedout.pal");
		const std::string PLAYER_PAL_FILE("assets_packed/graphics/miscpalettes/defaultplayer.pal");
		const std::string HUD_PAL_FILE("assets_packed/graphics/static/hud/hud.pal");
		const std::string INV_ITEM_PAL_FILE("assets_packed/graphics/static/inventory/invitempal.pal");
		const std::string SWORD_PAL_FILE("assets_packed/graphics/miscpalettes/swordpalswaps.pal");
		const std::string ARMOUR_PAL_FILE("assets_packed/graphics/miscpalettes/armourpalswaps.pal");
		const std::string DOWN_ARROW_FILE("assets_packed/graphics/static/textbox/downarrow.bin");
		const std::string RIGHT_ARROW_FILE("assets_packed/graphics/static/textbox/rightarrow.bin");
		const std::string STATUS_FX_POINTER_FILE("code/pointertables/graphics/statusanimptrs.asm");
		const std::string STATUS_FX_DATA_FILE("code/graphics/staticimages/statusfx.asm");
		const std::string STATUS_FX_POISON_FILE("assets_packed/graphics/static/statuseffects/poison%d.lz77");
		const std::string STATUS_FX_CONFUSION_FILE("assets_packed/graphics/static/statuseffects/confusion%d.lz77");
		const std::string STATUS_FX_PARALYSIS_FILE("assets_packed/graphics/static/statuseffects/paralysis%d.lz77");
		const std::string STATUS_FX_CURSE_FILE("assets_packed/graphics/static/statuseffects/curse%d.lz77");
		const std::string SWORD_FX_DATA_FILE("code/graphics/staticimages/swordfx.asm");
		const std::string INV_TILEMAP_FILE("assets_packed/graphics/static/inventory/invtilemap.lz77");
		const std::string SWORD_MAGIC_FILE("assets_packed/graphics/static/swordeffects/magic.lz77");
		const std::string SWORD_THUNDER_FILE("assets_packed/graphics/static/swordeffects/thunder.lz77");
		const std::string SWORD_GAIA_FILE("assets_packed/graphics/static/swordeffects/gaia.lz77");
		const std::string SWORD_ICE_FILE("assets_packed/graphics/static/swordeffects/ice.lz77");
		const std::string COINFALL_FILE("assets_packed/graphics/static/swordeffects/coinfall.lz77");
		const std::string TEXTBOX_2LINE_MAP_FILE("assets_packed/graphics/static/textbox/twolinetextbox.map");
		const std::string TEXTBOX_3LINE_MAP_FILE("assets_packed/graphics/static/textbox/threelinetextbox.map");
		const std::string HUD_TILEMAP_FILE("assets_packed/graphics/static/hud/hudtilemap.map");
		const std::string HUD_TILESET_FILE("assets_packed/graphics/static/hud/hud.lz77");
		const std::string END_CREDITS_DATA_FILE("code/ending/endcreditsdata.asm");
		const std::string END_CREDITS_PAL_FILE("assets_packed/graphics/static/ending/credits.pal");
		const std::string END_CREDITS_FONT_FILE("assets_packed/graphics/fonts/credits.lz77");
		const std::string END_CREDITS_LOGOS_FILE("assets_packed/graphics/static/ending/logos.lz77");
		const std::string END_CREDITS_MAP_FILE("assets_packed/graphics/static/ending/logos.rle");
		const std::string ISLAND_MAP_DATA_FILE("code/graphics/staticimages/islandmap.asm");
		const std::string ISLAND_MAP_FG_TILES_FILE("assets_packed/graphics/static/islandmap/foreground.lz77");
		const std::string ISLAND_MAP_FG_MAP_FILE("assets_packed/graphics/static/islandmap/foreground.rle");
		const std::string ISLAND_MAP_BG_TILES_FILE("assets_packed/graphics/static/islandmap/background.lz77");
		const std::string ISLAND_MAP_BG_MAP_FILE("assets_packed/graphics/static/islandmap/background.rle");
		const std::string ISLAND_MAP_DOTS_FILE("assets_packed/graphics/static/islandmap/dots.lz77");
		const std::string ISLAND_MAP_FRIDAY_FILE("assets_packed/graphics/static/islandmap/friday.lz77");
		const std::string ISLAND_MAP_FG_PAL_FILE("assets_packed/graphics/static/islandmap/foreground.pal");
		const std::string ISLAND_MAP_BG_PAL_FILE("assets_packed/graphics/static/islandmap/background.pal");
		const std::string TITLE_DATA_FILE("code/graphics/staticimages/titlescreen.asm");
		const std::string TITLE_PALETTE_BLUE_FILE("assets_packed/graphics/static/titlescreen/blues.pal");
		const std::string TITLE_PALETTE_YELLOW_FILE("assets_packed/graphics/static/titlescreen/yellows.pal");
		const std::string TITLE_1_TILES_FILE("assets_packed/graphics/static/titlescreen/title1.lz77");
		const std::string TITLE_1_MAP_FILE("assets_packed/graphics/static/titlescreen/title1.rle");
		const std::string TITLE_2_TILES_FILE("assets_packed/graphics/static/titlescreen/title2.lz77");
		const std::string TITLE_2_MAP_FILE("assets_packed/graphics/static/titlescreen/title2.rle");
		const std::string TITLE_3_TILES_FILE("assets_packed/graphics/static/titlescreen/title3.lz77");
		const std::string TITLE_3_MAP_FILE("assets_packed/graphics/static/titlescreen/title3.rle");
		const std::string TITLE_3_PAL_FILE("assets_packed/graphics/static/titlescreen/title3.pal");
		const std::string TITLE_3_PAL_HIGHLIGHT_FILE("assets_packed/graphics/static/titlescreen/title3_highlight.pal");
		const std::string TITLE_ROUTINES_1_FILE("code/graphics/staticimages/titlescreen1.asm");
		const std::string TITLE_ROUTINES_2_FILE("code/graphics/staticimages/titlescreen2.asm");
		const std::string TITLE_ROUTINES_3_FILE("code/graphics/staticimages/titlescreen3.asm");
		const std::string LITHOGRAPH_DATA_FILE("code/graphics/staticimages/lithographdata.asm");
		const std::string LITHOGRAPH_PAL_FILE("assets_packed/graphics/static/lithograph/lithograph.pal");
		const std::string LITHOGRAPH_TILES_FILE("assets_packed/graphics/static/lithograph/lithograph.lz77");
		const std::string LITHOGRAPH_MAP_FILE("assets_packed/graphics/static/lithograph/lithograph.rle");
		const std::string SEGA_LOGO_DATA_FILE("code/graphics/staticimages/segalogo.asm");
		const std::string SEGA_LOGO_ROUTINES1_FILE("code/graphics/staticimages/segalogo1.asm");
		const std::string SEGA_LOGO_PAL_FILE("assets_packed/graphics/static/logos/sega.pal");
		const std::string SEGA_LOGO_ROUTINES2_FILE("code/graphics/staticimages/segalogo2.asm");
		const std::string SEGA_LOGO_TILES_FILE("assets_packed/graphics/static/logos/sega.lz77");
		const std::string CLIMAX_LOGO_DATA_FILE("code/graphics/staticimages/climaxlogodata.asm");
		const std::string CLIMAX_LOGO_PAL_FILE("assets_packed/graphics/static/logos/climax.pal");
		const std::string CLIMAX_LOGO_TILES_FILE("assets_packed/graphics/static/logos/climax.lz77");
		const std::string CLIMAX_LOGO_MAP_FILE("assets_packed/graphics/static/logos/climax.rle");
		const std::string GAME_LOAD_DATA_FILE("code/title/gameloadscreen.asm");
		const std::string GAME_LOAD_ROUTINES_1_FILE("code/title/gameloadscreen1.asm");
		const std::string GAME_LOAD_ROUTINES_2_FILE("code/title/gameloadscreen2.asm");
		const std::string GAME_LOAD_ROUTINES_3_FILE("code/title/gameloadscreen3.asm");
		const std::string GAME_LOAD_PALETTE_FILE("assets_packed/graphics/static/loadgame/loadgame.pal");
		const std::string GAME_LOAD_PLAYER_PALETTE_FILE("assets_packed/graphics/static/loadgame/nigel.pal");
		const std::string GAME_LOAD_CHARS_FILE("assets_packed/graphics/static/loadgame/chars.lz77");
		const std::string GAME_LOAD_TILES_FILE("assets_packed/graphics/static/loadgame/tiles.lz77");
		const std::string GAME_LOAD_MAP_FILE("assets_packed/graphics/static/loadgame/tilemap.rle");
	}

	namespace Tilesets
	{
		const std::string INTRO_FONT("IntroFont");
		const std::string INTRO_FONT_PTR("IntroFontPtr");
		const std::string DATA_LOC("TilesetData");
		const std::string PTRTAB_LOC("TilesetPtrTable");
		const std::string ANIM_DATA_LOC("AnimatedTilesetTable");
		const std::string ANIM_LIST_LOC("AnimatedTilesetData");
		const std::string ANIM_IDX_LOC("AnimatedTilesetIdxs");
		const std::string PTRTAB_BEGIN_LOC("Tilesets");
		const std::string PTR_LOC("TilesetPointers");
		const std::string LABEL_FORMAT_STRING("Tileset%02d");
		const std::string ANIM_LABEL_FORMAT_STRING("Tileset%02dAnim%02d");
		const std::string ANIM_PTR_LABEL_FORMAT_STRING("Tileset%02dAnim%02dPtr");
		const std::string SECTION("TilesetSection");

		const std::string ANIM_FILENAME_FORMAT_STRING("assets_packed/graphics/tilesets/animated/tileset%02dAnim%02d.bin");
		const std::string FILENAME_FORMAT_STRING("assets_packed/graphics/tilesets/tileset%02d.lz77");
		const std::string INTRO_FONT_FILENAME("assets_packed/graphics/fonts/introfont.bin");
		const std::string INCLUDE_FILE("code/graphics/tileset_data.asm");
		const std::string PTRTAB_FILE("code/pointertables/graphics/tilesetpointers.asm");
		const std::string ANIM_FILE("code/maps/animtilesettbl.asm");
	}

	namespace Blocksets
	{
		const std::string PRI_PTRS("BlocksetPrimaryPointers");
		const std::string SEC_PTRS("BlocksetSecondaryPointers");
		const std::string DATA("Blocksets");
		const std::string POINTER("BigTilesListPtr");
		const std::string PRI_LABEL("BigTilesList");
		const std::string SECTION("BlocksetSection");
		const std::string SEC_LABEL("BTPtr%02d");
		const std::string BLOCKSET_LABEL("BT%02d_%02d");

		const std::string PRI_PTR_FILE("code/pointertables/blocks/primaryblocksetpointers.asm");
		const std::string SEC_PTR_FILE("code/pointertables/blocks/secondaryblocksetpointers.asm");
		const std::string DATA_FILE("code/blocks/blocksets.asm");
		const std::string BLOCKSET_FILE("assets_packed/graphics/blocksets/blockset%02d_%02d.cbs");
	}

	namespace Rooms
	{
		const std::string ROOM_DATA_PTR("RoomDataPtr");
		const std::string ROOM_DATA("RoomData_0");
		const std::string MAP_DATA("RoomMaps");
		const std::string ROOM_EXITS_PTR("RoomExitsPtr");
		const std::string ROOM_EXITS("RoomExits");
		const std::string ROOM_PALS_PTR("RoomPalPtr");
		const std::string ROOM_PALS("RoomPals");
		const std::string ROOM_PAL_NAME("RoomPal%02d");
		const std::string PALETTE_FORMAT_STRING("pal%03d");
		const std::string MAP_FORMAT_STRING("Map%03d");
		const std::string ROOM_FALL_DEST("RoomFallDestination");
		const std::string ROOM_CLIMB_DEST("RoomClimbDestination");
		const std::string ROOM_TRANSITIONS("RoomTransitionLookup");
		const std::string PALETTE_LAVA("LavaPaletteRotation");
		const std::string PALETTE_WARP("KazaltWarpPalette");
		const std::string PALETTE_LANTERN("LabrynthLitPal");
		const std::string CHEST_CONTENTS("ChestContents");
		const std::string CHEST_OFFSETS("RoomChestOffsets");

		const std::string MISC_WARP_SECTION("MiscWarpSection");
		const std::string FALL_TABLE_LEA_LOC("FallTableLeaLoc");
		const std::string CLIMB_TABLE_LEA_LOC("ClimbTableLeaLoc");
		const std::string TRANSITION_TABLE_LEA_LOC1("TransitionTableLeaLoc1");
		const std::string TRANSITION_TABLE_LEA_LOC2("TransitionTableLeaLoc2");
		const std::string ROOM_DATA_SECTION("RoomDataSection");
		const std::string CHEST_SECTION("ChestSection");

		const std::string ROOM_VISIT_FLAGS("RoomVisitedFlagLookup");

		const std::string DOOR_OFFSET_TABLE("DoorLookup");
		const std::string DOOR_TABLE("DoorTable");
		const std::string DOOR_TABLE_SECTION("DoorTableSection");
		const std::string GFX_SWAP_FLAGS("RoomGfxSwapFlags");
		const std::string GFX_SWAP_LOCKED_DOOR_FLAGS_LEA1("LockedDoorGfxSwapFlags");
		const std::string GFX_SWAP_LOCKED_DOOR_FLAGS_LEA2("LockedDoorGfxSwapFlagsLEA");
		const std::string GFX_SWAP_TREE_FLAGS("TreeWarpGfxSwapFlags");
		const std::string GFX_SWAP_TABLE("TileSwaps");
		const std::string GFX_SWAP_SECTION("GfxSwapSection");
		const std::string LIFESTOCK_SOLD_FLAGS("LifestockSoldFlags");
		const std::string LIFESTOCK_SOLD_FLAGS_SECTION("LifestockSoldFlagsSection");
		const std::string BIG_TREE_LOCATIONS("BigTreeLocations");
		const std::string BIG_TREE_LOCATIONS_SECTION("BigTreeLocationsSection");
		const std::string SHOP_LIST_LEA1("Shops");
		const std::string SHOP_LIST_LEA2("ShopsLea");
		const std::string SHOP_LIST_SECTION("ShopsSection");
		const std::string LANTERN_ROOM_FLAGS("LightableRooms");
		const std::string LANTERN_ROOM_FLAGS_SECTION("LightableRoomsSection");

		const std::string DOOR_OFFSET_TABLE_FILE("assets_packed/roomdata/misc/doorlookup.bin");
		const std::string DOOR_TABLE_FILE("assets_packed/roomdata/misc/doortable.bin");
		const std::string GFX_SWAP_FLAGS_FILE("assets_packed/roomdata/flagactions/roomgfxswapflags.bin");
		const std::string GFX_SWAP_LOCKED_DOOR_FLAGS_FILE("assets_packed/roomdata/flagactions/lockeddoorgfxswapflags.bin");
		const std::string GFX_SWAP_TREE_FLAGS_FILE("assets_packed/roomdata/flagactions/treewarpgfxswapflags.bin");
		const std::string GFX_SWAP_TABLE_FILE("assets_packed/roomdata/flagactions/tileswaps.bin");
		const std::string LIFESTOCK_SOLD_FLAGS_FILE("assets_packed/roomdata/flagactions/lifestocksoldflags.bin");
		const std::string BIG_TREE_LOCATIONS_FILE("assets_packed/roomdata/misc/bigtreelocs.bin");
		const std::string SHOP_LIST_FILE("assets_packed/roomdata/misc/shops.bin");
		const std::string LANTERN_ROOM_FILE("assets_packed/roomdata/flagactions/lanternflags.bin");

		const std::string ROOM_DATA_FILE("code/pointertables/maps/roomlist.asm");
		const std::string MAP_DATA_FILE("code/graphics/roommaps.asm");
		const std::string MAP_FILENAME_FORMAT_STRING("assets_packed/maps/%s.cmp");
		const std::string PALETTE_DATA_FILE("code/palettes/roompals.asm");
		const std::string PALETTE_FILENAME_FORMAT_STRING("assets_packed/graphics/roompalettes/%s.pal");
		const std::string WARP_FILENAME("assets_packed/roomdata/warps/exits.bin");
		const std::string FALL_DEST_FILENAME("assets_packed/roomdata/warps/roomfalldests.bin");
		const std::string CLIMB_DEST_FILENAME("assets_packed/roomdata/warps/roomclimbdests.bin");
		const std::string TRANSITION_FILENAME("assets_packed/roomdata/flagactions/roomtransitions.bin");
		const std::string PALETTE_LAVA_FILENAME("assets_packed/graphics/miscpalettes/lavapalette.pal");
		const std::string PALETTE_WARP_FILENAME("assets_packed/graphics/miscpalettes/kazaltwarp.pal");
		const std::string PALETTE_LANTERN_FILENAME("assets_packed/graphics/miscpalettes/labrynthlit.pal");
		const std::string ROOM_VISIT_FLAGS_FILE("assets_packed/roomdata/flagactions/roomvisitflags.bin");
		const std::string CHEST_CONTENTS_FILENAME("assets_packed/roomdata/chests/chestcontents.bin");
		const std::string CHEST_OFFSETS_FILENAME("assets_packed/roomdata/chests/chestoffsets.bin");
	}

} // namespace RomOffsets

#ifndef _SPRITE_DATA_H_
#define _SPRITE_DATA_H_

#include "DataManager.h"
#include "DataTypes.h"
#include <set>
#include <Entity.h>

class SpriteData : public DataManager
{
public:
	SpriteData(const filesystem::path& asm_file);
	SpriteData(const Rom& rom);

	virtual ~SpriteData();

	virtual bool Save(const filesystem::path& dir);
	virtual bool Save();

	virtual bool HasBeenModified() const;
	virtual void RefreshPendingWrites(const Rom& rom);

	bool IsEntity(uint8_t id) const;
	bool IsSprite(uint8_t id) const;
	bool IsItem(uint8_t sprite_id) const;
	std::string GetSpriteName(uint8_t id) const;
	uint8_t GetSpriteId(const std::string& name) const;
	uint8_t GetSpriteFromEntity(uint8_t id) const;
	std::vector<uint8_t> GetEntitiesFromSprite(uint8_t id) const;

	std::pair<uint8_t, uint8_t> GetSpriteHitbox(uint8_t id) const;
	void SetSpriteHitbox(uint8_t id, uint8_t height, uint8_t base);


	uint32_t GetSpriteAnimationCount(uint8_t id) const;
	std::vector<std::string> GetSpriteAnimations(uint8_t id) const;
	std::vector<std::string> GetSpriteAnimations(const std::string& name) const;
	uint32_t GetSpriteFrameCount(uint8_t id) const;
	std::vector<std::string> GetSpriteFrames(uint8_t id) const;
	std::vector<std::string> GetSpriteFrames(const std::string& name) const;
	std::shared_ptr<SpriteFrameEntry> GetDefaultEntityFrame(uint8_t id) const;
	std::shared_ptr<SpriteFrameEntry> GetSpriteFrame(const std::string& name) const;
	std::shared_ptr<SpriteFrameEntry> GetSpriteFrame(uint8_t id, uint8_t frame) const;
	std::shared_ptr<SpriteFrameEntry> GetSpriteFrame(uint8_t id, uint8_t anim, uint8_t frame) const;
	std::shared_ptr<SpriteFrameEntry> GetSpriteFrame(const std::string& anim_name, uint8_t frame) const;
	uint32_t GetSpriteAnimationFrameCount(uint8_t id, uint8_t anim_id) const;
	uint32_t GetSpriteAnimationFrameCount(const std::string& name) const;
	std::vector<std::string> GetSpriteAnimationFrames(uint8_t id, uint8_t anim_id) const;
	std::vector<std::string> GetSpriteAnimationFrames(const std::string& anim) const;
	std::vector<std::string> GetSpriteAnimationFrames(const std::string& name, uint8_t anim_id) const;

	std::vector<Entity> GetRoomEntities(uint16_t room) const;
	void SetRoomEntities(uint16_t room, const std::vector<Entity>& entities);

	const std::map<std::string, std::shared_ptr<PaletteEntry>>& GetAllPalettes() const;
	std::shared_ptr<PaletteEntry> GetPalette(const std::string& name) const;
	std::shared_ptr<Palette> GetSpritePalette(int lo, int hi = -1) const;
	std::shared_ptr<Palette> GetSpritePalette(uint8_t idx) const;
	uint8_t GetLoPaletteCount() const;
	std::shared_ptr<PaletteEntry> GetLoPalette(uint8_t idx) const;
	uint8_t GetHiPaletteCount() const;
	std::shared_ptr<PaletteEntry> GetHiPalette(uint8_t idx) const;
	uint8_t GetProjectile1PaletteCount() const;
	std::shared_ptr<PaletteEntry> GetProjectile1Palette(uint8_t idx) const;
	uint8_t GetProjectile2PaletteCount() const;
	std::shared_ptr<PaletteEntry> GetProjectile2Palette(uint8_t idx) const;

protected:
	virtual void CommitAllChanges();
private:
	bool LoadAsmFilenames();
	void SetDefaultFilenames();
	bool CreateDirectoryStructure(const filesystem::path& dir);
	void InitCache();

	ByteVector SerialisePaletteLUT() const;
	void DeserialisePaletteLUT(const ByteVector& bytes);
	ByteVector SerialisePalArray(const std::vector<std::shared_ptr<PaletteEntry>>& pals) const;
	std::vector<std::shared_ptr<PaletteEntry>> DeserialisePalArray(const ByteVector& bytes, const std::string& name,
		const filesystem::path& path, Palette::Type type, bool unique_path = false);
	void DeserialiseRoomEntityTable(const ByteVector& offsets, const ByteVector& bytes);
	std::pair<ByteVector, ByteVector> SerialiseRoomEntityTable() const;

	bool AsmLoadSpriteFrames();
	bool AsmLoadSpritePointers();
	bool AsmLoadSpritePalettes();
	bool AsmLoadSpriteData();

	bool RomLoadSpriteFrames(const Rom& rom);
	bool RomLoadSpritePalettes(const Rom& rom);
	bool RomLoadSpriteData(const Rom& rom);

	bool AsmSaveSpriteFrames(const filesystem::path& dir);
	bool AsmSaveSpritePointers(const filesystem::path& dir);
	bool AsmSaveSpritePalettes(const filesystem::path& dir);
	bool AsmSaveSpriteData(const filesystem::path& dir);

	bool RomPrepareInjectSpriteFrames(const Rom& rom);
	bool RomPrepareInjectSpritePalettes(const Rom& rom);
	bool RomPrepareInjectSpriteData(const Rom& rom);

	filesystem::path m_palette_data_file;
	filesystem::path m_palette_lut_file;
	filesystem::path m_proj1_pal_file;
	filesystem::path m_proj2_pal_file;
	filesystem::path m_sprite_lut_file;
	filesystem::path m_sprite_anims_file;
	filesystem::path m_sprite_anim_frames_file;
	filesystem::path m_sprite_frames_data_file;
	filesystem::path m_item_properties_file;
	filesystem::path m_sprite_behaviour_offset_file;
	filesystem::path m_sprite_behaviour_table_file;
	filesystem::path m_unknown_sprite_lookup_file;
	filesystem::path m_sprite_visibility_flags_file;
	filesystem::path m_one_time_event_flags_file;
	filesystem::path m_room_clear_flags_file;
	filesystem::path m_locked_door_sprite_flags_file;
	filesystem::path m_permanent_switch_flags_file;
	filesystem::path m_sacred_tree_flags_file;
	filesystem::path m_sprite_gfx_idx_lookup_file;
	filesystem::path m_sprite_dimensions_lookup_file;
	filesystem::path m_room_sprite_table_offsets_file;
	filesystem::path m_enemy_stats_file;
	filesystem::path m_room_sprite_table_file;

	std::map<uint8_t, std::string> m_names;
	std::map<std::string, uint8_t> m_ids;
	std::map<uint8_t, std::set<std::string>> m_sprite_frames;

	std::map<uint8_t, uint16_t> m_sprite_mystery_data;
	std::map<uint8_t, uint16_t> m_sprite_mystery_data_orig;
	std::map<uint8_t, std::vector<std::string>> m_animations;
	std::map<uint8_t, std::vector<std::string>> m_animations_orig;
	std::map<std::string, std::vector<std::string>> m_animation_frames;
	std::map<std::string, std::vector<std::string>> m_animation_frames_orig;
	std::map<std::string, std::shared_ptr<SpriteFrameEntry>> m_frames;
	std::map<std::string, std::shared_ptr<SpriteFrameEntry>> m_frames_orig;

	std::vector<std::shared_ptr<PaletteEntry>> m_lo_palettes;
	std::vector<std::shared_ptr<PaletteEntry>> m_lo_palettes_orig;
	std::vector<std::shared_ptr<PaletteEntry>> m_hi_palettes;
	std::vector<std::shared_ptr<PaletteEntry>> m_hi_palettes_orig;
	std::vector<std::shared_ptr<PaletteEntry>> m_projectile1_palettes;
	std::vector<std::shared_ptr<PaletteEntry>> m_projectile1_palettes_orig;
	std::vector<std::shared_ptr<PaletteEntry>> m_projectile2_palettes;
	std::vector<std::shared_ptr<PaletteEntry>> m_projectile2_palettes_orig;
	std::map<std::string, std::shared_ptr<PaletteEntry>> m_palettes_by_name;

	std::map<uint8_t, std::shared_ptr<PaletteEntry>> m_lo_palette_lookup;
	std::map<uint8_t, std::shared_ptr<PaletteEntry>> m_lo_palette_lookup_orig;
	std::map<uint8_t, std::shared_ptr<PaletteEntry>> m_hi_palette_lookup;
	std::map<uint8_t, std::shared_ptr<PaletteEntry>> m_hi_palette_lookup_orig;

	std::vector<std::array<uint8_t, 4>> m_sprite_visibility_flags;
	std::vector<std::array<uint8_t, 4>> m_sprite_visibility_flags_orig;
	std::vector<std::array<uint8_t, 6>> m_one_time_event_flags;
	std::vector<std::array<uint8_t, 6>> m_one_time_event_flags_orig;
	std::vector<std::array<uint8_t, 4>> m_room_clear_flags;
	std::vector<std::array<uint8_t, 4>> m_room_clear_flags_orig;
	std::vector<std::array<uint8_t, 4>> m_locked_door_flags;
	std::vector<std::array<uint8_t, 4>> m_locked_door_flags_orig;
	std::vector<std::array<uint8_t, 4>> m_permanent_switch_flags;
	std::vector<std::array<uint8_t, 4>> m_permanent_switch_flags_orig;
	std::vector<std::array<uint8_t, 4>> m_sacred_tree_flags;
	std::vector<std::array<uint8_t, 4>> m_sacred_tree_flags_orig;

	std::map<uint8_t, uint8_t> m_sprite_to_entity_lookup;
	std::map<uint8_t, uint8_t> m_sprite_to_entity_lookup_orig;
	std::map<uint8_t, std::array<uint8_t, 2>> m_sprite_dimensions;
	std::map<uint8_t, std::array<uint8_t, 2>> m_sprite_dimensions_orig;
	std::map<uint8_t, std::array<uint8_t, 5>> m_enemy_stats;
	std::map<uint8_t, std::array<uint8_t, 5>> m_enemy_stats_orig;
	std::vector<std::array<uint8_t, 4>> m_item_properties;
	std::vector<std::array<uint8_t, 4>> m_item_properties_orig;
	std::map<uint8_t, uint8_t> m_unknown_entity_lookup;
	std::map<uint8_t, uint8_t> m_unknown_entity_lookup_orig;

	std::map<uint16_t, std::vector<Entity>> m_room_entities;
	std::map<uint16_t, std::vector<Entity>> m_room_entities_orig;

	std::vector<uint8_t> m_sprite_behaviours;
	std::vector<uint8_t> m_sprite_behaviours_orig;
	std::vector<uint8_t> m_sprite_behaviour_offsets;
	std::vector<uint8_t> m_sprite_behaviour_offsets_orig;
};

#endif // _SPRITE_DATA_H_

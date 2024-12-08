#ifndef _SPRITE_DATA_H_
#define _SPRITE_DATA_H_

#include <set>
#include <cstdint>
#include <landstalker/main/include/DataManager.h>
#include <landstalker/main/include/DataTypes.h>
#include <landstalker/rooms/include/Entity.h>
#include <landstalker/rooms/include/Flags.h>
#include <landstalker/behaviours/include/Behaviours.h>

class SpriteData : public DataManager
{
public:
	enum class FlagType
	{
		SPRITE_VISIBILITY
	};

	struct AnimationFlags
	{
		AnimationFlags();
		AnimationFlags(uint8_t packed);
		uint8_t Pack() const;
		bool IsDefault() const;
		bool operator==(const AnimationFlags& rhs) const;
		bool operator!=(const AnimationFlags& rhs) const;

		enum class IdleAnimationFrameCount
		{
			TWO_FRAMES = 0,
			ONE_FRAME = 1
		} idle_animation_frames;
		enum class IdleAnimationSource
		{
			USE_WALK_FRAMES = 0,
			DEDICATED = 1,
		} idle_animation_source;
		enum class JumpAnimationSource
		{
			USE_IDLE_FRAMES = 0,
			DEDICATED = 1,
		} jump_animation_source;
		enum class WalkAnimationFrameCount
		{
			FOUR_FRAMES = 0,
			TWO_FRAMES = 1,
		} walk_animation_frame_count;
		bool do_not_rotate;
		enum class TakeDamageAnimationSource
		{
			USE_IDLE_FRAMES = 0,
			DEDICATED = 1,
		} take_damage_animation_source;
		bool has_full_animations;
	};

	struct ItemProperties
	{
		uint8_t max_quantity;
		uint8_t verb;
		uint8_t equipment_index;
		uint16_t price;
		ItemProperties() : max_quantity(0), verb(12), equipment_index(0), price(0) {}
		ItemProperties(const std::array<uint8_t, 4>& elems);
		std::array<uint8_t, 4> Pack() const;
	};

	struct EnemyStats
	{
		uint8_t health;
		uint8_t defence;
		uint8_t attack;
		uint8_t gold_drop;
		uint8_t item_drop;
		enum class DropProbability : uint8_t
		{
			ONE_IN_64,
			ONE_IN_128,
			ONE_IN_256,
			ONE_IN_512,
			ONE_IN_1024,
			ONE_IN_2048,
			NO_DROP,
			GUARANTEED_DROP
		} drop_probability;
		EnemyStats() : health(0), defence(0), attack(0), gold_drop(0), item_drop(0), drop_probability(DropProbability::NO_DROP) {}
		EnemyStats(const std::array<uint8_t, 5>& elems);
		std::array<uint8_t, 5> Pack() const;
	};

	SpriteData(const std::filesystem::path& asm_file);
	SpriteData(const Rom& rom);

	virtual ~SpriteData();

	virtual bool Save(const std::filesystem::path& dir);
	virtual bool Save();

	virtual bool HasBeenModified() const;
	virtual void RefreshPendingWrites(const Rom& rom);

	static std::wstring GetEntityDisplayName(uint8_t id);
	static std::wstring GetSpriteDisplayName(uint8_t id);
	std::wstring GetSpriteAnimationDisplayName(uint8_t id, const std::string& name) const;
	std::wstring GetSpriteFrameDisplayName(uint8_t id, const std::string& name) const;
	static std::wstring GetSpriteLowPaletteDisplayName(uint8_t id);
	static std::wstring GetSpriteHighPaletteDisplayName(uint8_t id);
	static std::wstring GetBehaviourDisplayName(int behav_id);

	bool IsEntity(uint8_t id) const;
	bool IsSprite(uint8_t id) const;
	bool IsItem(uint8_t sprite_id) const;
	bool IsEntityItem(uint8_t entity_id) const;
	bool IsEntityEnemy(uint8_t entity_id) const;
	bool HasFrontAndBack(uint8_t id) const;
	bool CanRotate(uint8_t id) const;
	std::string GetSpriteName(uint8_t id) const;
	uint8_t GetSpriteId(const std::string& name) const;
	uint8_t GetSpriteFromEntity(uint8_t id) const;
	void SetEntitySprite(uint8_t entity, uint8_t sprite);
	bool EntityHasSprite(uint8_t id) const;
	std::vector<uint8_t> GetEntitiesFromSprite(uint8_t id) const;

	std::pair<uint8_t, uint8_t> GetSpriteHitbox(uint8_t id) const;
	std::pair<uint8_t, uint8_t> GetEntityHitbox(uint8_t id) const;
	void SetSpriteHitbox(uint8_t id, uint8_t base, uint8_t height);

	bool SpriteFrameExists(const std::string& name) const;
	void DeleteSpriteFrame(const std::string& name);
	void AddSpriteFrame(uint8_t sprite_id, const std::string& name);

	bool SpriteAnimationExists(const std::string& name) const;
	void DeleteSpriteAnimation(const std::string& name);
	void AddSpriteAnimation(uint8_t sprite_id, const std::string& name);
	void MoveSpriteAnimation(uint8_t sprite_id, const std::string& name, int pos);

	void DeleteSpriteAnimationFrame(const std::string& animation_name, int frame_id);
	void InsertSpriteAnimationFrame(const std::string& animation_name, int frame_id, const std::string& name);
	void ChangeSpriteAnimationFrame(const std::string& animation_name, int frame_id, const std::string& name);
	void MoveSpriteAnimationFrame(const std::string& animation_name, int old_pos, int new_pos);

	uint32_t GetSpriteAnimationCount(uint8_t id) const;
	std::vector<std::string> GetSpriteAnimations(uint8_t id) const;
	std::vector<std::string> GetSpriteAnimations(const std::string& name) const;
	uint32_t GetSpriteFrameCount(uint8_t id) const;
	std::vector<std::string> GetSpriteFrames(uint8_t id) const;
	std::vector<std::string> GetSpriteFrames(const std::string& name) const;
	int GetSpriteFrameId(uint8_t sprite_id, const std::string& name) const;
	int GetDefaultEntityAnimationId(uint8_t id) const;
	int GetDefaultEntityFrameId(uint8_t id) const;
	int GetDefaultAbsFrameId(uint8_t sprite_id) const;
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
	AnimationFlags GetSpriteAnimationFlags(uint8_t id) const;
	void SetSpriteAnimationFlags(uint8_t id, const AnimationFlags& flags);
	uint16_t GetSpriteVolume(uint8_t id) const;
	void SetSpriteVolume(uint8_t id, uint16_t val);

	std::vector<Entity> GetRoomEntities(uint16_t room) const;
	void SetRoomEntities(uint16_t room, const std::vector<Entity>& entities);
	std::vector<EntityFlag> GetEntityVisibilityFlagsForRoom(uint16_t room);
	void SetEntityVisibilityFlagsForRoom(uint16_t room, const std::vector<EntityFlag>& data);
	std::vector<OneTimeEventFlag> GetOneTimeEventFlagsForRoom(uint16_t room);
	void SetOneTimeEventFlagsForRoom(uint16_t room, const std::vector<OneTimeEventFlag>& data);
	std::vector<RoomClearFlag> GetMultipleEntityHideFlagsForRoom(uint16_t room);
	void SetMultipleEntityHideFlagsForRoom(uint16_t room, const std::vector<RoomClearFlag>& data);
	std::vector<RoomClearFlag> GetLockedDoorFlagsForRoom(uint16_t room);
	void SetLockedDoorFlagsForRoom(uint16_t room, const std::vector<RoomClearFlag>& data);
	std::vector<RoomClearFlag> GetPermanentSwitchFlagsForRoom(uint16_t room);
	void SetPermanentSwitchFlagsForRoom(uint16_t room, const std::vector<RoomClearFlag>& data);
	std::vector<SacredTreeFlag> GetSacredTreeFlagsForRoom(uint16_t room);
	void SetSacredTreeFlagsForRoom(uint16_t room, const std::vector<SacredTreeFlag>& data);

	const std::map<std::string, std::shared_ptr<PaletteEntry>>& GetAllPalettes() const;
	std::shared_ptr<PaletteEntry> GetPalette(const std::string& name) const;
	std::shared_ptr<Palette> GetSpritePalette(int lo, int hi = -1) const;
	std::shared_ptr<Palette> GetEntityPalette(uint8_t idx) const;
	void SetEntityPalette(uint8_t entity, int lo, int hi);
	std::pair<int, int> GetEntityPaletteIdxs(uint8_t idx) const;
	uint8_t GetLoPaletteCount() const;
	std::shared_ptr<PaletteEntry> GetLoPalette(uint8_t idx) const;
	uint8_t GetHiPaletteCount() const;
	std::shared_ptr<PaletteEntry> GetHiPalette(uint8_t idx) const;
	uint8_t GetProjectile1PaletteCount() const;
	std::shared_ptr<PaletteEntry> GetProjectile1Palette(uint8_t idx) const;
	uint8_t GetProjectile2PaletteCount() const;
	std::shared_ptr<PaletteEntry> GetProjectile2Palette(uint8_t idx) const;

	ItemProperties GetItemProperties(uint8_t entity_index) const;
	void SetItemProperties(uint8_t entity_index, const ItemProperties& props);
	EnemyStats GetEnemyStats(uint8_t entity_index) const;
	void SetEnemyStats(uint8_t entity_index, const EnemyStats& stats);
	void ClearEnemyStats(uint8_t entity_index);

	std::map<int, std::string> GetScriptNames() const;
	std::pair<std::string, std::vector<Behaviours::Command>> GetScript(int id) const;
	void SetScript(int id, const std::string& name, const std::vector<Behaviours::Command>& cmds);
	void SetScript(int id, const std::vector<Behaviours::Command>& cmds);

protected:
	virtual void CommitAllChanges();
private:
	bool LoadAsmFilenames();
	void SetDefaultFilenames();
	bool CreateDirectoryStructure(const std::filesystem::path& dir);
	void InitCache();

	ByteVector SerialisePaletteLUT() const;
	void DeserialisePaletteLUT(const ByteVector& bytes);
	ByteVector SerialisePalArray(const std::vector<std::shared_ptr<PaletteEntry>>& pals) const;
	std::vector<std::shared_ptr<PaletteEntry>> DeserialisePalArray(const ByteVector& bytes, const std::string& name,
		const std::filesystem::path& path, Palette::Type type, bool unique_path = false);
	void DeserialiseRoomEntityTable(const ByteVector& offsets, const ByteVector& bytes);
	std::pair<ByteVector, ByteVector> SerialiseRoomEntityTable() const;

	bool AsmLoadSpriteFrames();
	bool AsmLoadSpritePointers();
	bool AsmLoadSpritePalettes();
	bool AsmLoadSpriteData();

	bool RomLoadSpriteFrames(const Rom& rom);
	bool RomLoadSpritePalettes(const Rom& rom);
	bool RomLoadSpriteData(const Rom& rom);

	bool AsmSaveSpriteFrames(const std::filesystem::path& dir);
	bool AsmSaveSpritePointers(const std::filesystem::path& dir);
	bool AsmSaveSpritePalettes(const std::filesystem::path& dir);
	bool AsmSaveSpriteData(const std::filesystem::path& dir);

	bool RomPrepareInjectSpriteFrames(const Rom& rom);
	bool RomPrepareInjectSpritePalettes(const Rom& rom);
	bool RomPrepareInjectSpriteData(const Rom& rom);

	std::filesystem::path m_palette_data_file;
	std::filesystem::path m_palette_lut_file;
	std::filesystem::path m_proj1_pal_file;
	std::filesystem::path m_proj2_pal_file;
	std::filesystem::path m_sprite_lut_file;
	std::filesystem::path m_sprite_anims_file;
	std::filesystem::path m_sprite_anim_frames_file;
	std::filesystem::path m_sprite_frames_data_file;
	std::filesystem::path m_item_properties_file;
	std::filesystem::path m_sprite_behaviour_offset_file;
	std::filesystem::path m_sprite_behaviour_table_file;
	std::filesystem::path m_sprite_anim_flags_lookup_file;
	std::filesystem::path m_sprite_visibility_flags_file;
	std::filesystem::path m_one_time_event_flags_file;
	std::filesystem::path m_room_clear_flags_file;
	std::filesystem::path m_locked_door_sprite_flags_file;
	std::filesystem::path m_permanent_switch_flags_file;
	std::filesystem::path m_sacred_tree_flags_file;
	std::filesystem::path m_sprite_gfx_idx_lookup_file;
	std::filesystem::path m_sprite_dimensions_lookup_file;
	std::filesystem::path m_room_sprite_table_offsets_file;
	std::filesystem::path m_enemy_stats_file;
	std::filesystem::path m_room_sprite_table_file;

	std::map<uint8_t, std::string> m_names;
	std::map<std::string, uint8_t> m_ids;
	std::map<uint8_t, std::set<std::string>> m_sprite_frames;

	std::map<uint8_t, uint16_t> m_sprite_volume;
	std::map<uint8_t, uint16_t> m_sprite_volume_orig;
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

	std::vector<EntityFlag> m_sprite_visibility_flags;
	std::vector<EntityFlag> m_sprite_visibility_flags_orig;
	std::vector<OneTimeEventFlag> m_one_time_event_flags;
	std::vector<OneTimeEventFlag> m_one_time_event_flags_orig;
	std::vector<RoomClearFlag> m_room_clear_flags;
	std::vector<RoomClearFlag> m_room_clear_flags_orig;
	std::vector<RoomClearFlag> m_locked_door_flags;
	std::vector<RoomClearFlag> m_locked_door_flags_orig;
	std::vector<RoomClearFlag> m_permanent_switch_flags;
	std::vector<RoomClearFlag> m_permanent_switch_flags_orig;
	std::vector<SacredTreeFlag> m_sacred_tree_flags;
	std::vector<SacredTreeFlag> m_sacred_tree_flags_orig;

	std::map<uint8_t, uint8_t> m_sprite_to_entity_lookup;
	std::map<uint8_t, uint8_t> m_sprite_to_entity_lookup_orig;
	std::map<uint8_t, std::array<uint8_t, 2>> m_sprite_dimensions;
	std::map<uint8_t, std::array<uint8_t, 2>> m_sprite_dimensions_orig;
	std::map<uint8_t, std::array<uint8_t, 5>> m_enemy_stats;
	std::map<uint8_t, std::array<uint8_t, 5>> m_enemy_stats_orig;
	std::vector<std::array<uint8_t, 4>> m_item_properties;
	std::vector<std::array<uint8_t, 4>> m_item_properties_orig;
	std::map<uint8_t, AnimationFlags> m_sprite_animation_flags;
	std::map<uint8_t, AnimationFlags> m_sprite_animation_flags_orig;

	std::map<uint16_t, std::vector<Entity>> m_room_entities;
	std::map<uint16_t, std::vector<Entity>> m_room_entities_orig;

	std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>> m_sprite_behaviours;
	std::map<int, std::pair<std::string, std::vector<Behaviours::Command>>> m_sprite_behaviours_orig;
};

#endif // _SPRITE_DATA_H_

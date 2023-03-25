#ifndef _SPRITE_DATA_H_
#define _SPRITE_DATA_H_

#include "DataManager.h"
#include "DataTypes.h"

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

protected:
	virtual void CommitAllChanges();
private:
	bool LoadAsmFilenames();
	void SetDefaultFilenames();
	bool CreateDirectoryStructure(const filesystem::path& dir);
	void InitCache();

	bool AsmLoadSpriteFrames();
	bool AsmLoadSpritePointers();

	bool RomLoadSpriteFrames(const Rom& rom);

	bool AsmSaveSpriteFrames(const filesystem::path& dir);
	bool AsmSaveSpritePointers(const filesystem::path& dir);

	bool RomPrepareInjectSpriteFrames(const Rom& rom);

	filesystem::path m_palette_data_file;
	filesystem::path m_palette_lut_file;
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

	std::map<uint8_t, uint16_t> m_sprite_mystery_data;
	std::map<uint8_t, uint16_t> m_sprite_mystery_data_orig;
	std::map<uint8_t, std::vector<std::string>> m_animations;
	std::map<uint8_t, std::vector<std::string>> m_animations_orig;
	std::map<std::string, std::vector<std::string>> m_animation_frames;
	std::map<std::string, std::vector<std::string>> m_animation_frames_orig;
	std::map<std::string, std::shared_ptr<SpriteFrameEntry>> m_frames;
	std::map<std::string, std::shared_ptr<SpriteFrameEntry>> m_frames_orig;
};

#endif // _SPRITE_DATA_H_

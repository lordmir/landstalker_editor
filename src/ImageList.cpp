#include <Icons.h>
#include <ImageList.h>

ImageList::ImageList(bool img32x32)
	: wxImageList(img32x32 ? 32 : 16, img32x32 ? 32 : 16, true)
{
	if (img32x32)
	{
		m_images["axe_magic"] = wxBITMAP_PNG_FROM_DATA(axemagic_32x32);
		m_images["blue_ribbon"] = wxBITMAP_PNG_FROM_DATA(blueribbon_32x32);
		m_images["bell"] = wxBITMAP_PNG_FROM_DATA(bell_32x32);
		m_images["broad_sword"] = wxBITMAP_PNG_FROM_DATA(broadsword_32x32);
		m_images["chest"] = wxBITMAP_PNG_FROM_DATA(chest_32x32);
		m_images["death_statue"] = wxBITMAP_PNG_FROM_DATA(deathstatue_32x32);
		m_images["einstein_whistle"] = wxBITMAP_PNG_FROM_DATA(einsteinwhistle_32x32);
		m_images["key"] = wxBITMAP_PNG_FROM_DATA(key_32x32);
		m_images["record_book"] = wxBITMAP_PNG_FROM_DATA(recordbook_32x32);
		m_images["switch_off"] = wxBITMAP_PNG_FROM_DATA(switch1off_32x32);
		m_images["switch_on"] = wxBITMAP_PNG_FROM_DATA(switch1on_32x32);
	}
	else
	{
		m_images["alpha"] = wxBITMAP_PNG_FROM_DATA(alpha_16x16);
		m_images["append_tile"] = wxBITMAP_PNG_FROM_DATA(append_tile_16x16);
		m_images["ats"] = wxBITMAP_PNG_FROM_DATA(ats_16x16);
		m_images["big_tiles"] = wxBITMAP_PNG_FROM_DATA(bigtiles_16x16);
		m_images["chest16"] = wxBITMAP_PNG_FROM_DATA(chest16_16x16);
		m_images["copy"] = wxBITMAP_PNG_FROM_DATA(COPY_16x16);
		m_images["cut"] = wxBITMAP_PNG_FROM_DATA(CUT_16x16);
		m_images["delete"] = wxBITMAP_PNG_FROM_DATA(DELETE_16x16);
		m_images["delete_column"] = wxBITMAP_PNG_FROM_DATA(delete_col_16x16);
		m_images["delete_row"] = wxBITMAP_PNG_FROM_DATA(delete_row_16x16);
		m_images["delete_tile"] = wxBITMAP_PNG_FROM_DATA(delete_tile_16x16);
		m_images["dialogue"] = wxBITMAP_PNG_FROM_DATA(dialogue_16x16);
		m_images["down"] = wxBITMAP_PNG_FROM_DATA(down_16x16);
		m_images["drawing"] = wxBITMAP_PNG_FROM_DATA(DRAWING_16x16);
		m_images["ehitbox"] = wxBITMAP_PNG_FROM_DATA(ehitbox_16x16);
		m_images["entity"] = wxBITMAP_PNG_FROM_DATA(entity_16x16);
		m_images["epanel"] = wxBITMAP_PNG_FROM_DATA(epanel_16x16);
		m_images["find"] = wxBITMAP_PNG_FROM_DATA(FIND_16x16);
		m_images["flags"] = wxBITMAP_PNG_FROM_DATA(flags_16x16);
		m_images["fonts"] = wxBITMAP_PNG_FROM_DATA(fonts_16x16);
		m_images["gridlines"] = wxBITMAP_PNG_FROM_DATA(gridlines_16x16);
		m_images["heightmap"] = wxBITMAP_PNG_FROM_DATA(heightmap_16x16);
		m_images["hflip"] = wxBITMAP_PNG_FROM_DATA(hflip_16x16);
		m_images["hm_cell_down"] = wxBITMAP_PNG_FROM_DATA(hm_cell_down_16x16);
		m_images["hm_cell_up"] = wxBITMAP_PNG_FROM_DATA(hm_cell_up_16x16);
		m_images["hm_delete_nesw"] = wxBITMAP_PNG_FROM_DATA(hm_delete_nesw_16x16);
		m_images["hm_delete_nwse"] = wxBITMAP_PNG_FROM_DATA(hm_delete_nwse_16x16);
		m_images["hm_insert_ne"] = wxBITMAP_PNG_FROM_DATA(hm_insert_ne_16x16);
		m_images["hm_insert_nw"] = wxBITMAP_PNG_FROM_DATA(hm_insert_nw_16x16);
		m_images["hm_insert_se"] = wxBITMAP_PNG_FROM_DATA(hm_insert_se_16x16);
		m_images["hm_insert_sw"] = wxBITMAP_PNG_FROM_DATA(hm_insert_sw_16x16);
		m_images["hm_npc_walkable"] = wxBITMAP_PNG_FROM_DATA(hm_npc_walkable_16x16);
		m_images["hm_nudge_ne"] = wxBITMAP_PNG_FROM_DATA(hm_nudge_ne_16x16);
		m_images["hm_nudge_nw"] = wxBITMAP_PNG_FROM_DATA(hm_nudge_nw_16x16);
		m_images["hm_nudge_se"] = wxBITMAP_PNG_FROM_DATA(hm_nudge_se_16x16);
		m_images["hm_nudge_sw"] = wxBITMAP_PNG_FROM_DATA(hm_nudge_sw_16x16);
		m_images["hm_player_walkable"] = wxBITMAP_PNG_FROM_DATA(hm_player_walkable_16x16);
		m_images["hm_raft_track"] = wxBITMAP_PNG_FROM_DATA(hm_raft_track_16x16);
		m_images["image"] = wxBITMAP_PNG_FROM_DATA(img_16x16);
		m_images["insert_after"] = wxBITMAP_PNG_FROM_DATA(insert_after_16x16);
		m_images["insert_before"] = wxBITMAP_PNG_FROM_DATA(insert_before_16x16);
		m_images["insert_column_after"] = wxBITMAP_PNG_FROM_DATA(insert_col_after_16x16);
		m_images["insert_column_before"] = wxBITMAP_PNG_FROM_DATA(insert_col_before_16x16);
		m_images["insert_row_after"] = wxBITMAP_PNG_FROM_DATA(insert_row_after_16x16);
		m_images["insert_row_before"] = wxBITMAP_PNG_FROM_DATA(insert_row_before_16x16);
		m_images["layers"] = wxBITMAP_PNG_FROM_DATA(layers_16x16);
		m_images["lightning"] = wxBITMAP_PNG_FROM_DATA(lightning_16x16);
		m_images["map"] = wxBITMAP_PNG_FROM_DATA(map_16x16);
		m_images["map_bg_active"] = wxBITMAP_PNG_FROM_DATA(map_bg_active_16x16);
		m_images["map_delete_nesw"] = wxBITMAP_PNG_FROM_DATA(map_delete_nesw_16x16);
		m_images["map_delete_nwse"] = wxBITMAP_PNG_FROM_DATA(map_delete_nwse_16x16);
		m_images["map_fg_active"] = wxBITMAP_PNG_FROM_DATA(map_fg_active_16x16);
		m_images["map_grid"] = wxBITMAP_PNG_FROM_DATA(map_grid_16x16);
		m_images["map_highlight_fg"] = wxBITMAP_PNG_FROM_DATA(map_highlight_fg_16x16);
		m_images["map_insert_ne"] = wxBITMAP_PNG_FROM_DATA(map_insert_ne_16x16);
		m_images["map_insert_nw"] = wxBITMAP_PNG_FROM_DATA(map_insert_nw_16x16);
		m_images["map_insert_se"] = wxBITMAP_PNG_FROM_DATA(map_insert_se_16x16);
		m_images["map_insert_sw"] = wxBITMAP_PNG_FROM_DATA(map_insert_sw_16x16);
		m_images["map_mode"] = wxBITMAP_PNG_FROM_DATA(map_mode_16x16);
		m_images["mcr"] = wxBITMAP_PNG_FROM_DATA(MCR_16x16);
		m_images["minus"] = wxBITMAP_PNG_FROM_DATA(minus_16x16);
		m_images["mouse"] = wxBITMAP_PNG_FROM_DATA(mouse_16x16);
		m_images["new"] = wxBITMAP_PNG_FROM_DATA(NEW_16x16);
		m_images["nigel"] = wxBITMAP_PNG_FROM_DATA(nigel_16x16);
		m_images["palette"] = wxBITMAP_PNG_FROM_DATA(pal_16x16);
		m_images["paste"] = wxBITMAP_PNG_FROM_DATA(PASTE_16x16);
		m_images["pencil"] = wxBITMAP_PNG_FROM_DATA(pencil_16x16);
		m_images["plus"] = wxBITMAP_PNG_FROM_DATA(plus_16x16);
		m_images["priority"] = wxBITMAP_PNG_FROM_DATA(priority_16x16);
		m_images["properties"] = wxBITMAP_PNG_FROM_DATA(PROP_16x16);
		m_images["redo"] = wxBITMAP_PNG_FROM_DATA(REDO_16x16);
		m_images["room"] = wxBITMAP_PNG_FROM_DATA(room_16x16);
		m_images["save"] = wxBITMAP_PNG_FROM_DATA(SAVE_16x16);
		m_images["sprite"] = wxBITMAP_PNG_FROM_DATA(sprite_16x16);
		m_images["string"] = wxBITMAP_PNG_FROM_DATA(string_16x16);
		m_images["swap"] = wxBITMAP_PNG_FROM_DATA(swap_16x16);
		m_images["tile_nums"] = wxBITMAP_PNG_FROM_DATA(tilenums_16x16);
		m_images["tileset"] = wxBITMAP_PNG_FROM_DATA(ts_16x16);
		m_images["undo"] = wxBITMAP_PNG_FROM_DATA(UNDO_16x16);
		m_images["up"] = wxBITMAP_PNG_FROM_DATA(up_16x16);
		m_images["vflip"] = wxBITMAP_PNG_FROM_DATA(vflip_16x16);
		m_images["warning"] = wxBITMAP_PNG_FROM_DATA(warning_16x16);
		m_images["warp"] = wxBITMAP_PNG_FROM_DATA(warp_16x16);
		m_images["wpanel"] = wxBITMAP_PNG_FROM_DATA(wpanel_16x16);
	}

	for (const auto& img : m_images)
	{
		m_idxs.insert({ img.first, m_idxs.size() });
		wxIcon ico;
		ico.CopyFromBitmap(img.second);
		this->Add(ico);
	}
}

wxBitmap& ImageList::GetImage(const std::string& name)
{
	auto result = m_images.find(name);
	if (result == m_images.end())
	{
		return wxNullBitmap;
	}
	return result->second;
}

const wxBitmap& ImageList::GetImage(const std::string& name) const
{
	auto result = m_images.find(name);
	if (result == m_images.end())
	{
		return wxNullBitmap;
	}
	return result->second;
}

int ImageList::GetIdx(const std::string& name) const
{
	if (m_idxs.find(name) == m_idxs.end()) return -1;
	return m_idxs.at(name);
}

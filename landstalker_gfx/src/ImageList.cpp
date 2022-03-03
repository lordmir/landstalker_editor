#include <Icons.h>
#include <ImageList.h>

ImageList::ImageList()
{
	m_images["alpha"]                  = wxBITMAP_PNG_FROM_DATA(alpha_16x16);
	m_images["append_tile"]            = wxBITMAP_PNG_FROM_DATA(append_tile_16x16);
	m_images["axe_magic"]              = wxBITMAP_PNG_FROM_DATA(axemagic_32x32);
	m_images["bell"]                   = wxBITMAP_PNG_FROM_DATA(bell_32x32);
	m_images["big_tiles"]              = wxBITMAP_PNG_FROM_DATA(bigtiles_16x16);
	m_images["blue_ribbon"]            = wxBITMAP_PNG_FROM_DATA(blueribbon_32x32);
	m_images["broad_sword"]            = wxBITMAP_PNG_FROM_DATA(broadsword_32x32);
	m_images["chest"]                  = wxBITMAP_PNG_FROM_DATA(chest_32x32);
	m_images["copy"]                   = wxBITMAP_PNG_FROM_DATA(COPY_16x16);
	m_images["cut"]                    = wxBITMAP_PNG_FROM_DATA(CUT_16x16);
	m_images["death_statue"]           = wxBITMAP_PNG_FROM_DATA(deathstatue_32x32);
	m_images["delete"]                 = wxBITMAP_PNG_FROM_DATA(DELETE_16x16);
	m_images["delete_column"]          = wxBITMAP_PNG_FROM_DATA(delete_col_16x16);
	m_images["delete_row"]             = wxBITMAP_PNG_FROM_DATA(delete_row_16x16);
	m_images["delete_tile"]            = wxBITMAP_PNG_FROM_DATA(delete_tile_16x16);
	m_images["drawing"]                = wxBITMAP_PNG_FROM_DATA(DRAWING_16x16);
	m_images["einstein_whistle"]       = wxBITMAP_PNG_FROM_DATA(einsteinwhistle_32x32);
	m_images["find"]                   = wxBITMAP_PNG_FROM_DATA(FIND_16x16);
	m_images["gridlines"]              = wxBITMAP_PNG_FROM_DATA(gridlines_16x16);
	m_images["image"]                  = wxBITMAP_PNG_FROM_DATA(img_16x16);
	m_images["insert_after"]           = wxBITMAP_PNG_FROM_DATA(insert_after_16x16);
	m_images["insert_before"]          = wxBITMAP_PNG_FROM_DATA(insert_before_16x16);
	m_images["insert_column_after"]    = wxBITMAP_PNG_FROM_DATA(insert_col_after_16x16);
	m_images["insert_column_before"]   = wxBITMAP_PNG_FROM_DATA(insert_col_before_16x16);
	m_images["insert_row_after"]       = wxBITMAP_PNG_FROM_DATA(insert_row_after_16x16);
	m_images["insert_row_before"]      = wxBITMAP_PNG_FROM_DATA(insert_row_before_16x16);
	m_images["key"]                    = wxBITMAP_PNG_FROM_DATA(key_32x32);
	m_images["mcr"]                    = wxBITMAP_PNG_FROM_DATA(MCR_16x16);
	m_images["new"]                    = wxBITMAP_PNG_FROM_DATA(NEW_16x16);
	m_images["palette"]                = wxBITMAP_PNG_FROM_DATA(pal_16x16);
	m_images["paste"]                  = wxBITMAP_PNG_FROM_DATA(PASTE_16x16);
	m_images["pencil"]                 = wxBITMAP_PNG_FROM_DATA(pencil_16x16);
	m_images["properties"]             = wxBITMAP_PNG_FROM_DATA(PROP_16x16);
	m_images["record_book"]            = wxBITMAP_PNG_FROM_DATA(recordbook_32x32);
	m_images["redo"]                   = wxBITMAP_PNG_FROM_DATA(REDO_16x16);
	m_images["room"]                   = wxBITMAP_PNG_FROM_DATA(room_16x16);
	m_images["save"]                   = wxBITMAP_PNG_FROM_DATA(SAVE_16x16);
	m_images["sprite"]                 = wxBITMAP_PNG_FROM_DATA(sprite_16x16);
	m_images["string"]                 = wxBITMAP_PNG_FROM_DATA(string_16x16);
	m_images["swap"]                   = wxBITMAP_PNG_FROM_DATA(swap_16x16);
	m_images["switch_off"]             = wxBITMAP_PNG_FROM_DATA(switch1off_32x32);
	m_images["switch_on"]              = wxBITMAP_PNG_FROM_DATA(switch1on_32x32);
	m_images["tile_nums"]              = wxBITMAP_PNG_FROM_DATA(tilenums_16x16);
	m_images["tileset"]                = wxBITMAP_PNG_FROM_DATA(ts_16x16);
	m_images["undo"]                   = wxBITMAP_PNG_FROM_DATA(UNDO_16x16);
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
	return GetImage(name);
}

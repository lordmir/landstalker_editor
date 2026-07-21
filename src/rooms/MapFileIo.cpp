#include <rooms/MapFileIo.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include <landstalker/3d_maps/MapToTmx.h>
#include <landstalker/main/ImageBuffer.h>
#include <landstalker/misc/Utils.h>

namespace
{

// The blockset image a TMX references is a fixed 16 x 64 grid of blocks.
constexpr int BLOCKSET_COLUMNS = 16;
constexpr int BLOCKSET_ROWS = 64;

constexpr std::array<const char*, 3> CSV_SUFFIXES = { "_background", "_foreground", "_heightmap" };

bool ReadTextFile(const std::string& path, std::string& contents)
{
	std::ifstream file(path, std::ios::in);
	if (!file)
	{
		return false;
	}
	std::ostringstream buffer;
	buffer << file.rdbuf();
	contents = buffer.str();
	return true;
}

bool WriteTextFile(const std::string& path, const std::string& contents)
{
	std::ofstream file(path, std::ios::out | std::ios::trunc);
	if (!file)
	{
		return false;
	}
	file << contents;
	return file.good();
}

}

bool MapFileIo::ImportCmp(const std::string& path, Landstalker::Tilemap3D& map)
{
	try
	{
		const auto bytes = Landstalker::ReadBytes(path);
		if (bytes.empty())
		{
			return false;
		}
		Landstalker::Tilemap3D loaded;
		if (loaded.Decode(bytes.data()) == 0 || loaded.GetWidth() == 0 || loaded.GetHeight() == 0)
		{
			return false;
		}
		// The compressed format carries no tile size, so keep the destination's.
		const auto tile_width = map.GetTileWidth();
		const auto tile_height = map.GetTileHeight();
		map = loaded;
		map.SetTileDims(tile_width, tile_height);
		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

bool MapFileIo::ImportCsv(const std::array<std::string, 3>& paths, Landstalker::Tilemap3D& map)
{
	std::string background, foreground, heightmap;
	if (!ReadTextFile(paths[0], background) ||
		!ReadTextFile(paths[1], foreground) ||
		!ReadTextFile(paths[2], heightmap))
	{
		return false;
	}
	return map.FromCsv(foreground, background, heightmap);
}

bool MapFileIo::ImportTmx(const std::string& path, Landstalker::Tilemap3D& map)
{
	try
	{
		return Landstalker::MapToTmx::ImportFromTmx(path, map);
	}
	catch (const std::exception&)
	{
		return false;
	}
}

bool MapFileIo::ExportCmp(const std::string& path, const std::shared_ptr<Landstalker::Tilemap3DEntry>& map)
{
	if (!map)
	{
		return false;
	}
	try
	{
		const auto bytes = map->GetBytes();
		if (!bytes || bytes->empty())
		{
			return false;
		}
		Landstalker::WriteBytes(*bytes, path);
		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

bool MapFileIo::ExportCsv(const std::array<std::string, 3>& paths, const Landstalker::Tilemap3D& map)
{
	std::string foreground, background, heightmap;
	if (!map.ToCsv(foreground, background, heightmap))
	{
		return false;
	}
	return WriteTextFile(paths[0], background) &&
		WriteTextFile(paths[1], foreground) &&
		WriteTextFile(paths[2], heightmap);
}

bool MapFileIo::ExportTmx(const std::string& path, const Landstalker::Tilemap3D& map, const std::string& blockset_path)
{
	try
	{
		return Landstalker::MapToTmx::ExportToTmx(path, map, blockset_path);
	}
	catch (const std::exception&)
	{
		return false;
	}
}

std::array<std::string, 3> MapFileIo::CsvPathsFor(const std::string& path)
{
	const std::filesystem::path base(path);
	const auto directory = base.parent_path();
	const auto stem = base.stem().string();
	std::array<std::string, 3> paths;
	for (std::size_t i = 0; i < paths.size(); ++i)
	{
		paths[i] = (directory / (stem + CSV_SUFFIXES[i] + ".csv")).string();
	}
	return paths;
}

bool MapFileIo::RenderBlocksetPng(const std::string& path, const std::shared_ptr<Landstalker::GameData>& gd, uint16_t roomnum)
{
	if (!gd || roomnum >= gd->GetRoomData()->GetRoomCount())
	{
		return false;
	}
	const auto room_data = gd->GetRoomData();
	const auto tileset_entry = room_data->GetTilesetForRoom(roomnum);
	const auto palette_entry = room_data->GetPaletteForRoom(roomnum);
	const auto blockset = room_data->GetCombinedBlocksetForRoom(roomnum);
	if (!tileset_entry || !palette_entry || !blockset)
	{
		return false;
	}
	const auto tileset = tileset_entry->GetData();
	const std::vector<std::shared_ptr<Landstalker::Palette>> palette{ palette_entry->GetData() };

	const int block_width = static_cast<int>(Landstalker::MapBlock::GetBlockWidth() * tileset->GetTileWidth());
	const int block_height = static_cast<int>(Landstalker::MapBlock::GetBlockHeight() * tileset->GetTileHeight());
	const int pixel_width = block_width * BLOCKSET_COLUMNS;
	const int pixel_height = block_height * BLOCKSET_ROWS;

	Landstalker::ImageBuffer buffer(pixel_width, pixel_height);
	std::size_t i = 0;
	for (int y = 0; y < pixel_height && i < blockset->size(); y += block_height)
	{
		for (int x = 0; x < pixel_width && i < blockset->size(); x += block_width, ++i)
		{
			buffer.InsertBlock(x, y, 0, blockset->at(i), *tileset);
		}
	}
	return buffer.WritePNG(path, palette, true);
}

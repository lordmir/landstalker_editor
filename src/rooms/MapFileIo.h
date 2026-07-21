#ifndef _MAP_FILE_IO_H_
#define _MAP_FILE_IO_H_

#include <array>
#include <memory>
#include <string>

#include <landstalker/3d_maps/Tilemap3D.h>
#include <landstalker/main/GameData.h>

// Import and export for the map interchange formats the room editor understands. These are
// thin file-handling wrappers around the conversions in liblandstalker: they read and write
// only tilemap and heightmap data, ignoring anything else a file holds, so the same helpers
// serve both "overwrite the current map" and "fill a brand new map". Every importer
// validates before it writes, and leaves the destination map untouched when it fails.
namespace MapFileIo
{
	// CSV paths are background, foreground, heightmap - the order the exporter uses.
	bool ImportCmp(const std::string& path, Landstalker::Tilemap3D& map);
	bool ImportCsv(const std::array<std::string, 3>& paths, Landstalker::Tilemap3D& map);
	bool ImportTmx(const std::string& path, Landstalker::Tilemap3D& map);

	// Writes the entry's own serialised bytes, so an exported .cmp matches what a save
	// would produce rather than being re-encoded independently.
	bool ExportCmp(const std::string& path, const std::shared_ptr<Landstalker::Tilemap3DEntry>& map);
	bool ExportCsv(const std::array<std::string, 3>& paths, const Landstalker::Tilemap3D& map);
	bool ExportTmx(const std::string& path, const Landstalker::Tilemap3D& map, const std::string& blockset_path);

	// Names the three layer files for a CSV set, e.g. "maps/Foo.csv" becomes
	// maps/Foo_background.csv, maps/Foo_foreground.csv and maps/Foo_heightmap.csv.
	std::array<std::string, 3> CsvPathsFor(const std::string& path);

	// Renders the blockset image a Tiled TMX references. Needs a room to supply the
	// tileset and palette, so it is only available for maps some room actually uses.
	bool RenderBlocksetPng(const std::string& path, const std::shared_ptr<Landstalker::GameData>& gd, uint16_t roomnum);
}

#endif // _MAP_FILE_IO_H_

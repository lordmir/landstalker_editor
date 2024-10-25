#ifndef _MAP_TO_TMX_H_
#define _MAP_TO_TMX_H_

#include <string>
#include <wx/xml/xml.h>
#include <landstalker/main/include/DataTypes.h>

class MapToTmx
{
public:
	static bool ImportFromTmx(const std::string& fname, Tilemap3D& map);
	static wxXmlDocument GenerateXmlDocument(const std::string& fname, const Tilemap3D& map, const std::string& blockset_filename);
	static bool ExportToTmx(const std::string& fname, const Tilemap3D& map, const std::string& blockset_filename);
};

#endif // _MAP_TO_TMX_H_
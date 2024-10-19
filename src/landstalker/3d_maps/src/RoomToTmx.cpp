#include <landstalker/3d_maps/include/RoomToTmx.h>
#include <landstalker/3d_maps/include/MapToTmx.h>
#include <sstream>
#include <iomanip>
#include <wx/filename.h>
#include <wx/xml/xml.h>

bool RoomToTmx::ExportToTmx(const std::string& fname, int roomnum, std::shared_ptr<RoomData> roomData, const std::string& blockset_filename)
{
	wxXmlDocument tmx = MapToTmx::GenerateXmlDocument(fname, *(roomData->GetMapForRoom(roomnum)->GetData()), blockset_filename);
	return tmx.Save(fname);
}

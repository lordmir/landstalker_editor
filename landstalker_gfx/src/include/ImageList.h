#ifndef _IMAGE_LIST_H_
#define _IMAGE_LIST_H_

#include <wx/wx.h>
#include <unordered_map>
#include <string>

class ImageList
{
public:
	ImageList();
	const wxBitmap& GetImage(const std::string& name) const;
	wxBitmap& GetImage(const std::string& name);
private:
	std::unordered_map<std::string, wxBitmap> m_images;
};

#endif // _IMAGE_LIST_H_
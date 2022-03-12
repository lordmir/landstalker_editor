#ifndef _IMAGE_LIST_H_
#define _IMAGE_LIST_H_

#include <wx/wx.h>
#include <unordered_map>
#include <string>

class ImageList : public wxImageList
{
public:
	ImageList();
	const wxBitmap& GetImage(const std::string& name) const;
	wxBitmap& GetImage(const std::string& name);
	int GetIdx(const std::string& name) const;
private:
	std::unordered_map<std::string, wxBitmap> m_images;
	std::unordered_map<std::string, int> m_idxs;
};

#endif // _IMAGE_LIST_H_
#ifndef _IMAGE_BUFFER_WX_H_
#define _IMAGE_BUFFER_WX_H_

#include <landstalker/main/ImageBuffer.h>
#include <wx/wx.h>

class ImageBufferWx : public Landstalker::ImageBuffer
{
public:
    ImageBufferWx() : Landstalker::ImageBuffer() {}
	ImageBufferWx(int width, int height) : Landstalker::ImageBuffer(width, height) {}
	virtual ~ImageBufferWx() = default;
	const ImageBuffer& Get() const { return *this; }
	ImageBuffer& Get() { return *this; }

	std::shared_ptr<wxBitmap> MakeBitmap(const std::vector<std::shared_ptr<Landstalker::Palette>>& pals, bool use_alpha = false, uint8_t low_pri_max_opacity = 0xFF, uint8_t high_pri_max_opacity = 0xFF) const;
	wxImage MakeImage(const std::vector<std::shared_ptr<Landstalker::Palette>>& pals, bool use_alpha = false, uint8_t low_pri_max_opacity = 0xFF, uint8_t high_pri_max_opacity = 0xFF) const;
};

#endif // _IMAGE_BUFFER_WX_H_
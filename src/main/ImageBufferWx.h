#ifndef _IMAGE_BUFFER_WX_H_
#define _IMAGE_BUFFER_WX_H_

#include <landstalker/main/ImageBuffer.h>
#include <wx/wx.h>
#include <functional>
#include <vector>

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

// Renders BGRA pixel data (the Palette::getBGRA format: red in the low byte, alpha in the top
// byte) into a native-resolution bitmap with alpha, ready for a single scaled blit. Drawing a
// zoomed tile as one DC rectangle per pixel costs a brush change plus a GDI call each and is
// orders of magnitude slower than blitting.
// `lightness` darkens (<100) or lightens (>100) the tile, for greyed-out cells.
wxBitmap MakeTileBitmap(const std::vector<uint32_t>& bgra_pixels, int width, int height, int lightness = 100);

// Writes the same BGRA pixel data into a region of an existing alpha-enabled wxImage, for
// building a whole grid of tiles into one bitmap up front instead of allocating a bitmap
// per tile per paint.
void WriteTileToImage(wxImage& img, int x, int y, const std::vector<uint32_t>& bgra_pixels, int width, int height, int lightness = 100);

// Shared pixel-drawing geometry for the canvas editors' shape tools.
enum class ShapeTool
{
	Line,
	RectangleOutline,
	RectangleFilled,
	CircleOutline,
	CircleFilled
};

// Calls `plot` for every pixel on the Bresenham line from a to b, both endpoints included.
// Also the pencil-stroke joiner: Windows coalesces mouse moves, so consecutive samples land
// several pixels apart and must be joined or fast strokes leave gaps.
void PlotShapeLine(const wxPoint& a, const wxPoint& b, const std::function<void(int, int)>& plot);

// The pixels making up a shape dragged from a to b: Bresenham lines, rectangle perimeters
// and fills, and ellipses inscribed in the drag rectangle (outline keeps the boundary
// pixels - those inside with at least one 4-neighbour outside).
std::vector<wxPoint> MakeShapeToolPoints(ShapeTool tool, const wxPoint& a, const wxPoint& b);

#endif // _IMAGE_BUFFER_WX_H_

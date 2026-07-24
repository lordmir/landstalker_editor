#include <main/ImageBufferWx.h>

#include <algorithm>

std::shared_ptr<wxBitmap> ImageBufferWx::MakeBitmap(const std::vector<std::shared_ptr<Landstalker::Palette>>& pals, bool use_alpha, uint8_t low_pri_max_opacity, uint8_t high_pri_max_opacity) const
{
    wxImage img = MakeImage(pals, use_alpha, low_pri_max_opacity, high_pri_max_opacity);
    return std::make_shared<wxBitmap>(img);
}

wxImage ImageBufferWx::MakeImage(const std::vector<std::shared_ptr<Landstalker::Palette>>& pals, bool use_alpha, uint8_t low_pri_max_opacity, uint8_t high_pri_max_opacity) const
{
    GetRGB(pals);
    wxImage img(GetWidth(), GetHeight(), const_cast<uint8_t*>(GetRGB(pals).data()), true);
    if (use_alpha)
    {
        img.SetAlpha(const_cast<uint8_t*>(GetAlpha(pals, low_pri_max_opacity, high_pri_max_opacity).data()), true);
    }
    return img;
}

wxBitmap MakeTileBitmap(const std::vector<uint32_t>& bgra_pixels, int width, int height, int lightness)
{
    wxImage img(width, height);
    img.SetAlpha();
    WriteTileToImage(img, 0, 0, bgra_pixels, width, height, lightness);
    return wxBitmap(img, 32);
}

void WriteTileToImage(wxImage& img, int x, int y, const std::vector<uint32_t>& bgra_pixels, int width, int height, int lightness)
{
    unsigned char* rgb = img.GetData();
    unsigned char* alpha = img.GetAlpha();
    const int img_width = img.GetWidth();
    const int img_height = img.GetHeight();
    for (int py = 0; py < height; ++py)
    {
        for (int px = 0; px < width; ++px)
        {
            const std::size_t src = static_cast<std::size_t>(px) + static_cast<std::size_t>(py) * width;
            const int dx = x + px;
            const int dy = y + py;
            if ((src >= bgra_pixels.size()) || (dx < 0) || (dx >= img_width) || (dy < 0) || (dy >= img_height))
            {
                continue;
            }
            const uint32_t c = bgra_pixels[src];
            wxColour colour(c & 0xFFFFFF);
            if (lightness != 100)
            {
                colour = colour.ChangeLightness(lightness);
            }
            const std::size_t dst = static_cast<std::size_t>(dx) + static_cast<std::size_t>(dy) * img_width;
            rgb[dst * 3] = colour.Red();
            rgb[dst * 3 + 1] = colour.Green();
            rgb[dst * 3 + 2] = colour.Blue();
            alpha[dst] = c >> 24;
        }
    }
}
void PlotShapeLine(const wxPoint& a, const wxPoint& b, const std::function<void(int, int)>& plot)
{
    int x0 = a.x;
    int y0 = a.y;
    const int dx = std::abs(b.x - x0);
    const int dy = -std::abs(b.y - y0);
    const int sx = (x0 < b.x) ? 1 : -1;
    const int sy = (y0 < b.y) ? 1 : -1;
    int err = dx + dy;
    while (true)
    {
        plot(x0, y0);
        if ((x0 == b.x) && (y0 == b.y))
        {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

std::vector<wxPoint> MakeShapeToolPoints(ShapeTool tool, const wxPoint& a, const wxPoint& b)
{
    std::vector<wxPoint> pts;
    const int x0 = std::min(a.x, b.x);
    const int x1 = std::max(a.x, b.x);
    const int y0 = std::min(a.y, b.y);
    const int y1 = std::max(a.y, b.y);
    switch (tool)
    {
    case ShapeTool::Line:
        PlotShapeLine(a, b, [&pts](int x, int y) { pts.emplace_back(x, y); });
        break;
    case ShapeTool::RectangleOutline:
        for (int x = x0; x <= x1; ++x)
        {
            pts.emplace_back(x, y0);
            if (y1 != y0)
            {
                pts.emplace_back(x, y1);
            }
        }
        for (int y = y0 + 1; y < y1; ++y)
        {
            pts.emplace_back(x0, y);
            if (x1 != x0)
            {
                pts.emplace_back(x1, y);
            }
        }
        break;
    case ShapeTool::RectangleFilled:
        for (int y = y0; y <= y1; ++y)
        {
            for (int x = x0; x <= x1; ++x)
            {
                pts.emplace_back(x, y);
            }
        }
        break;
    case ShapeTool::CircleOutline:
    case ShapeTool::CircleFilled:
    {
        const double cx = (x0 + x1) / 2.0;
        const double cy = (y0 + y1) / 2.0;
        const double rx = std::max(0.5, (x1 - x0) / 2.0);
        const double ry = std::max(0.5, (y1 - y0) / 2.0);
        const auto inside = [&](int x, int y)
        {
            const double nx = (x - cx) / rx;
            const double ny = (y - cy) / ry;
            return (nx * nx + ny * ny) <= 1.0;
        };
        for (int y = y0; y <= y1; ++y)
        {
            for (int x = x0; x <= x1; ++x)
            {
                if (!inside(x, y))
                {
                    continue;
                }
                if ((tool == ShapeTool::CircleFilled) ||
                    !inside(x - 1, y) || !inside(x + 1, y) || !inside(x, y - 1) || !inside(x, y + 1))
                {
                    pts.emplace_back(x, y);
                }
            }
        }
        break;
    }
    default:
        break;
    }
    return pts;
}

#include <main/ImageBufferWx.h>

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
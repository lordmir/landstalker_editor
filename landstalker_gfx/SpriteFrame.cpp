#include "SpriteFrame.h"
#include <vector>
#include "Rom.h"
#include "LZ77.h"
#include "Utils.h"

SpriteFrame::SpriteFrame(const uint8_t* src)
{
	size_t tile_idx = 0;
	do
	{
		int y = ((*src & 0x7C) << 1);
		if (y > 0x80)
		{
			y -= 0x100;
		}
		size_t w = (*src & 0x03) + 1;
		int x = ((*++src & 0x7C) << 1);
		if (x > 0x80)
		{
			x -= 0x100;
		}
		size_t h = (*src & 0x03) + 1;
		m_subsprites.push_back({ x,y,w,h, tile_idx });
		tile_idx += w * h;
	} while ((*src++ & 0x80) == 0);

	for (const auto subs : m_subsprites)
	{
		std::ostringstream ss;
		ss << "Sprite T:" << subs.tile_idx << " X:" << subs.x << " Y:" << subs.y << " W:" << subs.w << " H:" << subs.h << std::endl;
		Debug(ss.str().c_str());
	}
	std::ostringstream ss;
	ss << "Total tiles to load: " << tile_idx << std::endl;
	Debug(ss.str().c_str());
	std::vector<uint8_t> sprite_gfx(tile_idx * 32, 0);
	auto dest_it = sprite_gfx.begin();

	uint16_t command;
	uint8_t ctrl;
	uint16_t count;
	do
	{
		command = (src[0] << 8) | src[1];
		ctrl = command >> 12;
		count = command & 0xFFF;
		src += 2;

		if ((ctrl & 0x08) > 0)
		{
			std::ostringstream ss;
			ss << "Insert " << count << " zero words." << std::endl;
			Debug(ss.str().c_str());
			dest_it += count * 2;
		}
		else if ((ctrl & 0x02) > 0)
		{
			std::ostringstream ss;
			size_t elen = 0;
			src += LZ77::Decode(src, count, &(*dest_it), elen);
			ss << "Copy " << count << " compressed bytes, " << elen << " bytes decompressed." << std::endl;
			Debug(ss.str().c_str());
			dest_it += elen;
		}
		else
		{
			std::ostringstream ss;
			ss << "Copy " << count << " words directly." << std::endl;
			Debug(ss.str().c_str());
			std::copy(src, src + count * 2, dest_it);
			dest_it += count * 2;
			src += count * 2;
		}
	} while ((ctrl & 0x04) == 0);

	m_sprite_gfx.setBits(sprite_gfx.data(), tile_idx);

	Debug("Done!");
}
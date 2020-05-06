#ifndef SPRITE_GRAPHIC_H
#define SPRITE_GRAPHIC_H

#include <vector>
#include <cstdint>

class SpriteGraphic
{
public:
	SpriteGraphic(size_t index);
	void AddAnimation(const std::vector<uint32_t>& frame_list);
	const size_t RetrieveFrameIdx(size_t animation, size_t frame) const;
	const size_t GetAnimationCount() const;
	const size_t GetFrameCount(size_t animation) const;
	const size_t GetIndex() const;
private:
	size_t m_index;
	std::vector< std::vector< uint32_t > > m_animation_frames;
};

#endif // SPRITE_GRAPHIC_H

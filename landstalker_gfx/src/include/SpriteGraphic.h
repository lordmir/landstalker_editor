#ifndef SPRITE_GRAPHIC_H
#define SPRITE_GRAPHIC_H

#include <vector>
#include <cstdint>

class SpriteGraphic
{
public:
	SpriteGraphic(std::size_t index);
	void AddAnimation(const std::vector<uint32_t>& frame_list);
	const std::size_t RetrieveFrameIdx(std::size_t animation, std::size_t frame) const;
	const std::size_t GetAnimationCount() const;
	const std::size_t GetFrameCount(std::size_t animation) const;
	const std::size_t GetIndex() const;
private:
	std::size_t m_index;
	std::vector< std::vector< uint32_t > > m_animation_frames;
};

#endif // SPRITE_GRAPHIC_H

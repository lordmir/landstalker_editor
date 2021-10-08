#include "SpriteGraphic.h"

SpriteGraphic::SpriteGraphic(std::size_t index)
: m_index(index)
{
}

void SpriteGraphic::AddAnimation(const std::vector<uint32_t>& frame_list)
{
	m_animation_frames.push_back(frame_list);
}

const std::size_t SpriteGraphic::RetrieveFrameIdx(std::size_t animation, std::size_t frame) const
{
	return m_animation_frames[animation][frame];
}

const std::size_t SpriteGraphic::GetAnimationCount() const
{
	return m_animation_frames.size();
}

const std::size_t SpriteGraphic::GetFrameCount(std::size_t animation) const
{
	return m_animation_frames[animation].size();
}

const std::size_t SpriteGraphic::GetIndex() const
{
	return m_index;
}

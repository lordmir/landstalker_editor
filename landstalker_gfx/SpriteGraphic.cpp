#include "SpriteGraphic.h"

SpriteGraphic::SpriteGraphic(size_t index)
: m_index(index)
{
}

void SpriteGraphic::AddAnimation(const std::vector<uint32_t>& frame_list)
{
	m_animation_frames.push_back(frame_list);
}

const size_t SpriteGraphic::RetrieveFrameIdx(size_t animation, size_t frame) const
{
	return m_animation_frames[animation][frame];
}

const size_t SpriteGraphic::GetAnimationCount() const
{
	return m_animation_frames.size();
}

const size_t SpriteGraphic::GetFrameCount(size_t animation) const
{
	return m_animation_frames[animation].size();
}

const size_t SpriteGraphic::GetIndex() const
{
	return m_index;
}

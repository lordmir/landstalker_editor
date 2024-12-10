#ifndef _WARP_LIST_H_
#define _WARP_LIST_H_

#include <vector>
#include <cstdint>
#include <list>
#include <map>
#include <landstalker/main/include/Rom.h>
#include <filesystem>

class WarpList
{
public:
	struct Warp
	{
		enum class Type : uint8_t
		{
			NORMAL,
			STAIR_SE,
			STAIR_SW,
			UNKNOWN
		};

		uint16_t room1;
		uint8_t x1;
		uint8_t y1;
		uint16_t room2;
		uint8_t x2;
		uint8_t y2;
		uint8_t x_size;
		uint8_t y_size;
		Type type;

		Warp(const std::vector<uint8_t>& raw);
		Warp() : room1(0xFFFF), x1(0), y1(0), room2(0xFFFF), x2(0), y2(0), x_size(1), y_size(1), type(Type::NORMAL) {}
		std::vector<uint8_t> GetRaw() const;
		bool IsValid() const;

		bool operator==(const Warp& rhs) const;
		bool operator!=(const Warp& rhs) const;
	};

	struct Transition
	{
		Transition(uint16_t src, uint16_t dst, uint16_t flagp)
			: src_rm(src), dst_rm(dst), flag(flagp)
		{}

		Transition(uint16_t src)
			: src_rm(src), dst_rm(0), flag(0)
		{}

		uint16_t src_rm;
		uint16_t dst_rm;
		uint16_t flag;

		bool operator==(const Transition& rhs) const;
		bool operator!=(const Transition& rhs) const;
		bool operator<(const Transition& rhs) const;
		bool operator<=(const Transition& rhs) const;
		bool operator>(const Transition& rhs) const;
		bool operator>=(const Transition& rhs) const;
	};
	WarpList(const std::filesystem::path& warp_path, const std::filesystem::path& fall_dest_path, const std::filesystem::path& climb_dest_path, const std::filesystem::path& transition_path);
	WarpList(const Rom& rom);
	WarpList() = default;

	bool operator==(const WarpList& rhs) const;
	bool operator!=(const WarpList& rhs) const;

	std::vector<Warp> GetWarpsForRoom(uint16_t room) const;
	void UpdateWarpsForRoom(uint16_t room, const std::vector<Warp>& warps);
	bool HasFallDestination(uint16_t room) const;
	uint16_t GetFallDestination(uint16_t room) const;
	bool HasClimbDestination(uint16_t room) const;
	uint16_t GetClimbDestination(uint16_t room) const;
	std::vector<Transition> GetAllTransitionsForRoom(uint16_t room) const;
	std::vector<Transition> GetSrcTransitionsForRoom(uint16_t room) const;
	void SetSrcTransitionsForRoom(uint16_t room, const std::vector<Transition>& data);

	void SetHasFallDestination(uint16_t room, bool enabled);
	void SetFallDestination(uint16_t room, uint16_t dest);
	void SetHasClimbDestination(uint16_t room, bool enabled);
	void SetClimbDestination(uint16_t room, uint16_t dest);

	std::vector<uint8_t> GetWarpBytes() const;
	std::vector<uint8_t> GetFallBytes() const;
	std::vector<uint8_t> GetClimbBytes() const;
	std::vector<uint8_t> GetTransitionBytes() const;

private:
	void ProcessWarpList(const std::vector<uint8_t>& bytes);
	void ProcessRouteList(const std::vector<uint8_t>& bytes, std::map<uint16_t, uint16_t>& routes);
	void ProcessTransitionList(const std::vector<uint8_t>& bytes);

	std::vector<Warp> m_warps;
	std::map<uint16_t, uint16_t> m_fall_dests;
	std::map<uint16_t, uint16_t> m_climb_dests;
	std::vector<Transition> m_transitions;
};

#endif // _WARP_LIST_H_

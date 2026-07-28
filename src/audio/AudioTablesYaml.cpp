#include <audio/AudioTablesYaml.h>

#include <audio/YamlIo.h>

#include <stdexcept>
#include <string>

using Landstalker::AudioData;
using Landstalker::MusicData;

namespace
{
	int ReadInt(const YAML::Node& node, const std::string& context, int min, int max)
	{
		return ReadYamlInt(node, context, min, max);
	}

	// Applies one `<hex slot id>: <pool entry name>` map onto a slot map, resolving names against
	// the pool's entries.
	template <typename Pool>
	void ApplySlotSection(const YAML::Node& section, const std::string& section_name, const Pool& pool,
		std::size_t first_id, std::vector<std::size_t>& slot_map)
	{
		if (!section.IsDefined())
		{
			return;
		}
		if (!section.IsMap())
		{
			throw std::runtime_error("'" + section_name + "' should be a map of slot id to entry name");
		}
		for (const auto& entry : section)
		{
			const std::string context = section_name + "[" + entry.first.as<std::string>() + "]";
			const int id = ReadInt(entry.first, section_name + " key", 0, 0xFF);
			if (static_cast<std::size_t>(id) < first_id
				|| static_cast<std::size_t>(id) >= first_id + slot_map.size())
			{
				throw std::runtime_error(context + ": slot id out of range");
			}
			const std::string name = entry.second.as<std::string>();
			std::size_t pool_index = pool.size();
			for (std::size_t i = 0; i < pool.size(); ++i)
			{
				if (pool[i].name == name)
				{
					pool_index = i;
					break;
				}
			}
			if (pool_index == pool.size())
			{
				throw std::runtime_error(context + ": no pool entry named '" + name + "'");
			}
			slot_map[id - first_id] = pool_index;
		}
	}
}

void EmitPcmSampleTableYaml(YAML::Emitter& out, const std::vector<AudioData::PcmSample>& table)
{
	out << YAML::BeginMap;
	out << YAML::Key << "samples" << YAML::Value << YAML::BeginSeq;
	for (std::size_t i = 0; i < table.size(); ++i)
	{
		const auto& sample = table[i];
		out << YAML::BeginMap;
		out << YAML::Key << "id" << YAML::Value << static_cast<int>(i + 1);
		out << YAML::Key << "delay" << YAML::Value << static_cast<int>(sample.rate);
		out << YAML::Key << "bank" << YAML::Value << YAML::Hex << static_cast<int>(sample.bank) << YAML::Dec;
		out << YAML::Key << "length" << YAML::Value << static_cast<int>(sample.length);
		out << YAML::Key << "start_addr" << YAML::Value << YAML::Hex << static_cast<int>(sample.start_offset) << YAML::Dec;
		if (sample.reserved != 0)
		{
			out << YAML::Key << "reserved" << YAML::Value << YAML::Hex << static_cast<int>(sample.reserved) << YAML::Dec;
		}
		if (sample.reserved2 != 0)
		{
			out << YAML::Key << "reserved2" << YAML::Value << YAML::Hex << static_cast<int>(sample.reserved2) << YAML::Dec;
		}
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	out << YAML::EndMap;
}

std::vector<AudioData::PcmSample> PcmSampleTableFromYaml(const YAML::Node& root)
{
	const YAML::Node samples = root["samples"];
	if (!samples.IsDefined() || !samples.IsSequence())
	{
		throw std::runtime_error("expected a top-level 'samples' list");
	}
	std::vector<AudioData::PcmSample> table;
	for (std::size_t i = 0; i < samples.size(); ++i)
	{
		const YAML::Node& entry = samples[i];
		const std::string context = "samples[" + std::to_string(i) + "]";
		AudioData::PcmSample sample;
		sample.rate = static_cast<uint8_t>(ReadInt(entry["delay"], context + ".delay", 0, 255));
		sample.bank = static_cast<uint8_t>(ReadInt(entry["bank"], context + ".bank", 0, 255));
		sample.length = static_cast<uint16_t>(ReadInt(entry["length"], context + ".length", 0, 0xFFFF));
		sample.start_offset = static_cast<uint16_t>(ReadInt(entry["start_addr"], context + ".start_addr", 0, 0xFFFF));
		if (entry["reserved"].IsDefined())
		{
			sample.reserved = static_cast<uint8_t>(ReadInt(entry["reserved"], context + ".reserved", 0, 255));
		}
		if (entry["reserved2"].IsDefined())
		{
			sample.reserved2 = static_cast<uint8_t>(ReadInt(entry["reserved2"], context + ".reserved2", 0, 255));
		}
		table.push_back(sample);
	}
	return table;
}

void EmitBankMappingYaml(YAML::Emitter& out, const MusicData& md)
{
	const auto emit_section = [&](const char* key, const auto& pool, const std::vector<std::size_t>& slot_map,
		std::size_t first_id)
	{
		out << YAML::Key << key << YAML::Value << YAML::BeginMap;
		for (std::size_t i = 0; i < slot_map.size(); ++i)
		{
			out << YAML::Key << YAML::Hex << static_cast<int>(first_id + i) << YAML::Dec;
			out << YAML::Value << ((slot_map[i] < pool.size()) ? pool[slot_map[i]].name : std::string());
		}
		out << YAML::EndMap;
	};
	out << YAML::BeginMap;
	emit_section("music_slots", md.GetMusicTrackPool(), md.GetMusicSlotMap(), 0);
	emit_section("sfx_slots", md.GetSfxPool(), md.GetSfxSlotMap(), 0x41);
	out << YAML::EndMap;
}

void ApplyBankMappingFromYaml(const YAML::Node& root, const MusicData& md,
	std::vector<std::size_t>& music_slot_map, std::vector<std::size_t>& sfx_slot_map)
{
	if (!root["music_slots"].IsDefined() && !root["sfx_slots"].IsDefined())
	{
		throw std::runtime_error("expected a 'music_slots' and/or 'sfx_slots' map");
	}
	ApplySlotSection(root["music_slots"], "music_slots", md.GetMusicTrackPool(), 0, music_slot_map);
	ApplySlotSection(root["sfx_slots"], "sfx_slots", md.GetSfxPool(), 0x41, sfx_slot_map);
}

#include <audio/YmInstrumentYaml.h>

#include <audio/YamlIo.h>

#include <cstdio>
#include <stdexcept>
#include <string>

using Landstalker::MusicData;

namespace
{
	// Field name and byte-group offset for each of an operator's seven packed register bytes, in
	// the emit order used by the Python tool.
	constexpr std::pair<const char*, std::size_t> OPERATOR_FIELDS[7] = {
		{ "det_mult", 0 }, { "total_level", 4 }, { "ks_ar", 8 }, { "am_dr", 12 },
		{ "sr", 16 }, { "sl_rr", 20 }, { "ssg_eg", 24 }
	};

	// Operator key and its byte position within a four-byte register group. The bytes are stored
	// in YM2612 slot order (S1, S3, S2, S4), and the Python tool names/emits them in that same
	// order - op1, op3, op2, op4.
	constexpr std::pair<const char*, std::size_t> OPERATOR_KEYS[4] = {
		{ "op1", 0 }, { "op3", 1 }, { "op2", 2 }, { "op4", 3 }
	};

	std::string InstrumentLabel(std::size_t index)
	{
		char buf[16];
		std::snprintf(buf, sizeof(buf), "YM_INSTMT_%02zX", index);
		return buf;
	}

	uint8_t ReadByte(const YAML::Node& node, const std::string& context)
	{
		return static_cast<uint8_t>(ReadYamlInt(node, context, 0, 255));
	}
}

void EmitYmInstrumentsYaml(YAML::Emitter& out, const MusicData::YmInstrumentTable& table)
{
	out << YAML::BeginMap;
	out << YAML::Key << "ym_instruments" << YAML::Value << YAML::BeginMap;
	for (std::size_t i = 0; i < table.size(); ++i)
	{
		const auto& inst = table[i];
		out << YAML::Key << InstrumentLabel(i) << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "feedback_algo" << YAML::Value << YAML::Hex << static_cast<int>(inst[28]) << YAML::Dec;
		out << YAML::Key << "operators" << YAML::Value << YAML::BeginMap;
		for (const auto& [op_key, op_pos] : OPERATOR_KEYS)
		{
			out << YAML::Key << op_key << YAML::Value << YAML::Flow << YAML::BeginMap;
			for (const auto& [field, group] : OPERATOR_FIELDS)
			{
				out << YAML::Key << field << YAML::Value << YAML::Hex << static_cast<int>(inst[group + op_pos]) << YAML::Dec;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap; // operators
		out << YAML::EndMap; // instrument
	}
	out << YAML::EndMap; // ym_instruments
	out << YAML::EndMap;
}

void ApplyYmInstrumentsFromYaml(const YAML::Node& root, MusicData::YmInstrumentTable& table)
{
	const YAML::Node instruments = root["ym_instruments"];
	if (!instruments.IsDefined() || !instruments.IsMap())
	{
		throw std::runtime_error("expected a top-level 'ym_instruments' map");
	}
	for (const auto& entry : instruments)
	{
		const std::string label = entry.first.as<std::string>();
		constexpr const char* PREFIX = "YM_INSTMT_";
		// %n + a terminator check makes the parse strict: trailing junk ("YM_INSTMT_0G") must not
		// silently alias the entry it happens to share a numeric prefix with.
		unsigned index = 0;
		int consumed = 0;
		if (label.rfind(PREFIX, 0) != 0
			|| std::sscanf(label.c_str() + std::string(PREFIX).size(), "%x%n", &index, &consumed) != 1
			|| label.c_str()[std::string(PREFIX).size() + consumed] != '\0'
			|| index >= MusicData::YM_INSTRUMENT_COUNT)
		{
			throw std::runtime_error("'" + label + "' is not a YM_INSTMT_00-"
				+ InstrumentLabel(MusicData::YM_INSTRUMENT_COUNT - 1).substr(std::string(PREFIX).size()) + " label");
		}
		auto& inst = table[index];
		inst[28] = ReadByte(entry.second["feedback_algo"], label + ".feedback_algo");
		const YAML::Node ops = entry.second["operators"];
		if (!ops.IsDefined() || !ops.IsMap())
		{
			throw std::runtime_error(label + ": expected an 'operators' map");
		}
		for (const auto& [op_key, op_pos] : OPERATOR_KEYS)
		{
			const YAML::Node op = ops[op_key];
			if (!op.IsDefined())
			{
				throw std::runtime_error(label + ".operators: missing '" + op_key + "'");
			}
			for (const auto& [field, group] : OPERATOR_FIELDS)
			{
				inst[group + op_pos] = ReadByte(op[field], label + ".operators." + op_key + "." + field);
			}
		}
	}
}

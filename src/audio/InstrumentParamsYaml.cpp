#include <audio/InstrumentParamsYaml.h>

#include <audio/YamlIo.h>

#include <cstdio>
#include <stdexcept>
#include <string>

using Landstalker::MusicData;

namespace
{
	constexpr uint8_t EFFECT_LOOP = 0x80;
	constexpr uint8_t EFFECT_HOLD = 0x81;

	int ReadInt(const YAML::Node& node, const std::string& context, int min, int max)
	{
		return ReadYamlInt(node, context, min, max);
	}

	// Splits an envelope into audio.py's attack/release phases, or returns false if the byte list
	// doesn't reconstruct exactly from that form.
	bool DecomposeEnvelope(const std::vector<uint8_t>& bytes,
		std::vector<uint8_t>& attack, std::vector<uint8_t>& release)
	{
		attack.clear();
		release.clear();
		std::vector<uint8_t> reconstructed;
		bool in_release = false;
		bool done = false;
		for (const uint8_t b : bytes)
		{
			if (done)
			{
				return false; // trailing bytes after the final marker
			}
			const bool marker = (b & 0x80) != 0;
			(in_release ? release : attack).push_back(b & 0x7F);
			if (marker)
			{
				if (!in_release)
				{
					in_release = true;
				}
				else
				{
					done = true;
				}
			}
		}
		// Rebuild and compare: each phase's final byte carries the marker.
		if (attack.empty())
		{
			return false;
		}
		for (std::size_t i = 0; i < attack.size(); ++i)
		{
			reconstructed.push_back(static_cast<uint8_t>(attack[i] | (i + 1 == attack.size() ? 0x80 : 0)));
		}
		for (std::size_t i = 0; i < release.size(); ++i)
		{
			reconstructed.push_back(static_cast<uint8_t>(release[i] | (i + 1 == release.size() ? 0x80 : 0)));
		}
		return reconstructed == bytes;
	}

	std::size_t IndexFromLabel(const std::string& label, const char* prefix, std::size_t count)
	{
		// %n + a terminator check makes the parse strict: trailing junk ("t_PITCH_EFFECT_1x")
		// must not silently alias the entry it happens to share a numeric prefix with.
		unsigned index = 0;
		int consumed = 0;
		if (label.rfind(prefix, 0) != 0
			|| std::sscanf(label.c_str() + std::string(prefix).size(), "%u%n", &index, &consumed) != 1
			|| label.c_str()[std::string(prefix).size() + consumed] != '\0'
			|| index >= count)
		{
			throw std::runtime_error("'" + label + "' is not a " + prefix + "0-"
				+ std::to_string(count - 1) + " label");
		}
		return index;
	}

	template <typename Array>
	void EmitHexList(YAML::Emitter& out, const char* key, const Array& values)
	{
		out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (const auto v : values)
		{
			out << YAML::Hex << static_cast<int>(v) << YAML::Dec;
		}
		out << YAML::EndSeq;
	}

	template <typename Array>
	void ReadFixedList(const YAML::Node& node, const std::string& context, Array& values, int max)
	{
		if (!node.IsSequence() || node.size() != values.size())
		{
			throw std::runtime_error(context + ": expected a list of exactly "
				+ std::to_string(values.size()) + " values");
		}
		for (std::size_t i = 0; i < values.size(); ++i)
		{
			values[i] = static_cast<typename Array::value_type>(
				ReadInt(node[i], context + "[" + std::to_string(i) + "]", 0, max));
		}
	}
}

void EmitInstrumentParamsYaml(YAML::Emitter& out, const MusicData::InstrumentParams& params)
{
	out << YAML::BeginMap;

	out << YAML::Key << "psg_instruments" << YAML::Value << YAML::BeginMap;
	for (std::size_t i = 0; i < params.psg_envelopes.size(); ++i)
	{
		out << YAML::Key << ("t_PSG_INSTRUMENT_" + std::to_string(i)) << YAML::Value << YAML::BeginMap;
		std::vector<uint8_t> attack, release;
		if (DecomposeEnvelope(params.psg_envelopes[i], attack, release))
		{
			EmitHexList(out, "attack", attack);
			if (!release.empty())
			{
				EmitHexList(out, "release", release);
			}
		}
		else
		{
			EmitHexList(out, "raw", params.psg_envelopes[i]);
		}
		out << YAML::EndMap;
	}
	out << YAML::EndMap;

	out << YAML::Key << "instrument_params" << YAML::Value << YAML::BeginMap;
	EmitHexList(out, "ym_levels", params.ym_levels);
	EmitHexList(out, "slots_per_algo", params.slots_per_algo);
	EmitHexList(out, "ym_frequencies", params.ym_frequencies);
	EmitHexList(out, "psg_frequencies", params.psg_frequencies);
	out << YAML::Key << "pitch_effects" << YAML::Value << YAML::BeginMap;
	for (std::size_t i = 0; i < params.pitch_effects.size(); ++i)
	{
		out << YAML::Key << ("t_PITCH_EFFECT_" + std::to_string(i)) << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (const uint8_t b : params.pitch_effects[i])
		{
			if (b == EFFECT_LOOP)
			{
				out << "loop";
			}
			else if (b == EFFECT_HOLD)
			{
				out << "hold";
			}
			else
			{
				out << static_cast<int>(static_cast<int8_t>(b));
			}
		}
		out << YAML::EndSeq;
	}
	out << YAML::EndMap;
	out << YAML::EndMap;

	out << YAML::EndMap;
}

void ApplyInstrumentParamsFromYaml(const YAML::Node& root, MusicData::InstrumentParams& params)
{
	const YAML::Node psg = root["psg_instruments"];
	if (psg.IsDefined())
	{
		if (!psg.IsMap())
		{
			throw std::runtime_error("'psg_instruments' should be a map");
		}
		for (const auto& entry : psg)
		{
			const std::string label = entry.first.as<std::string>();
			const std::size_t index = IndexFromLabel(label, "t_PSG_INSTRUMENT_", MusicData::PSG_ENVELOPE_COUNT);
			std::vector<uint8_t> bytes;
			const YAML::Node raw = entry.second["raw"];
			if (raw.IsDefined())
			{
				for (std::size_t i = 0; i < raw.size(); ++i)
				{
					bytes.push_back(static_cast<uint8_t>(
						ReadInt(raw[i], label + ".raw[" + std::to_string(i) + "]", 0, 255)));
				}
			}
			else
			{
				const YAML::Node attack = entry.second["attack"];
				if (!attack.IsDefined() || !attack.IsSequence() || attack.size() == 0)
				{
					throw std::runtime_error(label + ": expected a non-empty 'attack' list (or 'raw')");
				}
				const auto read_phase = [&](const YAML::Node& phase, const std::string& name)
				{
					for (std::size_t i = 0; i < phase.size(); ++i)
					{
						const uint8_t amp = static_cast<uint8_t>(
							ReadInt(phase[i], label + "." + name + "[" + std::to_string(i) + "]", 0, 127));
						bytes.push_back(static_cast<uint8_t>(amp | (i + 1 == phase.size() ? 0x80 : 0)));
					}
				};
				read_phase(attack, "attack");
				const YAML::Node release = entry.second["release"];
				if (release.IsDefined())
				{
					if (!release.IsSequence())
					{
						throw std::runtime_error(label + ": 'release' should be a list");
					}
					read_phase(release, "release");
				}
			}
			params.psg_envelopes[index] = std::move(bytes);
		}
	}

	const YAML::Node ip = root["instrument_params"];
	if (ip.IsDefined())
	{
		if (!ip.IsMap())
		{
			throw std::runtime_error("'instrument_params' should be a map");
		}
		if (ip["ym_levels"].IsDefined())
		{
			ReadFixedList(ip["ym_levels"], "ym_levels", params.ym_levels, 255);
		}
		if (ip["slots_per_algo"].IsDefined())
		{
			ReadFixedList(ip["slots_per_algo"], "slots_per_algo", params.slots_per_algo, 255);
		}
		if (ip["ym_frequencies"].IsDefined())
		{
			ReadFixedList(ip["ym_frequencies"], "ym_frequencies", params.ym_frequencies, 0xFFFF);
		}
		if (ip["psg_frequencies"].IsDefined())
		{
			ReadFixedList(ip["psg_frequencies"], "psg_frequencies", params.psg_frequencies, 0xFFFF);
		}
		const YAML::Node effects = ip["pitch_effects"];
		if (effects.IsDefined())
		{
			if (!effects.IsMap())
			{
				throw std::runtime_error("'pitch_effects' should be a map");
			}
			for (const auto& entry : effects)
			{
				const std::string label = entry.first.as<std::string>();
				const std::size_t index = IndexFromLabel(label, "t_PITCH_EFFECT_", MusicData::PITCH_EFFECT_COUNT);
				if (!entry.second.IsSequence())
				{
					throw std::runtime_error(label + ": expected a list");
				}
				std::vector<uint8_t> bytes;
				for (std::size_t i = 0; i < entry.second.size(); ++i)
				{
					const YAML::Node& step = entry.second[i];
					const std::string context = label + "[" + std::to_string(i) + "]";
					std::string word;
					try
					{
						word = step.as<std::string>();
					}
					catch (const YAML::Exception&)
					{
						word.clear();
					}
					if (word == "loop")
					{
						bytes.push_back(EFFECT_LOOP);
					}
					else if (word == "hold")
					{
						bytes.push_back(EFFECT_HOLD);
					}
					else
					{
						bytes.push_back(static_cast<uint8_t>(
							static_cast<int8_t>(ReadInt(step, context, -127, 127))));
					}
				}
				params.pitch_effects[index] = std::move(bytes);
			}
		}
	}
}

#include <landstalker/text/include/HuffmanTrees.h>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <landstalker/misc/include/BitBarrel.h>
#include <landstalker/misc/include/BitBarrelWriter.h>
#include <landstalker/misc/include/Utils.h>

HuffmanTrees::HuffmanTrees()
	: m_num_chars(0)
{
}

HuffmanTrees::HuffmanTrees(const std::vector<std::shared_ptr<LSString>>& strings)
	: m_num_chars(0)
{
	RecalculateTrees(strings);
}

HuffmanTrees::HuffmanTrees(const uint8_t* huffman_char_offsets, size_t huffman_char_offset_size, const uint8_t* huffman_trees, size_t huffman_trees_size, size_t num_chars)
	: m_num_chars(num_chars)
{
	DecodeTrees(huffman_char_offsets, huffman_char_offset_size, huffman_trees, huffman_trees_size, num_chars);
}

void HuffmanTrees::DecodeTrees(const uint8_t* huffman_char_offsets, size_t huffman_char_offset_size, const uint8_t* huffman_trees, size_t huffman_trees_size, size_t num_chars)
{
	if (huffman_char_offset_size < num_chars * 2)
	{
		throw std::runtime_error("Not enough space in the offset table to encode all characters");
	}
	m_num_chars = num_chars;
	m_trees.clear();
	for (size_t c = 0; c < m_num_chars; ++c)
	{
		uint16_t tree_offset = *huffman_char_offsets++ << 8;
		tree_offset |= *huffman_char_offsets++;
		if (tree_offset != 0xFFFF)
		{
			m_trees[static_cast<uint8_t>(c)] = std::make_shared<HuffmanTree>(huffman_trees, tree_offset, huffman_trees_size);
		}
	}
}

void HuffmanTrees::EncodeTrees(std::vector<uint8_t>& huffman_char_offsets, std::vector<uint8_t>& huffman_trees)
{
	huffman_trees.clear();
	// Initialise tree offset table to all null trees
	huffman_char_offsets.clear();
	huffman_char_offsets.assign(std::max<size_t>(m_num_chars, m_trees.rbegin()->first + 1) * 2, 0xFF);
	// Then add in the encoding of each tree from our map of tress
	for (auto& elem : m_trees)
	{
		std::vector<uint8_t> tree;
		size_t result = elem.second->EncodeTree(tree) + huffman_trees.size();
		huffman_char_offsets[elem.first * 2] = (result >> 8) & 0xFF;
		huffman_char_offsets[elem.first * 2 + 1] = result & 0xFF;
		huffman_trees.insert(huffman_trees.end(), tree.begin(), tree.end());
		if ((result > 0xFFFE) || huffman_trees.size() > 0xFFFF)
		{
			throw std::runtime_error("Unable to encode trees: size out of range");
		}
	}
}

std::vector<uint8_t> HuffmanTrees::CompressString(const std::vector<uint8_t>& decompressed, uint8_t eos_marker)
{
	uint8_t last = eos_marker;
	BitBarrelWriter compressed;
	for(auto chr : decompressed)
	{
		if (m_trees.find(last) == m_trees.end())
		{
			std::ostringstream ss;
			ss << "Unable to compress string: Huffman table does not exist for character 0x" << std::hex << last << ".";
			throw std::runtime_error(ss.str());
		}
		if (m_trees[last]->EncodeChar(chr, compressed) == false)
		{
			std::ostringstream ss;
			ss << "Unable to compress string: No entry in Huffman table 0x" << std::hex
			   << static_cast<int>(last) << " for charater 0x" << static_cast<int>(chr) << ".";
			throw std::runtime_error(ss.str());
		}
		last = chr;
	}
	if (last != eos_marker)
	{
		throw std::runtime_error("String terminator " + Hex(eos_marker) + " not last character in string.");
	}
	return std::vector<uint8_t>(compressed.Begin(), compressed.End());
}

std::vector<uint8_t> HuffmanTrees::DecompressString(const std::vector<uint8_t>& compressed, uint8_t eos_marker)
{
	std::vector<uint8_t> decompressed;
	uint8_t last = eos_marker;
	BitBarrel bb(compressed.data());
	do
	{
		if (m_trees.find(last) == m_trees.end())
		{
			std::ostringstream ss;
			ss << "Unable to decompress string: Huffman table does not exist for character " << Hex(last) << ".";
			throw std::runtime_error(ss.str());
		}
		last = m_trees[last]->DecodeChar(bb);
		decompressed.push_back(last);
	} while (last != eos_marker && bb.getBytePosition() < compressed.size());
	return decompressed;
}

void HuffmanTrees::RecalculateTrees(const std::vector<std::shared_ptr<LSString>>& strings)
{
	Debug("Recalculating Huffman trees...");
	m_trees.clear();
	LSString::FrequencyCounts frequencies;
	for (const auto& s : strings)
	{
		s->AddFrequencyCounts(frequencies);
	}
	for (const auto& c : frequencies)
	{
		m_trees[c.first] = std::make_shared<HuffmanTree>(c.second);
	}
}

#ifndef _HUFFMAN_TREES_H_
#define _HUFFMAN_TREES_H_

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <landstalker/text/include/LSString.h>
#include <landstalker/text/include/HuffmanTree.h>

class HuffmanTrees
{
public:
	HuffmanTrees();
	HuffmanTrees(const std::vector<std::shared_ptr<LSString>>& strings);
	HuffmanTrees(const uint8_t* huffman_char_offsets, size_t huffman_char_offset_size,
	             const uint8_t* huffman_trees, size_t huffman_trees_size, size_t num_chars);

	void DecodeTrees(const uint8_t* huffman_char_offsets, size_t huffman_char_offset_size,
	                 const uint8_t* huffman_trees, size_t huffman_trees_size, size_t num_chars);

	void EncodeTrees(std::vector<uint8_t>& huffman_char_offsets, std::vector<uint8_t>& huffman_trees);

	std::vector<uint8_t> CompressString(const std::vector<uint8_t>& decompressed, uint8_t eos_marker);
	std::vector<uint8_t> DecompressString(const std::vector<uint8_t>& compressed, uint8_t eos_marker);

	void RecalculateTrees(const std::vector<std::shared_ptr<LSString>>& strings);

private:
	std::map<uint8_t, std::shared_ptr<HuffmanTree>> m_trees;
	size_t m_num_chars;
};

#endif // _HUFFMAN_TREES_H_

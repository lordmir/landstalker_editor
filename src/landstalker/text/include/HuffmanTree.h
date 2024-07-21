#ifndef _HUFFMAN_TREE_
#define _HUFFMAN_TREE_

#include <unordered_map>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <landstalker/misc/include/BitBarrel.h>
#include <landstalker/misc/include/BitBarrelWriter.h>

class HuffmanTree
{
public:
	typedef std::unordered_map<uint8_t, size_t> CharFrequencies;

	HuffmanTree();
	HuffmanTree(const CharFrequencies& frequencies);
	HuffmanTree(const uint8_t* tree_data, size_t offset, size_t buffer_size);
	~HuffmanTree();

	size_t  EncodeTree(std::vector<uint8_t>& tree);
	size_t  DecodeTree(const uint8_t* tree_data, size_t offset, size_t buffer_size);
	void    RecalculateTree(const CharFrequencies& frequencies);
	uint8_t DecodeChar(BitBarrel& bb);
	bool    EncodeChar(uint8_t chr, BitBarrelWriter& bb);
private:

	struct Node
	{
		Node() : chr(0xFF), weight(0), left(0), right(0), parent(0) {}
		Node(Node* p) : chr(0xFF), weight(0), left(0), right(0), parent(p) {}
		Node(uint8_t c, size_t w) : chr(c), weight(w), left(0), right(0), parent(0) {}
		~Node()
		{
			if (left) delete left;
			if (right) delete right;
		}

		uint8_t chr;
		size_t weight;
		Node* left;
		Node* right;
		Node* parent;
	};

	void EncodeTreePreorder(BitBarrelWriter& bb, std::vector<uint8_t>& chrs, const Node* node);
	void UpdateEncodingTable();
	void UpdateEncodingTable(Node* node, std::string encoding);

	Node* m_root;
	std::unordered_map<uint8_t, std::string> m_encoding;
};

#endif // _HUFFMAN_TREE_

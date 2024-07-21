#include <landstalker/text/include/HuffmanTree.h>
#include <iostream>
#include <queue>
#include <algorithm>

HuffmanTree::HuffmanTree()
{
	m_root = new Node();
}

HuffmanTree::HuffmanTree(const CharFrequencies& frequencies)
{
	m_root = new Node();
	RecalculateTree(frequencies);
}

HuffmanTree::HuffmanTree(const uint8_t* tree_data, size_t offset, size_t buffer_size)
{
	m_root = new Node();
	DecodeTree(tree_data, offset, buffer_size);
}

HuffmanTree::~HuffmanTree()
{
	if (m_root) delete m_root;
}

size_t HuffmanTree::EncodeTree(std::vector<uint8_t>& tree)
{
	size_t offset = 0;
	BitBarrelWriter bb;
	tree.clear();
	EncodeTreePreorder(bb, tree, m_root);
	std::reverse(tree.begin(), tree.end());
	offset = tree.size();
	tree.insert(tree.end(), bb.Begin(), bb.End());
	return offset;
}

size_t HuffmanTree::DecodeTree(const uint8_t* tree_data, size_t offset, size_t /*buffer_size*/)
{
	tree_data += offset;
	BitBarrel leaves(tree_data);
	Node* cur = m_root;
	while (true)
	{
		if (leaves.getNextBit() == false) // node
		{
			if (cur->left == nullptr)
			{
				cur->left = new Node(cur);
				cur = cur->left;

			}
			else
			{
				// We should never get here
				throw std::runtime_error("Huffman tree corruption detected.");
			}
		}
		else // leaf
		{
			cur->chr = *--tree_data;
			if (cur == m_root)
			{
				// At the root node already
				break;
			}
			do 
			{
				cur = cur->parent;
			} while (cur != m_root && cur->right != nullptr);
			if (cur == m_root && cur->right != nullptr)
			{
				// Filled the tree
				break;
			}
			cur->right = new Node(cur);
			cur = cur->right;
		}
	}

	UpdateEncodingTable();

	return leaves.getBytePosition();
}

void HuffmanTree::RecalculateTree(const CharFrequencies& frequencies)
{
	auto node_comparator = [](Node* lhs, Node* rhs) {return lhs->weight > rhs->weight;};
	std::priority_queue< Node*, std::vector<Node*>, decltype(node_comparator)> nodes(node_comparator);
	// First, create all empty nodes with the identified chrs and weights
	for (const auto& fc : frequencies)
	{
		nodes.push(new Node(fc.first, fc.second));
	}
	// Next, combine lowest weighted elements until we have a complete tree
	while (nodes.size() > 1)
	{
		Node* tmp = new Node();
		tmp->left = nodes.top();
		nodes.pop();
		tmp->right = nodes.top();
		nodes.pop();
		tmp->left->parent = tmp;
		tmp->right->parent = tmp;
		tmp->weight = tmp->left->weight + tmp->right->weight;
		nodes.push(tmp);
	}
	// This tree is now our new root
	if (m_root != nullptr) delete m_root;
	m_root = nodes.top();
	UpdateEncodingTable();
}

uint8_t HuffmanTree::DecodeChar(BitBarrel& bb)
{
	Node* cur = m_root;
	while (cur->chr == 0xFF)
	{
		if (bb.getNextBit() == false)
		{
			if (cur->left != nullptr)
			{
				cur = cur->left;
			}
			else
			{
				throw std::runtime_error("Huffman tree is corrupt.");
			}
		}
		else
		{
			if (cur->right != nullptr)
			{
				cur = cur->right;
			}
			else
			{
				throw std::runtime_error("Huffman tree is corrupt.");
			}
		}
	}
	return cur->chr;
}

bool HuffmanTree::EncodeChar(uint8_t chr, BitBarrelWriter& bb)
{
	bool success = false;

	if (m_encoding.find(chr) != m_encoding.end())
	{
		for (auto c : m_encoding[chr])
		{
			bb.WriteBits(c == '1', 1);
		}
		success = true;
	}

	return success;
}

void HuffmanTree::EncodeTreePreorder(BitBarrelWriter& bb, std::vector<uint8_t>& chrs, const Node* node)
{
	if (node == nullptr)
	{
		return;
	}
	if (node->chr != 0xFF)
	{
		chrs.push_back(node->chr);
		bb.WriteBits(1, 1);
	}
	else
	{
		if (node->left != nullptr)
		{
			bb.WriteBits(0, 1);
			EncodeTreePreorder(bb, chrs, node->left);
		}
		if (node->right != nullptr)
		{
			EncodeTreePreorder(bb, chrs, node->right);
		}
	}
}

void HuffmanTree::UpdateEncodingTable()
{
	m_encoding.clear();
	UpdateEncodingTable(m_root, "");
}

void HuffmanTree::UpdateEncodingTable(Node* node, std::string encoding)
{
	if (node->chr != 0xFF)
	{
		m_encoding[node->chr] = encoding;
	}
	else
	{
		if (node->left != nullptr)
		{
			UpdateEncodingTable(node->left, encoding + "0");
		}
		if (node->right != nullptr)
		{
			UpdateEncodingTable(node->right, encoding + "1");
		}
	}
}

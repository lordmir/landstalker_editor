#ifndef _BROWSER_TREE_CTRL_H_
#define _BROWSER_TREE_CTRL_H_

#include <wx/wx.h>
#include <wx/treectrl.h>

class TreeNodeData : public wxTreeItemData
{
public:
    enum class Node {
        BASE,
        STRING,
        IMAGE,
        TILESET,
        ANIM_TILESET,
        BLOCKSET,
        PALETTE,
        ROOM,
        SPRITE,
        SPRITE_FRAME,
        ENTITY,
        BEHAVIOUR_SCRIPT,
        SCRIPT
    };
    TreeNodeData(Node nodeType = Node::BASE, std::size_t value = 0, int img = -1, bool no_delete = true)
        : m_nodeType(nodeType), m_value(value), m_img(img), m_no_delete(no_delete) {}
    std::size_t GetValue() const { return m_value; }
    Node GetNodeType() const { return m_nodeType; }
    int GetNodeImage() const { return m_img; }
    bool DoNotDelete() const { return m_no_delete; }
private:
    const Node m_nodeType;
    const std::size_t m_value;
    const int m_img;
    bool m_no_delete;
};

class BrowserTreeCtrl : public wxTreeCtrl
{
	wxDECLARE_DYNAMIC_CLASS(BrowserTreeCtrl);
public:
    BrowserTreeCtrl();

	BrowserTreeCtrl(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxTR_DEFAULT_STYLE, const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxTreeCtrlNameStr);

	virtual ~BrowserTreeCtrl() {}

	virtual int OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2) override;
};

#endif // _BROWSER_TREE_CTRL_H_

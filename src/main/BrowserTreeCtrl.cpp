#include <main/BrowserTreeCtrl.h>

wxIMPLEMENT_DYNAMIC_CLASS(BrowserTreeCtrl, wxTreeCtrl);

BrowserTreeCtrl::BrowserTreeCtrl()
	: wxTreeCtrl()
{
}

BrowserTreeCtrl::BrowserTreeCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
	: wxTreeCtrl(parent, id, pos, size, style, validator, name)
{
}

int BrowserTreeCtrl::OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2)
{
	TreeNodeData* data1 = static_cast<TreeNodeData*>(GetItemData(item1));
	TreeNodeData* data2 = static_cast<TreeNodeData*>(GetItemData(item2));
	if (!data1 || !data2 || (data1->DoNotDelete() && data2->DoNotDelete()))
	{
		return 0;
	}
	
	// Deletable items always below non-deltetable items
	if (data1->DoNotDelete())
	{
		return +1;
	}
	else if (data2->DoNotDelete())
	{
		return -1;
	}

	wxString name1 = GetItemText(item1);
	wxString name2 = GetItemText(item2);
	if (data1->GetNodeType() == TreeNodeData::Node::BASE)
	{
		// Folders above non-folders
		if (data2->GetNodeType() == TreeNodeData::Node::BASE)
		{
			return name1.compare(name2);
		}
		else
		{
			return -1;
		}
	}
	else if (data2->GetNodeType() == TreeNodeData::Node::BASE)
	{
		return +1;
	}

	if (data1->GetValue() < data2->GetValue())
	{
		return -1;
	}
	else if (data1->GetValue() > data2->GetValue())
	{
		return +1;
	}

	return name1.compare(name2);
}

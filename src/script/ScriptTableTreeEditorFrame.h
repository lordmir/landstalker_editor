#ifndef _SCRIPT_TABLE_TREE_EDITOR_FRAME_H_
#define _SCRIPT_TABLE_TREE_EDITOR_FRAME_H_

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <script/ScriptTableTreeEditorCtrl.h>

// Editor frame for the four script targets (Shops, Custom Items, Characters, Cutscenes):
// hosts the tree-based ScriptTableTreeEditorCtrl (which owns its own embedded action buttons),
// contributes the YAML/ASM import/export menu items, and surfaces the selected entry's top-level
// attributes (shop room/markups, item id/shop/extra data) in the properties pane.
class ScriptTableTreeEditorFrame : public EditorFrame
{
public:
    ScriptTableTreeEditorFrame(wxWindow* parent, ImageList* imglst);
    virtual ~ScriptTableTreeEditorFrame();

    bool Open(ScriptTableTreeCategory category);
    virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
    virtual void ClearGameData();
    virtual void CommitPendingEdits();

    void UpdateUI() const;
private:
    virtual void InitProperties(wxPropertyGridManager& props) const;
    void RefreshLists() const;
    virtual void UpdateProperties(wxPropertyGridManager& props) const;
    void RefreshProperties(wxPropertyGridManager& props) const;
    virtual void OnPropertyChange(wxPropertyGridEvent& evt);
    virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
    virtual void OnMenuClick(wxMenuEvent& evt);
    virtual void ClearMenu(wxMenuBar& menu) const;
    // The selected entry's index into the underlying script table, or -1 when the selection
    // isn't table-backed (the synthesised "Other Functions" entry, or nothing selected).
    int GetSelectedTableIndex() const;

    mutable bool m_reset_props = false;
    ScriptTableTreeCategory m_category = ScriptTableTreeCategory::SHOP;

    mutable wxPGChoices m_rooms;
    mutable wxPGChoices m_rooms_plus_empty;
    mutable wxPGChoices m_items;

    mutable wxAuiManager m_mgr;
    ScriptTableTreeEditorCtrl* m_editor = nullptr;
};

#endif // _SCRIPT_TABLE_TREE_EDITOR_FRAME_H_

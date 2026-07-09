#ifndef _ENTITY_PROPERTIES_WINDOW_H_
#define _ENTITY_PROPERTIES_WINDOW_H_

#include <wx/dialog.h>
#include <wx/iconbndl.h>
#include <wx/artprov.h>
#include <misc/LookupChoiceControl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/arrstr.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <string>
#include <vector>
#include <memory>
#include <wx/dataview.h>
#include <landstalker/rooms/Entity.h>
#include <landstalker/rooms/RoomDialogueTable.h>
#include <landstalker/misc/Labels.h>
#include <landstalker/main/GameData.h>
#include <script/ScriptTreeNode.h>
#include <script/ScriptTreeDataViewModel.h>

class wxBitmapButton;

// The entity properties dialog is expensive to construct (dozens of controls, thousands of
// combo entries), so its owner keeps one instance alive and reuses it: construct once, then
// call SetEntity() before each ShowModal() to repoint every control at the entity being
// edited. The character script tree is built lazily, the first time the Dialogue tab is
// actually shown after each SetEntity().
class EntityPropertiesWindow : public wxDialog
{
public:

    EntityPropertiesWindow(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd);

    virtual ~EntityPropertiesWindow();

    // Repoints the dialog at an entity, refreshing every control. Must be called before each
    // ShowModal().
    void SetEntity(int id, uint16_t room, std::vector<Landstalker::Entity>& entities);

private:
    void UpdateUI();
    void UpdateBehaviourScript();
    void UpdateBehaviourControls(int behaviour_id);
    void UpdateBehaviourUsage(int behaviour_id);
    bool ApplyBehaviourNameChange();
    void RevertBehaviourNameChange();
    bool CommitBehaviourScript();

    // Points the Script combo at the mapping of the currently selected dialogue slot.
    void RefreshDialogueMapScript();
    // Shows the selected character's script tree, or disables the tree when the Script combo
    // is on "<None>" or names a character without a script table entry.
    void UpdateCharScriptTree();
    // Commits any in-place edit open on the character script tree, pushing its value through
    // the model into the script tables. The floating editor doesn't commit itself when focus
    // moves elsewhere, so anything that swaps the model or closes the dialog must flush first.
    void CommitCharScriptEditing();
    // Rebuilds the character category tree after a script edit, so reference counts and
    // shared-function markers stay accurate.
    void RebuildCharScriptTree();
    // Enables/disables the tree edit buttons to match the current selection's capabilities.
    void UpdateCharScriptButtons();
    // The tree model currently associated with the character script tree, if any.
    ScriptTreeDataViewModel* GetCharScriptModel() const;
    void ShowCharScriptAddMenu(bool child);
    void RemoveCharScriptItem();
    void MoveCharScriptItem(bool up);
    // Recomputes the "[NN] <character>" labels shown by both dialogue dropdowns (the
    // Properties tab's and the Dialogue tab's) after the room's dialogue mapping changes.
    void RefreshDialogueNames();

    void OnClickOK(wxCommandEvent& e);
    void OnClickCancel(wxCommandEvent& e);
    void OnChange(wxCommandEvent& e);
    void OnApplyBehaviourName(wxCommandEvent& e);
    void OnCancelBehaviourName(wxCommandEvent& e);
    void OnDialogueMapDialogueChange(wxCommandEvent& e);
    void OnDialogueMapScriptChange(wxCommandEvent& e);
    void OnCharScriptEditingDone(wxDataViewEvent& e);
    void OnCharScriptSelectionChanged(wxDataViewEvent& e);
    void OnCharScriptItemActivated(wxDataViewEvent& e);
    void OnPageChanged(wxBookCtrlEvent& e);

    std::vector<Landstalker::Entity>* m_entities;
    std::shared_ptr<Landstalker::GameData> m_gd_shared;
    const Landstalker::GameData* m_gd;
    int m_id;
    uint16_t m_room;
    
    int m_chest_id;
    bool m_disabled_for_room;
    uint8_t m_chest_contents;
    uint8_t m_orig_chest_contents;
    int m_chest_flag;
    int m_prev_chest_flag;
    uint8_t m_prev_chest_contents;
    int m_current_behaviour_id = wxNOT_FOUND;

    // Local copy of the room's dialogue->character mapping (RoomDialogueTable row for this
    // room): index = dialogue slot, value = character ID. Edited via the Dialogue tab and
    // committed on OK only if it changed.
    std::vector<Landstalker::Character> m_dialogue_map;
    std::vector<Landstalker::Character> m_dialogue_map_orig;

    // The Characters category tree (one entry per character script table row, plus the
    // trailing "Other Functions"); m_char_models holds lazily created per-entry tree models,
    // both indexed by character ID. Building the tree is the dialog's single most expensive
    // step, so it only happens when the Dialogue tab is actually shown, and is marked stale
    // (rather than rebuilt) by SetEntity() and by script edits made through the tree.
    ScriptTreeNode m_char_tree;
    std::vector<wxObjectDataPtr<ScriptTreeDataViewModel>> m_char_models;
    bool m_char_tree_stale = true;

    wxNotebook* m_notebook;
    wxWindow* m_dialogue_page;
    wxStaticText* m_ctrl_dialog_header;
    LookupChoiceControl* m_ctrl_entity_type;
    wxSpinCtrlDouble* m_ctrl_x;
    wxSpinCtrlDouble* m_ctrl_y;
    wxSpinCtrlDouble* m_ctrl_z;
    wxSpinCtrl* m_ctrl_speed;
    wxChoice* m_ctrl_orientation;
    wxChoice* m_ctrl_palette;
    LookupChoiceControl* m_ctrl_dialogue;
    LookupChoiceControl* m_ctrl_behaviour;
    wxCheckBox* m_ctrl_hostile;
    wxCheckBox* m_ctrl_no_rotate;
    wxCheckBox* m_ctrl_no_pickup;
    wxCheckBox* m_ctrl_has_dialogue;
    wxCheckBox* m_ctrl_visible;
    wxCheckBox* m_ctrl_solid;
    wxCheckBox* m_ctrl_has_gravity;
    wxCheckBox* m_ctrl_has_friction;
    wxCheckBox* m_ctrl_reserved;
    wxCheckBox* m_ctrl_copy_tiles;
    wxSpinCtrl* m_ctrl_copy_source;
    LookupChoiceControl* m_ctrl_behaviour_tab;
    wxTextCtrl* m_ctrl_behaviour_name;
    wxBitmapButton* m_ctrl_behaviour_name_apply;
    wxBitmapButton* m_ctrl_behaviour_name_cancel;
    wxTextCtrl* m_ctrl_behaviour_script;
    wxTextCtrl* m_ctrl_behaviour_usage;
    LookupChoiceControl* m_ctrl_dlg_map_dialogue;
    wxChoice* m_ctrl_dlg_map_script;
    wxStaticText* m_ctrl_dlg_map_hint;
    wxDataViewCtrl* m_ctrl_char_script = nullptr;
    wxButton* m_btn_char_add_child = nullptr;
    wxButton* m_btn_char_add_sibling = nullptr;
    wxButton* m_btn_char_remove = nullptr;
    wxButton* m_btn_char_move_up = nullptr;
    wxButton* m_btn_char_move_down = nullptr;
    wxStdDialogButtonSizer* m_sizer_btn;
    wxStaticText* m_chest_label;
    wxCheckBox* m_ctrl_chest_prev;
    wxTextCtrl* m_ctrl_chest_idx;
    LookupChoiceControl* m_ctrl_chest_content;
    wxButton* m_btn_ok;
    wxButton* m_btn_cancel;

};

#endif // _ENTITY_PROPERTIES_WINDOW_H_

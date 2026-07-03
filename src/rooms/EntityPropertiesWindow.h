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
#include <landstalker/rooms/Entity.h>
#include <landstalker/misc/Labels.h>
#include <landstalker/main/GameData.h>

class wxBitmapButton;

class EntityPropertiesWindow : public wxDialog
{
public:

    EntityPropertiesWindow(wxWindow* parent, int id, uint16_t room, std::vector<Landstalker::Entity>& entities, const Landstalker::GameData* gd, const std::vector<std::wstring>& char_names = {});

    virtual ~EntityPropertiesWindow();

private:
    void UpdateUI();
    void UpdateBehaviourScript();
    void UpdateBehaviourControls(int behaviour_id);
    void UpdateBehaviourUsage(int behaviour_id);
    bool ApplyBehaviourNameChange();
    void RevertBehaviourNameChange();
    bool CommitBehaviourScript();

    void OnClickOK(wxCommandEvent& e);
    void OnClickCancel(wxCommandEvent& e);
    void OnChange(wxCommandEvent& e);
    void OnApplyBehaviourName(wxCommandEvent& e);
    void OnCancelBehaviourName(wxCommandEvent& e);

    std::vector<Landstalker::Entity>* m_entities;
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
    wxStdDialogButtonSizer* m_sizer_btn;
    wxStaticText* m_chest_label;
    wxCheckBox* m_ctrl_chest_prev;
    wxTextCtrl* m_ctrl_chest_idx;
    LookupChoiceControl* m_ctrl_chest_content;
    wxButton* m_btn_ok;
    wxButton* m_btn_cancel;

};

#endif // _ENTITY_PROPERTIES_WINDOW_H_

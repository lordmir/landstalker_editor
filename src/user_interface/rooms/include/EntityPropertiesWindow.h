#ifndef _ENTITY_PROPERTIES_WINDOW_H_
#define _ENTITY_PROPERTIES_WINDOW_H_

#include <wx/dialog.h>
#include <wx/iconbndl.h>
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/arrstr.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <landstalker/rooms/include/Entity.h>
#include <landstalker/misc/include/Labels.h>

class EntityPropertiesWindow : public wxDialog
{
public:
    EntityPropertiesWindow(wxWindow* parent, int id, Entity* entity, const std::vector<std::wstring>& char_names = {});

    virtual ~EntityPropertiesWindow();

private:

    void OnClickOK(wxCommandEvent& e);
    void OnClickCancel(wxCommandEvent& e);

    Entity* m_entity;
    int m_id;

    wxStaticText* m_ctrl_dialog_header;
    wxChoice* m_ctrl_entity_type;
    wxSpinCtrlDouble* m_ctrl_x;
    wxSpinCtrlDouble* m_ctrl_y;
    wxSpinCtrlDouble* m_ctrl_z;
    wxSpinCtrl* m_ctrl_speed;
    wxChoice* m_ctrl_orientation;
    wxChoice* m_ctrl_palette;
    wxChoice* m_ctrl_dialogue;
    wxChoice* m_ctrl_behaviour;
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
    wxStdDialogButtonSizer* m_sizer_btn;
    wxButton* m_btn_ok;
    wxButton* m_btn_cancel;

};

#endif // _ENTITY_PROPERTIES_WINDOW_H_
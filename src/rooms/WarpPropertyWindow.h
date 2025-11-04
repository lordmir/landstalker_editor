#ifndef _WARP_PROPERTY_WINDOW_H_
#define _WARP_PROPERTY_WINDOW_H_

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
#include <landstalker/main/GameData.h>
#include <landstalker/rooms/WarpList.h>

class WarpPropertyWindow : public wxDialog
{
public:
    WarpPropertyWindow(wxWindow* parent, uint16_t src_room, int id, Landstalker::WarpList::Warp* warp, Landstalker::GameData& gd);

    virtual ~WarpPropertyWindow();

private:

    void OnClickOK(wxCommandEvent& e);
    void OnClickCancel(wxCommandEvent& e);

    Landstalker::WarpList::Warp* m_warp;
    int m_id;
    bool m_unknown_selectable;
    uint16_t m_room_src;

    wxStaticText* m_ctrl_dialog_header;
    wxChoice* m_ctrl_src_room;
    wxSpinCtrl* m_ctrl_src_x;
    wxSpinCtrl* m_ctrl_src_y;
    wxChoice* m_ctrl_dst_room;
    wxSpinCtrl* m_ctrl_dst_x;
    wxSpinCtrl* m_ctrl_dst_y;
    wxChoice* m_ctrl_type;
    wxChoice* m_ctrl_size;
    wxStdDialogButtonSizer* m_sizer_btn;
    wxButton* m_btn_ok;
    wxButton* m_btn_cancel;
};

#endif // _WARP_PROPERTY_WINDOW_H_
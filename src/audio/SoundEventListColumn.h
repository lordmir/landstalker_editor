#ifndef _SOUND_EVENT_LIST_COLUMN_H_
#define _SOUND_EVENT_LIST_COLUMN_H_

#include <functional>

#include <wx/string.h>

#include <audio/SoundEventFormat.h>
#include <landstalker/main/MusicData.h>

class wxWindow;
class wxSizer;
class wxStaticBoxSizer;
class wxListView;
class wxListEvent;
class wxButton;
class wxContextMenuEvent;
class wxMenu;
class wxPoint;

// One channel's event list, as a self-contained UI unit: a titled box containing a small
// add/delete/move-up/move-down toolbar and a list control (one row per event), plus a right-click
// context menu with the same actions. Reads/writes the channel's events through a pair of
// callbacks rather than owning the data itself, so the same component works for both
// MusicEditorFrame's fixed ten channels and SfxEditorFrame's type-dependent three-or-ten.
//
// Used by MusicEditorFrame/SfxEditorFrame, which also field the keyboard shortcuts (digits to
// switch focus, Del/Enter/Shift+arrows, and the letter keys to insert a given event kind) and
// forward them to whichever column currently has focus via HasListFocus()/InsertDefault() etc.
class SoundEventListColumn
{
public:
    using EventStream = Landstalker::MusicData::EventStream;
    using GetEventsFn = std::function<EventStream()>;
    using SetEventsFn = std::function<void(const EventStream&)>;

    SoundEventListColumn(wxWindow* parent, const wxString& label, SoundEventChannelKind kind,
        GetEventsFn get_events, SetEventsFn set_events);

    wxSizer* GetSizer() const;
    // The static box itself - needed to Show()/Hide() and relabel the whole column (SfxEditorFrame
    // shows only as many columns as the current SFX type has channels).
    wxWindow* GetBoxWindow() const;

    void SetLabel(const wxString& label);
    void SetChannelKind(SoundEventChannelKind kind);
    // Repopulates the list from GetEventsFn() - call after Set*() elsewhere changes this channel's
    // data (e.g. the owning frame loading a different pool entry).
    void Refresh();

    bool HasListFocus() const;
    void SetFocus();

    void InsertDefault(SoundEventInsertKind kind);
    void DeleteSelected();
    void EditSelected();
    void MoveSelected(int direction); // -1 up, +1 down

private:
    void BuildInsertMenu(wxMenu& menu, std::vector<SoundEventInsertKind>& kinds_out) const;
    void ShowMenuOn(wxWindow* window, const wxPoint* client_pos);
    void SelectRow(long row);

    void OnItemActivated(wxListEvent& evt);
    void OnContextMenu(wxContextMenuEvent& evt);

    wxStaticBoxSizer* m_box = nullptr;
    wxListView* m_list = nullptr;
    wxButton* m_add_btn = nullptr;
    wxButton* m_del_btn = nullptr;
    wxButton* m_up_btn = nullptr;
    wxButton* m_down_btn = nullptr;

    SoundEventChannelKind m_kind;
    GetEventsFn m_get;
    SetEventsFn m_set;
};

#endif // _SOUND_EVENT_LIST_COLUMN_H_

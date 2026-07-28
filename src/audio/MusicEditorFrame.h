#ifndef _MUSIC_EDITOR_FRAME_H_
#define _MUSIC_EDITOR_FRAME_H_

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <wx/panel.h>
#include <wx/string.h>

#include <audio/SoundEventListColumn.h>
#include <landstalker/main/GameData.h>
#include <landstalker/main/MusicData.h>
#include <main/EditorFrame.h>

class wxScrolledWindow;
class wxSizeEvent;
class wxSpinCtrl;
class wxStaticBox;
class wxStaticText;
class wxTextCtrl;

// Detail editor for a single music track pool entry (see MusicData::MusicTrackEntry) - each pool
// entry is its own leaf under the "Music" tree node. Which slot(s) (of the 64 playable ids,
// 00h-3Fh) play this track is edited separately, in AudioBankMappingFrame.
//
// A track's ten channels (see MusicData::SoundEvent) are shown as ten parallel SoundEventListColumn
// instances - see that class for the actual add/delete/move/edit UI. This class additionally binds
// the keyboard shortcuts (digits switch which column has focus; the rest apply to whichever column
// does) via a wxEVT_CHAR_HOOK handler, since those need to work regardless of which control (a
// column's list, or a detail field) currently has keyboard focus.
class MusicEditorFrame : public EditorFrame
{
public:
    MusicEditorFrame(wxWindow* parent, ImageList* imglst);
    virtual ~MusicEditorFrame();

    // Shows the pool entry at `index`. False (and the panel stays on whatever it last showed) if
    // out of range.
    bool Open(std::size_t index);
    virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
    virtual void ClearGameData();
    // Flushes the detail panel's fields (which only commit on blur/Enter) before a save/build.
    virtual void CommitPendingEdits();

private:
    using MusicTrackEntry = Landstalker::MusicData::MusicTrackEntry;
    using EventStream = Landstalker::MusicData::EventStream;

    void BuildUI();
    void LoadDetail();
    void RefreshMenuEnable() const;
    void CommitDetailFields();
    void RefreshUsageLabel();
    void RefreshChannelLists();
    void OnCharHook(wxKeyEvent& evt);

    virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
    virtual void OnMenuClick(wxMenuEvent& evt);
    virtual void ClearMenu(wxMenuBar& menu) const;

    void OnAddTrack();
    void OnDeleteTrack();
    void OnExportYaml();
    void OnImportYaml();

    EventStream GetChannelEvents(std::size_t channel) const;
    void SetChannelEvents(std::size_t channel, const EventStream& events);

    wxScrolledWindow* m_panel = nullptr;

    wxStaticBox* m_detail_box = nullptr;
    wxStaticText* m_index_label = nullptr;
    wxTextCtrl* m_name_ctrl = nullptr;
    wxSpinCtrl* m_tempo_ctrl = nullptr;
    wxStaticText* m_tempo_hz_label = nullptr;
    wxSpinCtrl* m_autofade_ctrl = nullptr;
    wxStaticText* m_usage_label = nullptr;
    std::array<std::unique_ptr<SoundEventListColumn>, Landstalker::MusicData::MUSIC_CHANNEL_COUNT> m_columns;

    std::size_t m_index = 0;
    bool m_have_selection = false;

    // Guards handlers against firing while the UI is being populated programmatically - see the
    // equivalent m_populating in SampleEditorFrame for why this is needed on top of the obvious
    // "don't bind until after SetValue" approach.
    bool m_populating = false;
};

#endif // _MUSIC_EDITOR_FRAME_H_

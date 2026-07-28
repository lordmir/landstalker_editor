#ifndef _SFX_EDITOR_FRAME_H_
#define _SFX_EDITOR_FRAME_H_

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
class wxBoxSizer;
class wxSpinCtrl;
class wxStaticBox;
class wxStaticText;
class wxTextCtrl;

// Detail editor for a single SFX pool entry (see MusicData::SfxPoolEntry) - each pool entry is its
// own leaf under the "SFX" tree node. Which slot(s) (of the 58 playable ids, 41h-7Ah) play this
// SFX is edited separately, in AudioBankMappingFrame. See MusicEditorFrame, which this mirrors;
// the difference is an SFX's channel count depends on its type (10 for a full effect, 3 for an
// overlay that plays over FM channels 4-6 without interrupting the music) - so only as many of the
// ten channel list columns are shown as the current type needs.
class SfxEditorFrame : public EditorFrame
{
public:
    SfxEditorFrame(wxWindow* parent, ImageList* imglst);
    virtual ~SfxEditorFrame();

    bool Open(std::size_t index);
    virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
    virtual void ClearGameData();
    virtual void CommitPendingEdits();

private:
    using SfxPoolEntry = Landstalker::MusicData::SfxPoolEntry;
    using EventStream = Landstalker::MusicData::EventStream;

    void BuildUI();
    void LoadDetail();
    void RefreshMenuEnable() const;
    void CommitDetailFields();
    void RefreshUsageLabel();
    void RefreshChannelLists();
    // Shows/labels exactly as many channel columns as the current type needs (3 or 10).
    void UpdateChannelColumnVisibility(uint8_t type);
    void OnCharHook(wxKeyEvent& evt);

    virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
    virtual void OnMenuClick(wxMenuEvent& evt);
    virtual void ClearMenu(wxMenuBar& menu) const;

    void OnAddSfx();
    void OnDeleteSfx();
    void OnExportYaml();
    void OnImportYaml();
    void OnExportMidi();

    EventStream GetChannelEvents(std::size_t channel) const;
    void SetChannelEvents(std::size_t channel, const EventStream& events);

    wxScrolledWindow* m_panel = nullptr;

    wxStaticBox* m_detail_box = nullptr;
    wxStaticText* m_index_label = nullptr;
    wxTextCtrl* m_name_ctrl = nullptr;
    wxSpinCtrl* m_type_ctrl = nullptr;
    wxStaticText* m_type_note = nullptr;
    wxStaticText* m_usage_label = nullptr;
    wxBoxSizer* m_channels_sizer = nullptr;
    std::array<std::unique_ptr<SoundEventListColumn>, Landstalker::MusicData::SFX_FULL_CHANNEL_COUNT> m_columns;

    std::size_t m_index = 0;
    bool m_have_selection = false;

    bool m_populating = false;
};

#endif // _SFX_EDITOR_FRAME_H_

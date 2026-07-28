#ifndef _AUDIO_BANK_MAPPING_FRAME_H_
#define _AUDIO_BANK_MAPPING_FRAME_H_

#include <cstdint>
#include <vector>

#include <wx/panel.h>
#include <wx/string.h>

#include <landstalker/main/GameData.h>
#include <landstalker/main/MusicData.h>
#include <main/EditorFrame.h>

class wxScrolledWindow;
class wxStaticText;
class wxChoice;

// Editor for the three tables that assign a pool entry (see MusicEditorFrame/SfxEditorFrame) to a
// playable id: music bank 4 (ids 00h-1Fh), music bank 3 (ids 20h-3Fh), and the SFX table (ids
// 41h-7Ah). Kept separate from the per-track/per-SFX detail editors since this is fundamentally a
// different task - routing ids to content, not editing content.
class AudioBankMappingFrame : public EditorFrame
{
public:
    AudioBankMappingFrame(wxWindow* parent, ImageList* imglst);
    virtual ~AudioBankMappingFrame();

    bool Open();
    virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
    virtual void ClearGameData();

private:
    struct SlotRow
    {
        wxChoice* choice = nullptr;
    };

    static constexpr int GRID_COLUMNS = 8; // id/choice pairs per row

    void BuildUI();
    void LoadValues();

    // Builds one bank's section (a label + grid of rows) into `sizer`, covering slot ids
    // [first_slot, first_slot + count).
    void BuildMusicSection(wxSizer* sizer, const wxString& title, std::size_t first_slot, std::size_t count,
        std::vector<SlotRow>& rows);
    void BuildSfxSection(wxSizer* sizer);

    void RefreshMusicChoices(std::vector<SlotRow>& rows, std::size_t first_slot);
    void RefreshSfxChoices();
    void OnMusicSlotChanged(std::size_t slot);
    void OnSfxSlotChanged(std::size_t slot);

    wxScrolledWindow* m_panel = nullptr;

    std::vector<SlotRow> m_bank4_rows; // slots 0-31 (ids 00h-1Fh)
    std::vector<SlotRow> m_bank3_rows; // slots 32-63 (ids 20h-3Fh)
    std::vector<SlotRow> m_sfx_rows;   // slots 0-57 (ids 41h-7Ah)

    bool m_populating = false;
};

#endif // _AUDIO_BANK_MAPPING_FRAME_H_

#ifndef _INSTRUMENT_PARAMS_FRAME_H_
#define _INSTRUMENT_PARAMS_FRAME_H_

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <wx/string.h>

#include <landstalker/main/GameData.h>
#include <landstalker/main/MusicData.h>
#include <main/EditorFrame.h>

class wxCheckBox;
class wxChoice;
class wxButton;
class wxListBox;
class wxListEvent;
class wxListView;
class wxScrolledWindow;
class wxSpinCtrl;

// Editor for the driver's instrument parameter tables (see MusicData::InstrumentParams /
// code/audio/instrument_params.asm): the 16 PSG amplitude envelopes and 16 vibrato/pitch-effect
// waveforms (each a small step list), plus the FM volume curve, the carriers-per-algorithm masks
// and the two note frequency tables.
//
// The two step-list groups share one generic sub-editor (StepGroup): a picker for which of the 16
// entries is selected, the selected entry's steps, add/delete/move buttons, and per-step edit
// controls. What differs per group is how a raw byte maps to the step controls - amplitude +
// sustain-marker bit for envelopes; signed delta or a loop/hold control byte for pitch effects.
class InstrumentParamsFrame : public EditorFrame
{
public:
    InstrumentParamsFrame(wxWindow* parent, ImageList* imglst);
    virtual ~InstrumentParamsFrame();

    bool Open();
    virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
    virtual void ClearGameData();

private:
    using InstrumentParams = Landstalker::MusicData::InstrumentParams;

    struct StepGroup
    {
        wxListBox* picker = nullptr;
        wxListView* steps = nullptr;
        wxButton* add_btn = nullptr;
        wxButton* delete_btn = nullptr;
        wxButton* up_btn = nullptr;
        wxButton* down_btn = nullptr;
        // Envelope step controls.
        wxSpinCtrl* amplitude = nullptr;
        wxCheckBox* sustain = nullptr;
        // Pitch-effect step controls.
        wxChoice* kind = nullptr;   // Delta / Loop (80h) / Hold (81h)
        wxSpinCtrl* delta = nullptr;
        std::size_t selected_entry = 0;
        bool is_envelope = false;
    };

    void BuildUI();
    wxSizer* BuildStepGroup(StepGroup& group, const wxString& title, bool is_envelope);
    wxSizer* BuildByteTable(const wxString& title, std::vector<wxSpinCtrl*>& ctrls, std::size_t count,
        int max, std::size_t per_row);
    wxSizer* BuildWordTable(const wxString& title, std::vector<wxSpinCtrl*>& ctrls, std::size_t count);

    // The selected entry's byte list within the game data, for a group.
    std::vector<uint8_t>* GroupBytes(StepGroup& group, InstrumentParams& params) const;
    wxString DescribeStep(const StepGroup& group, uint8_t byte) const;

    void RefreshAll();
    void RefreshGroupSteps(StepGroup& group);
    void LoadStepControls(StepGroup& group);
    void CommitStepControls(StepGroup& group);
    void OnGroupEntrySelected(StepGroup& group);
    void OnGroupAdd(StepGroup& group);
    void OnGroupDelete(StepGroup& group);
    void OnGroupMove(StepGroup& group, int direction);
    void CommitTables();

    virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
    virtual void OnMenuClick(wxMenuEvent& evt);
    virtual void ClearMenu(wxMenuBar& menu) const;
    void RefreshMenuEnable() const;
    void OnExportYaml();
    void OnImportYaml();

    template <typename Fn>
    void ModifyParams(Fn&& fn);

    wxScrolledWindow* m_panel = nullptr;
    StepGroup m_envelopes;
    StepGroup m_effects;
    std::vector<wxSpinCtrl*> m_levels;
    std::vector<wxSpinCtrl*> m_slots;
    std::vector<wxSpinCtrl*> m_ym_freqs;
    std::vector<wxSpinCtrl*> m_psg_freqs;

    bool m_populating = false;
};

#endif // _INSTRUMENT_PARAMS_FRAME_H_

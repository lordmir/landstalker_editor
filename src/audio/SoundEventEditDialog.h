#ifndef _SOUND_EVENT_EDIT_DIALOG_H_
#define _SOUND_EVENT_EDIT_DIALOG_H_

#include <wx/dialog.h>

#include <audio/SoundEventFormat.h>
#include <landstalker/main/MusicData.h>

class wxCheckBox;
class wxChoice;
class LookupChoiceControl;
class wxSpinCtrl;

// Edits one event's parameters - which fields are shown depends on the event's kind (see
// SoundEventInsertKind) and, for a handful of kinds, the channel it belongs to (e.g. a note's
// pitch field differs for FM/PSG tone vs DAC vs PSG noise). The kind itself is fixed for the
// dialog's lifetime; to change an event's kind, delete it and insert a new one.
class SoundEventEditDialog : public wxDialog
{
public:
    SoundEventEditDialog(wxWindow* parent, SoundEventChannelKind channel_kind,
        const Landstalker::MusicData::SoundEvent& event);

    // Valid any time; reflects the current (possibly just-edited) field values.
    Landstalker::MusicData::SoundEvent GetResult() const;

private:
    void BuildUI();
    wxSizer* BuildPitchFields(wxWindow* parent);
    wxSizer* BuildDurationFields(wxWindow* parent);
    void OnOk(wxCommandEvent& evt);

    SoundEventChannelKind m_channel_kind;
    SoundEventInsertKind m_kind;
    Landstalker::MusicData::SoundEvent m_event;

    // Note/rest
    wxChoice* m_note_choice = nullptr;
    wxSpinCtrl* m_octave_spin = nullptr;
    wxSpinCtrl* m_pitch_spin = nullptr; // DAC sample id / PSG noise mode
    wxCheckBox* m_duration_check = nullptr;
    wxSpinCtrl* m_duration_spin = nullptr;

    // Generic single/double byte fields, reused across several command kinds.
    LookupChoiceControl* m_instrument_choice = nullptr; // FM instrument picker, by name
    wxSpinCtrl* m_byte_spin = nullptr;
    wxSpinCtrl* m_byte_spin2 = nullptr;
    wxCheckBox* m_check1 = nullptr;
    wxCheckBox* m_check2 = nullptr;
    wxChoice* m_mode_choice = nullptr;

    // Loop begin / jump address
    wxSpinCtrl* m_count_spin = nullptr;
    wxSpinCtrl* m_address_spin = nullptr;
};

#endif // _SOUND_EVENT_EDIT_DIALOG_H_

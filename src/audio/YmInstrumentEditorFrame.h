#ifndef _YM_INSTRUMENT_EDITOR_FRAME_H_
#define _YM_INSTRUMENT_EDITOR_FRAME_H_

#include <array>
#include <cstdint>
#include <memory>

#include <wx/string.h>

#include <landstalker/main/GameData.h>
#include <landstalker/main/MusicData.h>
#include <main/EditorFrame.h>

class wxButton;
class wxCheckBox;
class wxListEvent;
class wxListView;
class wxPanel;
class wxSpinCtrl;
class wxSpinEvent;
class wxStaticText;

// Editor for the FM (YM2612) instrument patch table (see MusicData::YmInstrumentTable) - the 80
// fixed patches YM_INSTMT_00-4F an FM channel selects with its instrument command. A list of all
// 80 on the left; the selected patch's parameters on the right, with each packed register byte
// split into its YM2612 bit fields (detune/multiple, rate scaling/attack, etc.), one column per
// operator. Edits write straight back to MusicData; unused/undocumented bits of each register
// byte are preserved, so a load/save cycle is byte-exact even for out-of-spec data.
class YmInstrumentEditorFrame : public EditorFrame
{
public:
    YmInstrumentEditorFrame(wxWindow* parent, ImageList* imglst);
    virtual ~YmInstrumentEditorFrame();

    bool Open();
    virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
    virtual void ClearGameData();

private:
    using YmInstrument = Landstalker::MusicData::YmInstrument;

    // One operator column's controls. The UI shows operators in the musician's order OP1-OP4;
    // the underlying bytes are in YM2612 slot order (S1, S3, S2, S4) - see StorageIndex().
    struct OperatorControls
    {
        wxSpinCtrl* detune = nullptr;       // DT, reg 30h bits 4-6
        wxSpinCtrl* multiple = nullptr;     // MUL, reg 30h bits 0-3
        wxSpinCtrl* total_level = nullptr;  // TL, reg 40h bits 0-6
        wxSpinCtrl* rate_scaling = nullptr; // RS, reg 50h bits 6-7
        wxSpinCtrl* attack_rate = nullptr;  // AR, reg 50h bits 0-4
        wxCheckBox* am_enable = nullptr;    // AM, reg 60h bit 7
        wxSpinCtrl* decay_rate = nullptr;   // D1R, reg 60h bits 0-4
        wxSpinCtrl* sustain_rate = nullptr; // D2R, reg 70h bits 0-4
        wxSpinCtrl* sustain_level = nullptr;// D1L, reg 80h bits 4-7
        wxSpinCtrl* release_rate = nullptr; // RR, reg 80h bits 0-3
        wxSpinCtrl* ssg_eg = nullptr;       // SSG-EG, reg 90h bits 0-3
    };

    void BuildUI();
    void RefreshList();
    void RefreshListRow(long row);
    void LoadDetail();
    void CommitDetail();
    wxString SummaryFor(const YmInstrument& instrument) const;

    virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
    virtual void OnMenuClick(wxMenuEvent& evt);
    virtual void ClearMenu(wxMenuBar& menu) const;
    void RefreshMenuEnable() const;
    void OnExportYaml();
    void OnImportYaml();

    void OnListSelection(wxListEvent& evt);
    void OnRename();
    wxString NameFor(std::size_t index) const;

    // The byte index within a patch for UI operator column `op` (0-3 = OP1-OP4) of the register
    // group starting at `group_offset` (0, 4, 8, ... 24).
    static std::size_t ByteIndex(std::size_t group_offset, std::size_t op);

    wxPanel* m_panel = nullptr;
    wxListView* m_list = nullptr;
    wxStaticText* m_name_label = nullptr;
    wxButton* m_rename_btn = nullptr;
    wxSpinCtrl* m_feedback = nullptr;   // FB, reg B0h bits 3-5
    wxSpinCtrl* m_algorithm = nullptr;  // ALG, reg B0h bits 0-2
    std::array<OperatorControls, 4> m_ops;

    std::size_t m_index = 0;
    bool m_populating = false;
};

#endif // _YM_INSTRUMENT_EDITOR_FRAME_H_

#ifndef _SCRIPT_ENTRY_EDITORS_H_
#define _SCRIPT_ENTRY_EDITORS_H_

#include <algorithm>
#include <memory>
#include <wx/wx.h>
#include <wx/dataview.h>
#include <landstalker/script/ScriptTableEntry.h>
#include <landstalker/main/GameData.h>

// Base class for the Main Script Editor's per-entry-type floating editors: one concrete subclass
// per ScriptTableEntryType "shape", each built once (in its constructor) around the entry it was
// opened with and torn down when editing ends - unlike the old single shared editor, there's no
// in-place type switching to support, since "Change Type" is now a row-level context menu action
// (ScriptEditorCtrl) that replaces the row directly rather than reusing an open editor.
class ScriptEntryEditorCtrl : public wxPanel
{
public:
    ScriptEntryEditorCtrl(wxWindow* parent, const wxRect& rect, const Landstalker::ScriptTableEntry& entry)
        : wxPanel(parent, wxID_ANY, rect.GetPosition(), rect.GetSize()),
          m_type(entry.GetType()),
          m_clear(entry.GetClear()),
          m_end(entry.GetEnd())
    {
        // This floats directly on top of the row's still-rendered bubble/label graphics, so it must
        // paint an opaque background itself over its real content - wxPanel does this automatically
        // on most platforms, but GTK needs it forced explicitly or gaps in the sizer let the
        // underlying text show through and clash with the editor's own controls.
        //
        // GTK's own cell-editing placement for this window (confirmed via logging - not something
        // this code can move; every attempt to override its position was silently reverted by GTK)
        // starts a fixed, unrequested amount to the left of where the Index column actually ends on
        // screen, so a strip on this window's own left edge always sits over the Index column's cell.
        // A prior version of this code left that strip unpainted on the theory that the Index
        // column's own (separately redrawn) content would keep showing through underneath it - that
        // turned out not to hold in practice (confirmed by testing multiple precisely-measured margin
        // values, all producing the same blank result), so there's no reliable way to know whether
        // anything actually repaints that area once this window covers it. Instead of gambling on
        // that, this window paints its own copy of the index number directly into that strip (see
        // OnPaint()) - fully self-contained, not dependent on unverifiable GTK repaint behaviour.
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &ScriptEntryEditorCtrl::OnPaint, this);

        // Unlike the old single-widget-per-editor design, this editor has genuinely blank areas
        // (the leading/trailing spacers in CreateContentSizer()/FinishContentSizer()) with no child
        // control to absorb clicks. A click landing there falls straight through to this panel with
        // nothing handling it, which can let it re-reach the underlying wxDataViewCtrl/GTK treeview
        // as a fresh activation on the same cell while this editor is still mid-teardown - racing
        // GTK's native widget destruction against wx's own deferred delete (wxPendingDelete) and
        // leaving a dangling GTK widget behind (seen as a segfault in wxCheckBox's destructor via
        // GTKDisconnect(), "instance with invalid (NULL) class pointer"). Consuming clicks here,
        // rather than leaving them unhandled, keeps them from going anywhere else.
        Bind(wxEVT_LEFT_DOWN, [](wxMouseEvent&) {});
        Bind(wxEVT_LEFT_UP, [](wxMouseEvent&) {});
        Bind(wxEVT_LEFT_DCLICK, [](wxMouseEvent&) {});
    }
    virtual ~ScriptEntryEditorCtrl() = default;

    virtual std::unique_ptr<Landstalker::ScriptTableEntry> GetValue() const = 0;

    // `index_text` is what OnPaint draws directly into the strip of this window that overlaps the
    // Index column (see the constructor's comment) - a self-drawn copy of the same text the Index
    // column's own wxDataViewTextRenderer("long") would show, so it's correct regardless of whether
    // anything else repaints that area. `dvc`/`item`/`col` are still used to find that strip's width
    // via GetItemRect() (scroll-aware, unlike the column's plain logical GetWidth()).
    void SetIndexCellReference(wxDataViewCtrl* dvc, const wxDataViewItem& item, wxDataViewColumn* col, const wxString& index_text)
    {
        m_index_cell_dvc = dvc;
        m_index_cell_item = item;
        m_index_cell_col = col;
        m_index_text = index_text;
        Refresh();
    }

protected:
    // Matches ScriptDataViewRenderer's LABEL_WIDTH - the space its type bubble (e.g. "PLAY
    // CUTSCENE") reserves before drawing the type-specific fields, so the editor's own bubble (see
    // CreateContentSizer()) and fields line up with where the read-only view's do.
    static constexpr int LEADING_OFFSET = 180;

    // Starts a horizontal content sizer with a real bubble control matching the row renderer's own
    // (colour/text looked up by entry type - see ScriptEntryEditors.cpp's kBubbleStyles), occupying
    // the same LEADING_OFFSET width its read-only counterpart does.
    wxBoxSizer* CreateContentSizer();

    // Adds a trailing dummy (non-functional) field that soaks up roughly half of whatever space is
    // left in the editor once GTK force-stretches its window (see ScriptDataViewRenderer::GetSize()'s
    // comment) - without this the real fields, now fixed/natural-sized rather than wxEXPAND, would
    // leave the other half looking empty/cut off rather than gracefully trailing into blank space.
    void FinishContentSizer(wxBoxSizer* sizer);

    Landstalker::ScriptTableEntryType m_type;

    // Clear/End pass-through for every type except STRING (the only one with its own checkboxes -
    // see ScriptStringEntryEditorCtrl): seeded from the entry this editor was opened with and
    // otherwise left untouched, so editing e.g. an item ID never disturbs Clear/End.
    bool m_clear;
    bool m_end;

private:
    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        const wxSize size = GetSize();

        int margin = 0;
        int local_left = 0;
        if (m_index_cell_dvc)
        {
            const wxRect cell_rect = m_index_cell_dvc->GetItemRect(m_index_cell_item, m_index_cell_col);
            // GetPosition()/GetItemRect() don't share a coordinate origin (confirmed via logging -
            // their Y values disagreed for the same row) - screen coordinates sidestep that, since
            // they're unambiguous regardless of which internal window each value was relative to.
            const wxPoint cell_right_screen = m_index_cell_dvc->ClientToScreen(wxPoint(cell_rect.x + cell_rect.width, cell_rect.y));
            const wxPoint cell_left_screen = m_index_cell_dvc->ClientToScreen(cell_rect.GetPosition());
            margin = std::max(0, cell_right_screen.x - GetScreenPosition().x);
            local_left = cell_left_screen.x - GetScreenPosition().x + 2;
        }

        // Only paint background from `margin` onward - leave the strip that overlaps the Index cell
        // untouched by this window's own opaque fill, rather than blotting out whatever's genuinely
        // there. The self-drawn text below is a *guaranteed-correct addition* layered into that same
        // strip, not a replacement for leaving it alone - painting the whole window opaque and
        // relying solely on the self-drawn text (tried previously) actively covered up content this
        // window has no business drawing over, which was worse than either approach alone.
        if (margin < size.GetWidth())
        {
            dc.SetBrush(wxBrush(GetBackgroundColour()));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(margin, 0, size.GetWidth() - margin, size.GetHeight());
        }

        if (m_index_cell_dvc && !m_index_text.empty())
        {
            // Matches the Index column's own wxDataViewTextRenderer("long") look: default (non-bold)
            // font/colour, vertically centred - so the self-drawn text lines up with every other
            // (non-editing) row's Index cell right next to it. Drawn at the cell's *actual* left edge
            // (converted back to this window's own local coordinates), not a fixed offset from this
            // window's own edge - those only coincide if this window's edge happens to line up with
            // the column's edge, which it usually doesn't. If local_left falls outside this window's
            // own bounds (this window doesn't reach the Index cell at all), DrawText simply clips it
            // away - correct, since there's nothing for this window to draw there in that case.
            const wxSize text_size = dc.GetTextExtent(m_index_text);
            dc.DrawText(m_index_text, local_left, std::max((size.GetHeight() - text_size.GetHeight()) / 2, 0));
        }
    }

    wxDataViewCtrl* m_index_cell_dvc = nullptr;
    wxDataViewItem m_index_cell_item;
    wxDataViewColumn* m_index_cell_col = nullptr;
    wxString m_index_text;
};

// Dispatches on entry.GetType() to build the matching concrete editor, seeded from `entry` (which
// already carries the correct Clear/End flags baked into its ToBytes()/FromBytes() encoding).
wxWindow* CreateScriptEntryEditor(wxWindow* parent, const wxRect& rect,
                                   const Landstalker::ScriptTableEntry& entry,
                                   std::shared_ptr<const Landstalker::GameData> gd);

#endif // _SCRIPT_ENTRY_EDITORS_H_

#ifndef _DATA_VIEW_EDITOR_KEYS_
#define _DATA_VIEW_EDITOR_KEYS_

#include <wx/dataview.h>
#include <wx/popupwin.h>
#include <wx/window.h>

namespace DataViewEditorKeysDetail
{
inline bool IsDescendantOfWindow(wxWindow* win, const wxWindow* ancestor)
{
    for (wxWindow* w = win; w; w = w->GetParent())
    {
        if (w == ancestor)
        {
            return true;
        }
    }
    return false;
}

// A dropdown-style sub-control (see LookupDataViewRenderer.h's embeddable combo) shows its popup
// list in a wxPopupWindow/wxPopupTransientWindow, which isn't reliably a GetParent()-descendant of
// the editor that spawned it on every platform (LookupEditorControl's own internal kill-focus
// handling has to check this separately for the same reason). Since popups aren't used for
// anything else in this codebase's dataview editors, treating any focus target under one as "still
// part of the current edit" is a safe, low-risk way to avoid discarding an edit out from under a
// user who's just interacting with an open dropdown.
inline bool HasPopupAncestor(wxWindow* win)
{
    for (wxWindow* w = win; w; w = w->GetParent())
    {
        if (dynamic_cast<wxPopupWindow*>(w) || dynamic_cast<wxPopupTransientWindow*>(w))
        {
            return true;
        }
    }
    return false;
}
}

// Wires Escape (abandon the edit), Enter (confirm it), and focus-loss (abandon it, unless focus
// moved to the owning wxDataViewCtrl itself or one of `editor`'s own dropdown popups) into a
// custom wxDataViewRenderer editor, recursively across every descendant of `editor` - a composite
// editor (e.g. a panel holding a text field and a checkbox) can have focus on any one of its
// sub-controls, and both wxEVT_CHAR_HOOK and wxEVT_KILL_FOCUS only reach the window that currently
// has it (then bubble up through its own parent chain, which stops at `editor` rather than
// continuing on to the renderer's own handling).
//
// This is needed because a CUSTOM (wxDataViewCustomRenderer) editor is just a plain wxWindow
// floating over the cell - unlike wx's built-in renderers (backed by native GTK cell-editable
// widgets, which already know how to react to Escape/Enter/focus-loss), it gets no such behaviour
// unless the application wires it up explicitly; none of this project's custom editors did before
// this. Without the focus-loss handling specifically, clicking on some other, unrelated control
// entirely (outside both the editor and the dataview) previously just left the floating editor
// open indefinitely, with no way to close it short of clicking back into the dataview.
inline void BindDataViewEditorEscapeEnter(wxWindow* editor, wxDataViewRenderer* renderer)
{
    editor->Bind(wxEVT_KILL_FOCUS, [editor, renderer](wxFocusEvent& evt)
    {
        evt.Skip();
        // Deferred via CallAfter() - at the moment kill-focus fires, the *new* focus target hasn't
        // necessarily been established yet (e.g. mid-transition between two sub-controls of this
        // same composite editor), so checking synchronously here would misfire.
        editor->CallAfter([editor, renderer]()
        {
            using namespace DataViewEditorKeysDetail;
            wxWindow* new_focus = wxWindow::FindFocus();
            if (!new_focus || IsDescendantOfWindow(new_focus, editor) || HasPopupAncestor(new_focus))
            {
                return;
            }
            wxDataViewColumn* col = renderer->GetOwner();
            wxDataViewCtrl* dvc = col ? col->GetOwner() : nullptr;
            if (dvc && IsDescendantOfWindow(new_focus, dvc))
            {
                return;
            }
            renderer->CancelEditing();
        });
    });
    editor->Bind(wxEVT_CHAR_HOOK, [editor, renderer](wxKeyEvent& evt)
    {
        switch (evt.GetKeyCode())
        {
        case WXK_ESCAPE:
        case WXK_RETURN:
        case WXK_NUMPAD_ENTER:
        {
            // wxEVT_CHAR_HOOK fires before wxEVT_KEY_DOWN/wxEVT_CHAR, so calling
            // CancelEditing()/FinishEditing() straight from here (as this used to) pre-empts any
            // more specific Escape/Enter handling a sub-control binds at those later stages - e.g.
            // LookupDataViewRenderer's combo box deliberately binds its own Return handling to
            // wxEVT_KEY_DOWN on GTK, to read the freshly-highlighted popup list item directly. That
            // handler would never even run: this one, not skipping, wins the race and finishes
            // editing immediately, before the combo has committed what the user actually selected -
            // falling back to a possibly-stale tracked value instead and appearing to revert to a
            // near-random choice. Skip() lets the event continue to whatever more specific handling
            // exists first, then commit/cancel deferred via CallAfter() so it runs after that,
            // rather than racing ahead of it.
            evt.Skip();
            const bool cancel = evt.GetKeyCode() == WXK_ESCAPE;
            editor->CallAfter([renderer, cancel]()
            {
                if (cancel)
                {
                    renderer->CancelEditing();
                }
                else
                {
                    renderer->FinishEditing();
                }
            });
            break;
        }
        default:
            evt.Skip();
            break;
        }
    });
    for (wxWindow* child : editor->GetChildren())
    {
        BindDataViewEditorEscapeEnter(child, renderer);
    }
}

#endif // _DATA_VIEW_EDITOR_KEYS_

#pragma once

#include <wx/dialog.h>

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

// GNOME can attach windows carrying the native dialog type hint to their
// parent, which prevents them from being moved independently. Keep the
// transient parent and modal state intact, but present this large editor as a
// normal top-level window so the window manager gives it a movable frame.
inline void MakeModalDialogMovable(wxDialog* dialog)
{
#ifdef __WXGTK__
	gtk_window_set_type_hint(GTK_WINDOW(dialog->GetHandle()), GDK_WINDOW_TYPE_HINT_NORMAL);
#else
	(void)dialog;
#endif
}

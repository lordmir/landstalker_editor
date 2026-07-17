#ifndef _DATA_VIEW_MODEL_ASSOCIATE_H_
#define _DATA_VIEW_MODEL_ASSOCIATE_H_

#include <wx/dataview.h>
#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

// Drop-in replacement for wxDataViewCtrl::AssociateModel() for controls that re-associate
// models over their lifetime. On GTK, re-associating deletes wx's per-model internal state -
// including the flag remembering that wx already installed its own selection function on the
// control's GtkTreeSelection - but the function itself stays installed on the (persistent)
// treeview. The next EditItem()/SetCurrentItem() then trips wx's "selection function has
// changed unexpectedly" assert (wxGtkTreeSelectionLock, wx's gtk/dataview.cpp). Clearing the
// selection function after every (re-)association restores the state a freshly created
// internal expects. No-op difference on other ports.
inline void AssociateDataViewModel(wxDataViewCtrl* ctrl, wxDataViewModel* model)
{
	ctrl->AssociateModel(model);
#ifdef __WXGTK__
	if (GtkWidget* treeview = ctrl->GtkGetTreeView())
	{
		gtk_tree_selection_set_select_function(
			gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), nullptr, nullptr, nullptr);
	}
#endif
}

#endif // _DATA_VIEW_MODEL_ASSOCIATE_H_

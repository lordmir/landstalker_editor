#pragma once

#include <wx/gdicmn.h>

// GTK's native spin buttons need more horizontal room than their Windows
// equivalents. Preserve the existing layout elsewhere while ensuring the
// button column isn't clipped by controls with an explicit width.
inline wxSize SpinCtrlSize(int width)
{
#ifdef __WXGTK__
	width += 30;
#endif
	return wxSize(width, -1);
}

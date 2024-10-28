// --------------------------------------------------------------------
// ipe::Tool
// --------------------------------------------------------------------
/*

	This file is part of the extensible drawing editor Ipe.
	Copyright (c) 1993-2024 Otfried Cheong

	Ipe is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	As a special exception, you have permission to link Ipe with the
	CGAL library and distribute executables, as long as you follow the
	requirements of the Gnu General Public License in regard to all of
	the software in the executable aside from CGAL.

	Ipe is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
	or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
	License for more details.

	You should have received a copy of the GNU General Public License
	along with Ipe; if not, you can find it at
	"http://www.gnu.org/copyleft/gpl.html", or write to the Free
	Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipetoolbase.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Tool
  \ingroup canvas
  \brief Abstract base class for various canvas tools.

  The Canvas doesn't know about the various modes for object creation,
  editing, and moving, but delegates the handling to a subclass of
  Tool.
*/

//! Constructor.
Tool::Tool(CanvasBase * canvas)
    : iCanvas(canvas) {
    // nothing
}

//! Virtual destructor.
Tool::~Tool() {
    // nothing
}

//! Called when a mouse button is pressed or released on the canvas.
/*! \a button is 1, 2, or 3, with Shift/Ctrl/Alt/Meta modifiers added
  in (as defined in CanvasBase::TModifiers.  \a press is true for
  button-down, and false for button-up. */
void Tool::mouseButton(int button, bool press) {
    // ignore it
}

//! Called when the mouse is moved on the canvas.
void Tool::mouseMove() {
    // ignore it
}

//! Called when a key is pressed.
/*! \a modifiers are as defined in CanvasBase::TModifiers. */
bool Tool::key(String text, int modifiers) {
    return false; // not handled
}

//! Snapping to vertices on object currently being drawn.
void Tool::snapVtx(const Vector & mouse, Vector & pos, double & bound, bool cp) const {
    // nothing
}

// --------------------------------------------------------------------

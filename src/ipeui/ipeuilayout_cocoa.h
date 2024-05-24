// -*- objc -*-
// ipeuilayout_cocoa.h
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

#ifndef IPEUILAYOUT_H
#define IPEUILAYOUT_H

extern void addToLayout(NSView *view, NSView *subview);

extern id layoutGuide(NSView *owner);

extern NSLayoutConstraint *layout(id a, id b, const char *rel,
				  double gap = 0.0,
				  double multiplier=1.0,
				  BOOL activate=YES);

extern NSString *ipeui_set_mnemonic(NSString *title, NSButton *button);

// --------------------------------------------------------------------
#endif

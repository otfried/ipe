// --------------------------------------------------------------------
// Helper class for Win32 Unicode interface
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

#ifndef IPEUI_WSTRING_H
#define IPEUI_WSTRING_H

class WString : public std::wstring {
 public:
#ifdef IPEBASE_H
  WString(const ipe::String &s) : std::wstring( std::move(s.w()) ) { /* nothing */ }
#endif
  WString(const std::string &s)  { init(s.data(), s.size()); }
  WString(const char *s) { init(s, -1); }
 private:
  void init(const char *s, int len);
};

extern BOOL setWindowText(HWND h, const char *s);
extern void sendMessage(HWND h, UINT code, const char *t, WPARAM wParam = 0);


// --------------------------------------------------------------------
#endif


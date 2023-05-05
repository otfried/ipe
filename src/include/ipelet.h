// -*- C++ -*-
// --------------------------------------------------------------------
// Ipelets
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

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

#ifndef IPELET_H
#define IPELET_H

#include "ipebase.h"
#include "ipeattributes.h"
#include "ipesnap.h"

#ifdef __MINGW32__
#define IPELET_DECLARE extern "C" __declspec(dllexport)
#else
#define IPELET_DECLARE extern "C"
#endif

// --------------------------------------------------------------------

namespace ipe {

  class Page;
  class Document;

  class IpeletHelper {
  public:
    enum { EOkButton, EOkCancelButtons, EYesNoCancelButtons,
	   EDiscardCancelButtons, ESaveDiscardCancelButtons };
    virtual ~IpeletHelper() = 0;
    //! Show a message in the status bar.
    virtual void message(const char *msg) = 0;
    //! Pop up a modal message box.
    /*! The \a details can be null.

      Choose one of EOkButton, EOkCancelButtons, EYesNoCancelButtons,
      EDiscardCancelButtons, ESaveDiscardCancelButtons
      for \a buttons.

      Returns 1 for Ok or Yes, 0 for No, -1 for Cancel. */
    virtual int messageBox(const char *text, const char *details,
			   int buttons) = 0;
    /*! Pop up a modal dialog asking the user to enter a string.
      Returns true if the user didn't cancel the dialog. */
    virtual bool getString(const char *prompt, String &str) = 0;

    /*! Retrieve a parameter value from a table in the Lua wrapper
      code.  If no table has been passed, or the key is not in the
      table, or its value is not a string or a number, then an empty
      string is returned. */
    virtual String getParameter(const char *key) = 0;
  };

  //! Information provided to an ipelet when it is run.
  struct IpeletData {
    Page *iPage;
    const Document *iDoc;
    int iPageNo, iView, iLayer;
    AllAttributes iAttributes;
    Snap iSnap;
  };

  // --------------------------------------------------------------------

  class Ipelet {
  public:
    virtual ~Ipelet() = 0;
    //! Return the version of Ipelib the Ipelet was linked against.
    virtual int ipelibVersion() const = 0;
    //! Run a function from the Ipelet.
    /*! Return true if page was changed and undo registration is necessary. */
    virtual bool run(int function, IpeletData *data, IpeletHelper *helper) = 0;
  };

} // namespace

// --------------------------------------------------------------------
#endif

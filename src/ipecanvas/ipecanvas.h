// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::Canvas
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

#ifndef IPECANVAS_H
#define IPECANVAS_H

#include "ipelib.h"

// --------------------------------------------------------------------

// Avoid including cairo.h
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;

// --------------------------------------------------------------------

namespace ipe {

  class Fonts;
  class Tool;
  class PdfResources;

  // --------------------------------------------------------------------

  class CanvasObserver {
  public:
    virtual ~CanvasObserver();
    // kind = 0: precise pan, 1: osx 'imprecise' pan, 2: zoom
    virtual void canvasObserverWheelMoved(double xDegrees,
					  double yDegrees,
					  int kind);
    virtual void canvasObserverMouseAction(int button);
    virtual void canvasObserverPositionChanged();
    virtual void canvasObserverToolChanged(bool hasTool);
    virtual void canvasObserverSizeChanged();
  };

  class CanvasBase {

  public:
    //! Keyboard modifiers
    enum TModifiers { EShift = 0x100, EControl = 0x200,
		      EAlt = 0x400, EMeta = 0x800,
		      ECommand = 0x1000 };

    enum TCursor { EStandardCursor, EHandCursor, ECrossCursor,
		   EDotCursor };

    /*! In pretty display, no dashed lines are drawn around text
      objects, and if Latex font data is not available, no text is
      drawn at all. */
    struct Style {
      Color paperColor;
      Color primarySelectionColor;
      Color secondarySelectionColor;
      Color selectionSurroundColor;
      double primarySelectionWidth;
      double secondarySelectionWidth;
      double selectionSurroundWidth;
      Color gridLineColor;
      bool pretty;
      bool classicGrid;
      double thinLine;
      double thickLine;
      int thinStep;
      int thickStep;
      bool paperClip;
      bool numberPages;
    };

    virtual ~CanvasBase();

    void setPage(const Page *page, int pno, int view, const Cascade *sheet);
    void setResources(const PdfResources *resources);

    //! Return current pan.
    inline Vector pan() const { return iPan; }
    //! Return current zoom.
    inline double zoom() const { return iZoom; }
    //! Return current style sheet cascade.
    inline const Cascade *cascade() const { return iCascade; }
    //! Return center of canvas.
    inline Vector center() const {return 0.5 * Vector(iWidth, iHeight); }
    //! Return last mouse position (snapped!) in user coordinates.
    inline Vector pos() const { return iMousePos; }
    //! Return last unsnapped mouse position in user coordinates.
    inline Vector unsnappedPos() const { return iUnsnappedMousePos; }
    //! Return global mouse position of last mouse press/release.
    inline Vector globalPos() const { return iGlobalPos; }
    Vector simpleSnapPos() const;
    //! Return current snapping information.
    inline const Snap &snap() const { return iSnap; }

    //! Set ink mode.
    inline void setInkMode(bool ink) { isInkMode = ink; }

    //! Return current additional modifiers.
    inline int additionalModifiers() const { return iAdditionalModifiers; }
    void setAdditionalModifiers(int mod);

    //! Has an attempt been made to use a Type3 font?
    bool type3Font();

    Vector devToUser(const Vector &arg) const;
    Vector userToDev(const Vector &arg) const;

    void setCanvasStyle(const Style &style);
    //! Return canvas style
    Style canvasStyle() const { return iStyle; }
    void setPan(const Vector &v);
    void setZoom(double zoom);
    void setSnap(const Snap &s);
    void setDimmed(bool dimmed);
    void setAutoOrigin(const Vector &v);

    Matrix canvasTfm() const;

    void setObserver(CanvasObserver *observer);

    void setFifiVisible(bool visible);
    void setSelectionVisible(bool visible);

    void setTool(Tool *tool);
    void finishTool();

    Tool *tool() { return iTool; }

    void update();
    void updateTool();

    int canvasWidth() const { return iWidth; }
    int canvasHeight() const { return iHeight; }

    virtual void setCursor(TCursor cursor, double w = 1.0,
			   Color *color = nullptr) = 0;

    static int selectPageOrView(Document *doc, int page = -1,
				int startIndex = 0,
				int pageWidth = 240,
				int width = 600, int height = 480);

    virtual void invalidate(int x, int y, int w, int h) = 0;

  protected:
    CanvasBase();
    void drawPaper(cairo_t *cc);
    void drawFrame(cairo_t *cc);
    void drawAxes(cairo_t *cc);
    void drawGrid(cairo_t *cc);
    void drawObjects(cairo_t *cc);
    void drawTool(Painter &painter);
    void snapToPaperAndFrame();
    void refreshSurface();
    void computeFifi(double x, double y);
    void drawFifi(cairo_t *cr);

    virtual void invalidate() = 0;

  protected:
    CanvasObserver *iObserver;
    Tool *iTool;
    const Page *iPage;
    int iPageNumber;
    int iView;
    const Cascade *iCascade;

    Style iStyle;

    Vector iPan;
    double iZoom;
    Snap iSnap;
    bool iDimmed;
    bool iAutoSnap;
    Vector iAutoOrigin;
    int iAdditionalModifiers;
    bool isInkMode;

    bool iRepaintObjects;
    double iWidth, iHeight;
    double iBWidth, iBHeight; // size of backing store
    cairo_surface_t *iSurface;

    Vector iUnsnappedMousePos;
    Vector iMousePos;
    Vector iGlobalPos;
    Vector iOldFifi;  // last fifi position that has been drawn
    bool iFifiVisible;
    Snap::TSnapModes iFifiMode;
    bool iSelectionVisible;

    const PdfResources *iResources;
    std::unique_ptr<Fonts> iFonts;
    bool iType3Font;
  };

} // namespace

// --------------------------------------------------------------------
#endif

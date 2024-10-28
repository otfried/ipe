// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::CairoPainter
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

#ifndef IPECAIROPAINTER_H
#define IPECAIROPAINTER_H

#include "ipeattributes.h"
#include "ipefonts.h"
#include "ipepainter.h"

#include <cairo.h>

// --------------------------------------------------------------------

namespace ipe {

class Cascade;
class PdfObj;

class CairoPainter : public Painter {
public:
    CairoPainter(const Cascade * sheet, Fonts * fonts, cairo_t * cc, double zoom,
		 bool pretty, bool filterBest);
    virtual ~CairoPainter() {}

    void setDimmed(bool dim) { iDimmed = dim; }

    void executeStream(const PdfDict * stream, const PdfDict * resources);

    bool type3Font() const { return iType3Font; }

protected:
    virtual void doPush() override;
    virtual void doPop() override;
    virtual void doMoveTo(const Vector & v) override;
    virtual void doLineTo(const Vector & v) override;
    virtual void doCurveTo(const Vector & v1, const Vector & v2,
			   const Vector & v3) override;
    virtual void doClosePath() override;
    virtual void doDrawArc(const Arc & arc) override;

    virtual void doAddClipPath() override;
    virtual void doDrawPath(TPathMode mode) override;
    virtual void doDrawBitmap(Bitmap bitmap) override;
    virtual void doDrawText(const Text * text) override;

private:
    const PdfDict * findResource(String kind, String name);
    void drawGlyphs(std::vector<cairo_glyph_t> & glyphs);
    void collectGlyphs(String s, std::vector<cairo_glyph_t> & glyphs, Vector & textPos);
    void execute(const PdfDict * stream, const PdfDict * resources,
		 bool applyMatrix = true);
    void clearArgs();
    void opcm();
    void opBT();
    void opET();
    void opTf();
    void opTm();
    void opTd(bool setLeading);
    void opTstar();
    void opTc(double * p);
    void opTz();
    void opTJ();
    void opTj(bool nextLine, bool setSpacing);
    void opk(bool stroke);
    void opg(bool stroke);
    void oprg(bool stroke);
    void opscn(bool stroke);
    void opw();
    void opd();
    void opDo();
    void opi();
    void opj();
    void opJ();
    void opM();
    void opW(bool eofill);
    void opgs();
    void opm();
    void opl();
    void opc();
    void opy();
    void opv();
    void oph();
    void opq();
    void opQ();
    void opre();
    void opn();
    void opStrokeFill(bool close, bool fill, bool stroke, bool eofill);
    void opsh();
    void createPattern();

private:
    Fonts * iFonts;
    cairo_t * iCairo;

    double iZoom;
    bool iPretty;

    bool iDimmed;
    bool iAfterMoveTo;

    bool iFilterBest;

    bool iType3Font;

    // PDF operator drawing
    std::vector<const PdfObj *> iArgs;

    std::vector<const PdfDict *> iResourceStack;

    struct PdfState {
	double iStrokeRgb[3];
	double iFillRgb[3];
	double iStrokeOpacity;
	double iFillOpacity;
	Face * iFont; // not owned
	double iFontSize;
	double iTextRise;
	double iCharacterSpacing;
	double iWordSpacing;
	double iHorizontalScaling;
	double iLeading;
	String iFillPattern;
    };

    std::vector<PdfState> iPdfState;

    // text matrix is only valid inside BT-ET pairs
    Matrix iTextMatrix;
    Matrix iTextLineMatrix;
};

} // namespace ipe

// --------------------------------------------------------------------
#endif

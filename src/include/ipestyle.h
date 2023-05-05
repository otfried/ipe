// -*- C++ -*-
// --------------------------------------------------------------------
// Ipe style sheet
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

#ifndef IPESTYLE_H
#define IPESTYLE_H

#include "ipeattributes.h"
#include "ipetext.h"

// --------------------------------------------------------------------

namespace ipe {

  struct Symbol {
    Symbol();
    Symbol(Object *object);
    Symbol(const Symbol &rhs);
    Symbol &operator=(const Symbol &rhs);
    ~Symbol();

    bool iXForm;
    TTransformations iTransformations;
    Object *iObject;
    std::vector<Vector> iSnap;
  };

  class StyleSheet {
  public:

    //! Style of the title on a page.
    struct TitleStyle {
      //! Has a TitleStyle been defined in the style sheet?
      bool iDefined;
      //! Position on the page (in Ipe coordinate system)
      Vector iPos;
      //! Text size.
      Attribute iSize;
      //! Text color.
      Attribute iColor;
      //! Horizontal alignment.
      THorizontalAlignment iHorizontalAlignment;
      //! Vertical alignment.
      TVerticalAlignment iVerticalAlignment;
    };

    //! How to show page numbers on the paper.
    struct PageNumberStyle {
      //! Has a PageNumberStyle been defined in the style sheet?
      bool iDefined;
      //! Position on the page
      Vector iPos;
      //! Text size.
      Attribute iSize;
      //! Text color.
      Attribute iColor;
      //! Text alignment, horizontal.
      THorizontalAlignment iHorizontalAlignment;
      //! Text alignment, vertical.
      TVerticalAlignment iVerticalAlignment;
      //! Template text.
      String iText;
    };

    StyleSheet();

    static StyleSheet *standard();

    void addSymbol(Attribute name, const Symbol &symbol);
    const Symbol *findSymbol(Attribute sym) const;

    void addGradient(Attribute name, const Gradient &s);
    const Gradient *findGradient(Attribute sym) const;

    void addTiling(Attribute name, const Tiling &s);
    const Tiling *findTiling(Attribute sym) const;

    void addEffect(Attribute name, const Effect &e);
    const Effect *findEffect(Attribute sym) const;

    void add(Kind kind, Attribute name, Attribute value);
    bool has(Kind kind, Attribute sym) const;
    Attribute find(Kind, Attribute sym) const;

    void remove(Kind kind, Attribute sym);

    void saveAsXml(Stream &stream, bool saveBitmaps = false) const;

    void allNames(Kind kind, AttributeSeq &seq) const;

    //! Return whether this is the standard style sheet built into Ipe.
    inline bool isStandard() const { return iStandard; }

    //! Return Latex preamble.
    inline String preamble() const { return iPreamble; }
    //! Set LaTeX preamble.
    inline void setPreamble(const String &str) { iPreamble = str; }

    const Layout *layout() const;
    void setLayout(const Layout &margins);

    const TextPadding *textPadding() const;
    void setTextPadding(const TextPadding &pad);

    const TitleStyle *titleStyle() const;
    void setTitleStyle(const TitleStyle &ts);

    const PageNumberStyle *pageNumberStyle() const;
    void setPageNumberStyle(const PageNumberStyle &pns);

    void setLineCap(TLineCap s);
    void setLineJoin(TLineJoin s);
    void setFillRule(TFillRule s);
    //! Return line cap.
    TLineCap lineCap() const { return iLineCap; }
    //! Return line join.
    TLineJoin lineJoin() const { return iLineJoin; }
    //! Return path fill rule.
    TFillRule fillRule() const { return iFillRule; }

    //! Return name of style sheet.
    inline String name() const { return iName; }
    //! Set name of style sheet.
    inline void setName(const String &name) { iName = name; }

  private:
    typedef std::map<int, Symbol> SymbolMap;
    typedef std::map<int, Gradient> GradientMap;
    typedef std::map<int, Tiling> TilingMap;
    typedef std::map<int, Effect> EffectMap;
    typedef std::map<int, Attribute> Map;

    bool iStandard;
    String iName;
    SymbolMap iSymbols;
    GradientMap iGradients;
    TilingMap iTilings;
    EffectMap iEffects;
    Map iMap;
    String iPreamble;
    Layout iLayout;
    TextPadding iTextPadding;
    TitleStyle iTitleStyle;
    PageNumberStyle iPageNumberStyle;

    TLineJoin iLineJoin;
    TLineCap iLineCap;
    TFillRule iFillRule;
  };


  class Cascade {
  public:
    Cascade();
    Cascade(const Cascade &rhs);
    Cascade &operator=(const Cascade &rhs);
    ~Cascade();

    //! Return number of style sheets.
    inline int count() const { return iSheets.size(); }
    //! Return StyleSheet at \a index.
    inline StyleSheet *sheet(int index) { return iSheets[index]; }
    //! Return StyleSheet at \a index.
    inline const StyleSheet *sheet(int index) const { return iSheets[index]; }

    void insert(int index, StyleSheet *sheet);
    void remove(int index);

    void saveAsXml(Stream &stream) const;

    bool has(Kind kind, Attribute sym) const;
    Attribute find(Kind, Attribute sym) const;
    const Symbol *findSymbol(Attribute sym) const;
    const Gradient *findGradient(Attribute sym) const;
    const Tiling *findTiling(Attribute sym) const;
    const Effect *findEffect(Attribute sym) const;
    const Layout *findLayout() const;
    const TextPadding *findTextPadding() const;
    const StyleSheet::TitleStyle *findTitleStyle() const;
    const StyleSheet::PageNumberStyle *findPageNumberStyle() const;
    String findPreamble() const;

    TLineCap lineCap() const;
    TLineJoin lineJoin() const;
    TFillRule fillRule() const;

    void allNames(Kind kind, AttributeSeq &seq) const;
    int findDefinition(Kind kind, Attribute sym) const;

  private:
    std::vector<StyleSheet *> iSheets;
  };

} // namespace

// --------------------------------------------------------------------
#endif
